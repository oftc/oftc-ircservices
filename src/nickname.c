/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  nickname.c - nickname related functions
 *
 *  Copyright (C) 2006 Stuart Walsh and the OFTC Coding department
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 *
 *  $Id: dbm.c 1260 2007-12-07 08:53:17Z swalsh $
 */

#include "stdinc.h"
#include "dbm.h"
#include "nickname.h"
#include "nickserv.h"
#include "parse.h"
#include "language.h"
#include "chanserv.h"
#include "interface.h"
#include "msg.h"

/* 
 * row_to_nickname: 
 *
 * Converts a database row to a nickname struct, allocating the required 
 * memory in the process.  Null database fields are left as NULL pointers.  
 *
 * See the query GET_FULL_NICK for details of the field mappings.
 *
 * Returns an allocated nickname.
 *
 */
static struct Nick *
row_to_nickname(row_t *row)
{
  struct Nick *nick = MyMalloc(sizeof(struct Nick));

  nick->id = atoi(row->cols[0]);
  nick->pri_nickid = atoi(row->cols[1]);
  nick->nickid = atoi(row->cols[2]);
  strlcpy(nick->nick, row->cols[3], sizeof(nick->nick));
  strlcpy(nick->pass, row->cols[4], sizeof(nick->pass));
  strlcpy(nick->salt, row->cols[5], sizeof(nick->salt));
  if(row->cols[6] != NULL)
    DupString(nick->url, row->cols[6]);
  DupString(nick->email, row->cols[7]);
  if(row->cols[8] != NULL)
    strlcpy(nick->cloak, row->cols[8], sizeof(nick->cloak));
  nick->enforce = atoi(row->cols[9]);
  nick->secure = atoi(row->cols[10]);
  nick->verified = atoi(row->cols[11]);
  nick->cloak_on = atoi(row->cols[12]);
  nick->admin = atoi(row->cols[13]);
  nick->email_verified = atoi(row->cols[14]);
  nick->priv = atoi(row->cols[15]);
  nick->language = atoi(row->cols[16]);
  if(row->cols[17] != NULL)
    DupString(nick->last_host, row->cols[17]);
  if(row->cols[18] != NULL)
    DupString(nick->last_realname, row->cols[18]);
  if(row->cols[19] != NULL)
    DupString(nick->last_quit, row->cols[19]);
  if(row->cols[20] != NULL)
    nick->last_quit_time = atoi(row->cols[20]);
  nick->reg_time = atoi(row->cols[21]);
  nick->nick_reg_time = atoi(row->cols[22]);
  nick->last_seen = atoi(row->cols[23]);
 
  return nick;
}

/* 
 * nickname_find: 
 *
 * Searches the database for a nickname.  Returns a nickname structure for 
 * the nick if one is found.  Allocates the storage for it, which must be 
 * freed with free_nickname.
 *
 * Returns the nickname structure on success, NULL on error or if the nickname
 * was not found.
 *
 */
struct Nick *
nickname_find(const char *nickname)
{
  result_set_t *results;
  struct Nick *nick;
  int error;

  results = db_execute(GET_FULL_NICK, &error, "s", nickname);
  if(error)
  {
    ilog(L_CRIT, "Database error %d trying to find nickname %s", error,
        nickname);
    return NULL;
  }

  if(results->row_count == 0)
  {
    ilog(L_DEBUG, "Nickname %s not found.", nickname);
    db_free_result(results);
    return NULL;
  }

  nick = row_to_nickname(&results->rows[0]);
  db_free_result(results);
 
  return nick;
}

/* 
 * nickname_register: 
 *
 * Registers a nickname in the database.  Will create a new account for it.
 *
 * Returns TRUE on success or FALSE otherwise.  
 *
 * Will not update the database unless successful.
 *
 */
int
nickname_register(struct Nick *nick)
{
  int accid, nickid, ret, tmp;

  db_begin_transaction();

  nickid = db_nextid("nickname", "id");
  if(nickid == -1)
    goto failure;

  ret = db_execute_nonquery(INSERT_ACCOUNT, "isssi", nickid, nick->pass, 
      nick->salt, nick->email, CurrentTime);

  if(ret == -1)
    goto failure;

  accid = db_insertid("account", "id");
  if(accid == -1)
    goto failure;

  ret = db_execute_nonquery(INSERT_NICK, "isiii", nickid, nick->nick, accid,
      CurrentTime, CurrentTime);

  if(ret == -1)
    goto failure;

  tmp = db_insertid("nickname", "id");
  if(tmp == -1)
    goto failure;

  assert(tmp == nickid);

  ret = db_execute_nonquery(SET_NICK_MASTER, "ii", nickid, accid);
  if(ret == -1)
    goto failure;

  if(!db_commit_transaction())
    goto failure;

  nick->id = accid;
  nick->nickid = nick->pri_nickid = nickid;
  nick->nick_reg_time = nick->reg_time = CurrentTime;

  return TRUE;
  
failure:
  db_rollback_transaction();
  return FALSE;
}

/* 
 * nickname_delete: 
 *
 * Deletes the specified nickname from the database.  
 *
 * Returns TRUE on success and FALSE on failure.  
 *
 * Will not update the database unless successful.
 *
 * If the account has no more nicknames on it after this one, the account will
 * be deleted as well.  If a master is deleted, it will attempt to find a new
 * master from the remaining links.
 *
 * Raises on_nick_drop callback.
 *
 */
int
nickname_delete(struct Nick *nick)
{
  int newid, ret, error;

  db_begin_transaction();
  
  if(nick->nickid == nick->pri_nickid)
  {
    char *tmp = db_execute_scalar(GET_NEW_LINK, &error, "ii", 
        nick->id, nick->nickid);
    if(error)
      goto failure;
    if(tmp == NULL)
    {
      ret = db_execute_nonquery(DELETE_NICK, "i", nick->nickid);
      if(ret == -1)
        goto failure;
      ret = db_execute_nonquery(DELETE_ACCOUNT_CHACCESS, "i", nick->id);
      if(ret == -1)
        goto failure;
      ret = db_execute_nonquery(DELETE_ACCOUNT, "i", nick->id);
      if(ret == -1)
        goto failure;
    }
    else
    {
      newid = atoi(tmp);
      MyFree(tmp);
      ret = db_execute_nonquery(SET_NICK_MASTER, "ii", newid, nick->id);
      if(ret == -1)
        goto failure;
    }
  }
  else
  {
    ret = db_execute_nonquery(DELETE_NICK, "i", nick->nickid);
    if(ret == -1)
      goto failure;
  }

  if(!db_commit_transaction())
    return FALSE;

  execute_callback(on_nick_drop_cb, nick->id, nick->nickid, nick->pri_nickid);
  return TRUE;
failure:
  db_rollback_transaction();
  return FALSE;
}

/* 
 * nickname_is_forbid: 
 * 
 * Tests a string nickname for a match in the forbid list.
 * 
 * Returns FALSE if not found OR if a database error occured.  
 *
 * Returns TRUE if a match was found and the nickname is forbidden.
 *
 */
int 
nickname_is_forbid(const char *nickname)
{
  char *nick;
  int error;

  nick = db_execute_scalar(GET_FORBID, &error, "s", nickname);
  if(error)
  {
    ilog(L_CRIT, "Database error %d trying to test forbidden nickname %s", 
        error, nickname);
    return 0;
  }

  if(nick == NULL)
  {
    MyFree(nick);
    return 0;
  }

  MyFree(nick);
  return 1;
}

/* 
 * nickname_forbid: 
 *
 * Adds the string nickname to the forbidden list.  
 *
 * Returns TRUE if successful, FALSE otherwise.  
 *
 * If successful, this will also result in the specified nickname being 
 * deleted, following the same rules as nickname_delete.
 *
 */
int
nickname_forbid(const char *nick)
{
  struct Nick *nickname;
  int ret;

  db_begin_transaction();

  if((nickname = nickname_find(nick)) != NULL)
  {
    nickname_delete(nickname);
    free_nick(nickname);
  }

  ret = db_execute_nonquery(INSERT_FORBID, "s", nick);

  if(ret == -1)
  {
    db_rollback_transaction();
    return FALSE;
  }

  return db_commit_transaction();
}

/* 
 * nickname_delete_forbid: 
 *
 * Deletes the string nickname from the forbidden list.  
 *
 * Returns TRUE on success, FALSE otherwise.  
 *
 * The database is not updated unless successful.
 *
 */
int 
nickname_delete_forbid(const char *nick)
{
  int ret;

  ret = db_execute_nonquery(DELETE_FORBID, "s", nick);
  if(ret == -1)
    return FALSE;

  return TRUE;
}

/* 
 * nickname_nick_from_id: 
 *
 * Retrieve a nickname from the database based on its id.  
 *
 * If is_accid is TRUE, it treats the specified id as an account is and
 * looks for the primary nickname of the account.  
 *
 * If is_accid is FALSE, it treats the id specificed as a nickname and 
 * returns that specific nickname.
 * 
 * Returns the nickname if successful.  Returns NULL on error, or if the
 * nickname was actually null, which shouldnt be possible.
 *
 * The nickname returned is allocated and must be freed.
 *
 */
char *
nickname_nick_from_id(int id, int is_accid)
{
  char *nick;
  int error;

  if(is_accid)
    nick = db_execute_scalar(GET_NICK_FROM_ACCID, &error, "i", id);
  else
    nick = db_execute_scalar(GET_NICK_FROM_NICKID, &error, "i", id);
  if(error || nick == NULL)
    return NULL;

  return nick;
}

/* 
 * nickname_id_from_nick: 
 *
 * Looks up the database id for the specified nickname.
 * 
 * Returns -1 on error, or the id of the specified nickname.  If is_accid is
 * TRUE, this will be the nickname's account id, otherwise it will be the id
 * of the nickname itself.
 *
 */
int
nickname_id_from_nick(const char *nick, int is_accid)
{
  int id, error;
  char *ret;

  if(is_accid)
    ret = db_execute_scalar(GET_ACCID_FROM_NICK, &error, "s", nick);
  else
    ret = db_execute_scalar(GET_NICKID_FROM_NICK, &error, "s", nick);

  id = atoi(ret);
  MyFree(ret);

  return id;
}

/*
 * nickname_set_master: 
 * 
 * Sets the master nickname of an account to the nickname specified.  
 *
 * Returns TRUE on success, FALSE otherwise.
 *
 */
int
nickname_set_master(struct Nick *nick, const char *master)
{
  int newid, ret;

  newid = nickname_id_from_nick(master, FALSE);

  if(newid == -1)
    return FALSE;

  ret = db_execute_nonquery(SET_NICK_MASTER, "ii", newid, nick->id);

  return (ret != -1);
}

/* 
 * nickname_link:
 *
 * Links the child nickname to the master.  The child must not already be a
 * master of another nickname.
 *
 * Returns TRUE on success, FALSE otherwise.
 *
 * The child account will be deleted and the channel access lists of both
 * nicknames will be merged.
 *
 */
int
nickname_link(struct Nick *master, struct Nick *child)
{
  int ret;

  db_begin_transaction();

  ret = db_execute_nonquery(SET_NICK_LINK, "ii", master->id, child->id);
  if(ret == -1)
    goto failure;

  ret = db_execute_nonquery(DELETE_DUPLICATE_CHACCESS, "iiii", child->id,
      master->id, master->id, child->id);
  if(ret == -1)
    goto failure;

  ret = db_execute_nonquery(MERGE_CHACCESS, "ii", master->id, child->id);
  if(ret == -1)
    goto failure;

  ret = db_execute_nonquery(DELETE_ACCOUNT, "i", child->id);
  if(ret == -1)
    goto failure;

  return db_commit_transaction();

failure:
  db_rollback_transaction();
  return FALSE;
}

/* 
 * nickname_unlink:
 *
 * Unlinks the specified nickname from its master.
 *
 * Returns TRUE on success, FALSE otherwise.
 *
 * A clone of the master account will be created and the nickname will be
 * assigned to that account.  If a master is unlinked, a new master will be
 * found for the remaining chain of nicknames.
 *
 */
int
nickname_unlink(struct Nick *nick)
{
  int ret, newid, new_nickid, error;
  char *tmp;

  db_begin_transaction();

  ret = db_execute_nonquery(INSERT_NICK_CLONE, "i", nick->id);
  if(ret == -1)
    goto failure;

  newid = db_insertid("account", "id");
  if(newid == -1)
    goto failure;

  if(nick->pri_nickid != nick->nickid)
  {
    ret = db_execute_nonquery(SET_NICK_LINK_EXCLUDE, "iii", newid,
        nick->id, nick->nickid);
    if(ret == -1)
      goto failure;
  }

  tmp = db_execute_scalar(GET_NEW_LINK, &error, "ii", 
      nick->id, nick->pri_nickid);
  if(error)
    goto failure;
  new_nickid = atoi(tmp);
  MyFree(tmp);

  if(nick->nickid == nick->pri_nickid)
  {
    ret = db_execute_nonquery(SET_NICK_MASTER, "ii", nick->pri_nickid,
        nick->id);
    if(ret == -1)
      goto failure;
    
    ret = db_execute_nonquery(SET_NICK_MASTER, "ii", new_nickid, newid);
    if(ret == -1)
      goto failure;

    ret = db_execute_nonquery(SET_NICK_LINK_EXCLUDE, "iii", newid,
        nick->id, new_nickid);
    if(ret == -1)
      goto failure;
  }
  else
  {
    ret = db_execute_nonquery(SET_NICK_MASTER, "ii", nick->nickid, newid);
    if(ret == -1)
      goto failure;
  }

  if(!db_commit_transaction())
    return -1;

  return newid;
failure:
  db_rollback_transaction();
  return -1;
}

/* 
 * nickname_save:
 *
 * Saves the specified nickname to the database.
 *
 * Returns TRUE on success or FALSE otherwise.
 *
 * Mainly intended to be used by scripts or external modules.
 *
 */
int
nickname_save(struct Nick *nick)
{
  int ret;

  db_begin_transaction();

  ret = db_execute_nonquery(SET_NICK_LAST_SEEN, "ii", nick->nickid, 
      nick->last_seen);
  if(ret == -1)
  {
    db_rollback_transaction();
    return FALSE;
  }

  ret = db_execute_nonquery(SAVE_NICK, "sssbbbbbbbisssii", nick->url,
      nick->email, nick->cloak, nick->enforce, nick->secure,
      nick->verified, nick->cloak_on, nick->admin, nick->email_verified,
      nick->priv, nick->language, nick->last_host, nick->last_realname,
      nick->last_quit, nick->last_quit_time, nick->id);
  if(ret == -1)
  {
    db_rollback_transaction();
    return FALSE;
  }

  return db_commit_transaction();
}

/* 
 * nickname_accesslist_add:
 *
 * Adds the mask specified to the database attached to the specified nickname.
 *
 * Returns TRUE on success, FALSE otherwise.
 *
 */
int
nickname_accesslist_add(struct AccessEntry *entry)
{
  int ret;

  ret = db_execute_nonquery(INSERT_NICKACCESS, "is", entry->id, entry->value);

  if(ret == -1)
    return FALSE;

  return TRUE;
}

struct AccessEntry *
row_to_access_entry(row_t *row)
{
  struct AccessEntry *entry = MyMalloc(sizeof(struct AccessEntry));

  entry->id = atoi(row->cols[0]);
  DupString(entry->value, row->cols[1]);

  return entry;
}

/*
 * nickname_accesslist_list:
 *
 * Return a list containing all of the access list entries for the nickname
 * specified.
 *
 * Returns the number of entries on success, -1 on error.
 *
 * The list paramter is populated with a list of AccessEntry structures.
 *
 */
int
nickname_accesslist_list(struct Nick *nick, dlink_list *list)
{
  result_set_t *results;
  int error, i;

  results = db_execute(GET_NICKACCESS, &error, "i", nick->id);
  if(results == NULL && error != 0)
  {
    ilog(L_CRIT, "nickname_accesslist_list: database error %d", error);
    return -1;
  }
  else if(results == NULL)
    return -1;

  for(i = 0; i < results->row_count; i++)
  {
    row_t *row = &results->rows[i];
    struct AccessEntry *entry = row_to_access_entry(row);

    dlinkAdd(entry, make_dlink_node(), list);
  }

  db_free_result(results);

  return dlink_list_length(list);
}

int
nickname_accesslist_delete(struct Nick *nick, const char *value, int index)
{
  int ret;

  if(value == NULL)
    ret = db_execute_nonquery(DELETE_NICKACCESS_IDX, "ii", nick->id, index);
  else
    ret = db_execute_nonquery(DELETE_NICKACCESS, "is", nick->id, value);

  if(ret == -1)
    return FALSE;

  return TRUE;
}

void
nickname_accesslist_free(dlink_list *list)
{
  dlink_node *ptr, *next;
  struct AccessEntry *entry;

  ilog(L_DEBUG, "Freeing nickname access list %p of length %lu", list,
      dlink_list_length(list));

  DLINK_FOREACH_SAFE(ptr, next, list->head)
  {
    entry = (struct AccessEntry *)ptr->data;
    MyFree(entry->value);
    MyFree(entry);
    dlinkDelete(ptr, list);
    free_dlink_node(ptr);
  }
}

int
nickname_accesslist_check(struct Nick *nick, const char *value)
{
  dlink_list list = { 0 };
  dlink_node *ptr;
  int found = FALSE;

  nickname_accesslist_list(nick, &list);

  DLINK_FOREACH(ptr, list.head)
  {
    struct AccessEntry *entry = (struct AccessEntry *)ptr->data;

    if(match(entry->value, value))
    {
      found = TRUE;
      break;
    }
  }

  nickname_accesslist_free(&list);

  return found;
}

int
nickname_cert_list(struct Nick *nick, dlink_list *list)
{
  result_set_t *results;
  int error, i;

  results = db_execute(GET_NICKCERTS, &error, "i", nick->id);
  if(results == NULL && error != 0)
  {
    ilog(L_CRIT, "nickname_cert_list: database error %d", error);
    return -1;
  }
  else if(results == NULL)
    return -1;

  for(i = 0; i < results->row_count; i++)
  {
    row_t *row = &results->rows[i];
    struct AccessEntry *entry = row_to_access_entry(row);

    dlinkAdd(entry, make_dlink_node(), list);
  }

  db_free_result(results);

  return dlink_list_length(list);
}

int
nickname_cert_delete(struct Nick *nick, const char *value, int index)
{
  int ret;

  if(value == NULL)
    ret = db_execute_nonquery(DELETE_NICKCERT_IDX, "ii", nick->id, index);
  else
    ret = db_execute_nonquery(DELETE_NICKCERT, "is", nick->id, value);

  if(ret == -1)
    return FALSE;

  return TRUE;
}

void
nickname_certlist_free(dlink_list *list)
{
  dlink_node *ptr, *next;
  struct AccessEntry *entry;

  ilog(L_DEBUG, "Freeing nickname cert list %p of length %lu", list,
      dlink_list_length(list));

  DLINK_FOREACH_SAFE(ptr, next, list->head)
  {
    entry = (struct AccessEntry *)ptr->data;
    MyFree(entry->value);
    MyFree(entry);
    dlinkDelete(ptr, list);
    free_dlink_node(ptr);
  }
}

int
nickname_cert_check(struct Nick *nick, const char *value)
{
  dlink_list list = { 0 };
  dlink_node *ptr;
  int found = FALSE;

  nickname_cert_list(nick, &list);

  DLINK_FOREACH(ptr, list.head)
  {
    struct AccessEntry *entry = (struct AccessEntry *)ptr->data;

    if(match(entry->value, value))
    {
      found = TRUE;
      break;
    }
  }

  nickname_certlist_free(&list);

  return found;
}

int
nickname_link_list(unsigned int id, dlink_list *list)
{
  return db_string_list(GET_NICK_LINKS, list, "i", id);
}

void
nickname_link_list_free(dlink_list *list)
{
  db_string_list_free(list);
}

static struct InfoChanList *
row_to_infochanlist(row_t *row)
{
  struct InfoChanList *chan = MyMalloc(sizeof(struct InfoChanList));
  chan->channel_id = atoi(row->cols[0]);
  DupString(chan->channel, row->cols[1]);
  chan->ilevel = atoi(row->cols[2]);
  switch(chan->ilevel)
  {
    case MASTER_FLAG:
      chan->level = "MASTER";
      break;
    case CHANOP_FLAG:
      chan->level = "CHANOP";
      break;
    case MEMBER_FLAG:
      chan->level = "MEMBER";
      break;
  }
  return chan;
}

int
nickname_chan_list(unsigned int id, dlink_list *list)
{
  int error, i;
  result_set_t *results;

  results = db_execute(GET_NICK_CHAN_INFO, &error, "i", id);

  if(results == NULL && error != 0)
  {
    ilog(L_CRIT, "nickname_chan_list: database error %d", error);
    return FALSE;
  }
  else if(results == NULL)
    return FALSE;

  if(results->row_count == 0)
    return FALSE;

  for(i = 0; i < results->row_count; ++i)
  {
    struct InfoChanList *chan;
    row_t *row = &results->rows[i];
    chan = row_to_infochanlist(row);
    dlinkAdd(chan, make_dlink_node(), list);
  }

  db_free_result(results);

  return dlink_list_length(list);
}

void
nickname_chan_list_free(dlink_list *list)
{
  dlink_node *ptr, *next;
  struct InfoChanList *chan;

  ilog(L_DEBUG, "Freeing string list %p of length %lu", list,
    dlink_list_length(list));

  DLINK_FOREACH_SAFE(ptr, next, list->head)
  {
    chan = (struct InfoChanList *)ptr->data;
    MyFree(chan->channel);
    MyFree(chan);
    dlinkDelete(ptr, list);
    free_dlink_node(ptr);
  }
}
