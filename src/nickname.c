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
#include "crypt.h"

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
static Nickname*
row_to_nickname(row_t *row)
{
  Nickname *nick = MyMalloc(sizeof(Nickname));
  memset(nick, 0, sizeof(Nickname));

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
 * freed with nickname_freename.
 *
 * Returns the nickname structure on success, NULL on error or if the nickname
 * was not found.
 *
 */
Nickname*
nickname_find(const char *nickname)
{
  result_set_t *results;
  Nickname *nick;
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
nickname_register(Nickname *nick)
{
  int accid, nickid, ret, tmp;

  db_begin_transaction();

  nickid = db_nextid("nickname", "id");
  if(nickid == -1)
    goto failure;

  ret = db_execute_nonquery(INSERT_ACCOUNT, "isssi", &nickid, nick->pass,
      nick->salt, nick->email, &CurrentTime);

  if(ret == -1)
    goto failure;

  accid = db_insertid("account", "id");
  if(accid == -1)
    goto failure;

  ret = db_execute_nonquery(INSERT_NICK, "isiii", &nickid, nick->nick, &accid,
      &CurrentTime, &CurrentTime);

  if(ret == -1)
    goto failure;

  tmp = db_insertid("nickname", "id");
  if(tmp == -1)
    goto failure;

  assert(tmp == nickid);

  ret = db_execute_nonquery(SET_NICK_MASTER, "ii", &nickid, &accid);
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
nickname_delete(Nickname *nick)
{
  int newid, ret, error;

  db_begin_transaction();
 
  if(nick->nickid == nick->pri_nickid)
  {
    char *tmp = db_execute_scalar(GET_NEW_LINK, &error, "ii",
        &nick->id, &nick->nickid);
    if(error)
      goto failure;
    if(tmp == NULL)
    {
      ret = db_execute_nonquery(DELETE_NICK, "i", &nick->nickid);
      if(ret == -1)
        goto failure;
      ret = db_execute_nonquery(DELETE_ACCOUNT_CHACCESS, "i", &nick->id);
      if(ret == -1)
        goto failure;
      ret = db_execute_nonquery(DELETE_ACCOUNT_GROUPACCESS, "i", &nick->id);
      if(ret == -1)
        goto failure;
      ret = db_execute_nonquery(DELETE_ACCOUNT, "i", &nick->id);
      if(ret == -1)
        goto failure;
    }
    else
    {
      newid = atoi(tmp);
      MyFree(tmp);
      ret = db_execute_nonquery(SET_NICK_MASTER, "ii", &newid, &nick->id);
      if(ret == -1)
        goto failure;
      ret = db_execute_nonquery(DELETE_NICK, "i", &nick->nickid);
      if(ret == -1)
        goto failure;
    }
  }
  else
  {
    ret = db_execute_nonquery(DELETE_NICK, "i", &nick->nickid);
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
    MyFree(nick);
    return FALSE;
  }

  if(nick == NULL)
  {
    MyFree(nick);
    return FALSE;
  }

  MyFree(nick);
  return TRUE;
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
  Nickname *nickname;
  int ret;

  db_begin_transaction();

  if((nickname = nickname_find(nick)) != NULL)
  {
    nickname_delete(nickname);
    nickname_free(nickname);
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
    nick = db_execute_scalar(GET_NICK_FROM_ACCID, &error, "i", &id);
  else
    nick = db_execute_scalar(GET_NICK_FROM_NICKID, &error, "i", &id);
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

  if(error)
  {
    ilog(L_CRIT, "Database error %d trying to find id for nickname %s", error,
        nick);
    return -1;
  }

  if(ret == NULL)
  {
    ilog(L_DEBUG, "Nickname %s not found(id lookup)", nick);
    return 0;
  }

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
nickname_set_master(Nickname *nick, const char *master)
{
  int newid, ret;

  newid = nickname_id_from_nick(master, FALSE);

  if(newid == -1)
    return FALSE;

  ret = db_execute_nonquery(SET_NICK_MASTER, "ii", &newid, &nick->id);

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
nickname_link(Nickname *master, Nickname *child)
{
  int ret;

  db_begin_transaction();

  ret = db_execute_nonquery(SET_NICK_LINK, "ii", &master->id, &child->id);
  if(ret == -1)
    goto failure;

  ret = db_execute_nonquery(DELETE_DUPLICATE_CHACCESS, "iiii", &child->id,
      &master->id, &master->id, &child->id);
  if(ret == -1)
    goto failure;

  ret = db_execute_nonquery(MERGE_CHACCESS, "ii", &master->id, &child->id);
  if(ret == -1)
    goto failure;

  ret = db_execute_nonquery(DELETE_ACCOUNT, "i", &child->id);
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
nickname_unlink(Nickname *nick)
{
  int ret, newid, new_nickid, error;
  char *tmp;

  db_begin_transaction();

  ret = db_execute_nonquery(INSERT_NICK_CLONE, "i", &nick->id);
  if(ret == -1)
    goto failure;

  newid = db_insertid("account", "id");
  if(newid == -1)
    goto failure;

  if(nick->pri_nickid != nick->nickid)
  {
    ret = db_execute_nonquery(SET_NICK_LINK_EXCLUDE, "iii", &newid,
        &nick->id, &nick->nickid);
    if(ret == -1)
      goto failure;

    ret = db_execute_nonquery(SET_NICK_MASTER, "ii", &nick->nickid, &newid);
    if(ret == -1)
      goto failure;
  }
  else
  {
    tmp = db_execute_scalar(GET_NEW_LINK, &error, "ii",
        &nick->id, &nick->pri_nickid);
    if(error)
      goto failure;
    new_nickid = atoi(tmp);
    MyFree(tmp);
    
    ret = db_execute_nonquery(SET_NICK_MASTER, "ii", &nick->pri_nickid,
        &nick->id);
    if(ret == -1)
      goto failure;
   
    ret = db_execute_nonquery(SET_NICK_MASTER, "ii", &new_nickid, &newid);
    if(ret == -1)
      goto failure;

    ret = db_execute_nonquery(SET_NICK_LINK_EXCLUDE, "iii", &newid,
        &nick->id, &new_nickid);
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

int
nickname_link_count(Nickname *nick)
{
  int error, ret;
  char *tmp = db_execute_scalar(GET_NICK_LINKS_COUNT, &error, "i", &nick->id);

  if(error || tmp == NULL)
    return 0;

  ret = atoi(tmp);
  MyFree(tmp);

  return ret - 1;
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
nickname_save(Nickname *nick)
{
  int ret;

  db_begin_transaction();

  ret = db_execute_nonquery(SET_NICK_LAST_SEEN, "ii", &nick->nickid,
      &nick->last_seen);
  if(ret == -1)
  {
    db_rollback_transaction();
    return FALSE;
  }

  ret = db_execute_nonquery(SAVE_NICK, "sssbbbbbbbisssii", nick->url,
      nick->email, nick->cloak, &nick->enforce, &nick->secure,
      &nick->verified, &nick->cloak_on, &nick->admin, &nick->email_verified,
      &nick->priv, &nick->language, nick->last_host, nick->last_realname,
      nick->last_quit, &nick->last_quit_time, &nick->id);
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

  ret = db_execute_nonquery(INSERT_NICKACCESS, "is", &entry->id, entry->value);

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
  if(row->cols[2] != NULL)
    entry->nickname_id = atoi(row->cols[2]);
  else
    entry->nickname_id = 0;

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
nickname_accesslist_list(Nickname *nick, dlink_list *list)
{
  result_set_t *results;
  int error, i;

  results = db_execute(GET_NICKACCESS, &error, "i", &nick->id);
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
nickname_accesslist_delete(Nickname *nick, const char *value)
{
  return db_execute_nonquery(DELETE_NICKACCESS, "is", &nick->id, value);
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
nickname_accesslist_check(Nickname *nick, const char *value)
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
nickname_ajoin_add(int account_id, int channel_id)
{
  int ret = db_execute_nonquery(INSERT_AJOIN, "ii", &account_id, &channel_id);

  if(ret == -1)
    return FALSE;

  return TRUE;
}

int
nickname_ajoin_delete(int account_id, int channel_id)
{
  return db_execute_nonquery(DELETE_AJOIN, "ii", &account_id, &channel_id);
}

int
nickname_ajoin_list(int account_id, dlink_list *list)
{
  result_set_t *results;
  int error, i;

  results = db_execute(GET_AJOINS, &error, "i", &account_id);
  if(results == NULL && error != 0)
  {
    ilog(L_CRIT, "nickname_ajoin_list: database error %d", error);
    return -1;
  }
  else if(results == NULL)
    return -1;

  for(i = 0; i < results->row_count; i++)
  {
    row_t *row = &results->rows[i];
    char *entry;
    
    DupString(entry, row->cols[0]);
    dlinkAdd(entry, make_dlink_node(), list);
  }

  db_free_result(results);

  return dlink_list_length(list);
}

void
nickname_ajoinlist_free(dlink_list *list)
{
  dlink_node *ptr, *next;
  char *entry;

  ilog(L_DEBUG, "Freeing nickname ajoin list %p of length %lu", list,
      dlink_list_length(list));

  DLINK_FOREACH_SAFE(ptr, next, list->head)
  {
    entry = (char*)ptr->data;
    MyFree(entry);
    dlinkDelete(ptr, list);
    free_dlink_node(ptr);
  }
}

int
nickname_cert_list(Nickname *nick, dlink_list *list)
{
  result_set_t *results;
  int error, i;

  results = db_execute(GET_NICKCERTS, &error, "i", &nick->id);
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
nickname_cert_add(struct AccessEntry *access)
{
  int ret = db_execute_nonquery(INSERT_NICKCERT, "isi", &access->id, access->value,
    access->nickname_id > 0 ? &access->nickname_id : NULL);

  if(ret == -1)
    return FALSE;

  return TRUE;
}

int
nickname_cert_delete(Nickname *nick, const char *value)
{
  return db_execute_nonquery(DELETE_NICKCERT, "is", &nick->id, value);
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
nickname_cert_check(Nickname *nick, const char *value, 
    struct AccessEntry **retentry)
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
      if(retentry != NULL)
      {
        *retentry = MyMalloc(sizeof(struct AccessEntry));
        memcpy(*retentry, entry, sizeof(struct AccessEntry));
      }
      break;
    }
  }

  nickname_certlist_free(&list);

  return found;
}

int
nickname_link_list(unsigned int id, dlink_list *list)
{
  return db_string_list_by_id(GET_NICK_LINKS, list, id);
}

void
nickname_link_list_free(dlink_list *list)
{
  db_string_list_free(list);
}

static struct InfoList *
row_to_infolist(row_t *row, char isgroup)
{
  struct InfoList *chan = MyMalloc(sizeof(struct InfoList));
  chan->id = atoi(row->cols[0]);
  DupString(chan->name, row->cols[1]);
  chan->ilevel = atoi(row->cols[2]);
  if(!isgroup)
  {
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
  }
  else
  {
    switch(chan->ilevel)
    {
      case GRPMASTER_FLAG:
        chan->level = "MASTER";
        break;
      case GRPMEMBER_FLAG:
        chan->level = "MEMBER";
        break;
    }
  }
  return chan;
}

static int
info_list(unsigned int query, char isgroup, unsigned int id, dlink_list *list)
{
  int error, i;
  result_set_t *results;

  results = db_execute(query, &error, "i", &id);

  if(results == NULL && error != 0)
  {
    ilog(L_CRIT, "nickname_info_list: database error %d", error);
    return FALSE;
  }
  else if(results == NULL)
    return FALSE;

  if(results->row_count == 0)
    return FALSE;

  for(i = 0; i < results->row_count; ++i)
  {
    struct InfoList *chan;
    row_t *row = &results->rows[i];
    chan = row_to_infolist(row, isgroup);
    dlinkAddTail(chan, make_dlink_node(), list);
  }

  db_free_result(results);

  return dlink_list_length(list);
}

int
nickname_chan_list(unsigned int id, dlink_list *list)
{
  return info_list(GET_NICK_CHAN_INFO, FALSE, id, list);
}

static void
info_list_free(dlink_list *list)
{
  dlink_node *ptr, *next;
  struct InfoList *chan;

  ilog(L_DEBUG, "Freeing info list %p of length %lu", list,
    dlink_list_length(list));

  DLINK_FOREACH_SAFE(ptr, next, list->head)
  {
    chan = (struct InfoList *)ptr->data;
    MyFree(chan->name);
    MyFree(chan);
    dlinkDelete(ptr, list);
    free_dlink_node(ptr);
  }
}

void
nickname_chan_list_free(dlink_list *list)
{
  info_list_free(list);
}

int
nickname_group_list(unsigned int account, dlink_list *list)
{
  return info_list(GET_GROUPS_BY_ACCOUNT, TRUE, account, list);
}

void
nickname_group_list_free(dlink_list *list)
{
  info_list_free(list);
}

inline int
nickname_list_all(dlink_list *list)
{
  return db_string_list(GET_NICKS_OPER, list);
}

inline void
nickname_list_all_free(dlink_list *list)
{
  db_string_list_free(list);
}

inline int
nickname_list_regular(dlink_list *list)
{
  return db_string_list(GET_NICKS, list);
}

inline void
nickname_list_regular_free(dlink_list *list)
{
  db_string_list_free(list);
}

inline int
nickname_list_forbid(dlink_list *list)
{
  return db_string_list(GET_FORBIDS, list);
}

inline void
nickname_list_forbid_free(dlink_list *list)
{
  db_string_list_free(list);
}

inline int
nickname_list_admins(dlink_list *list)
{
  return db_string_list(GET_ADMINS, list);
}

inline void
nickname_list_admins_free(dlink_list *list)
{
  db_string_list_free(list);
}

inline Nickname *
nickname_new()
{
  return MyMalloc(sizeof(Nickname));
}

inline void
nickname_free(Nickname *nick)
{
  ilog(L_DEBUG, "Freeing nick %p for %s", nick, nickname_get_nick(nick));
  MyFree(nick->email);
  MyFree(nick->url);
  MyFree(nick->last_quit);
  MyFree(nick->last_host);
  MyFree(nick->last_realname);
  MyFree(nick);
}

unsigned int
nickname_reset_pass(Nickname *this, char **clear_pass)
{
  char new_pass[SALTLEN+1];
  char new_salt[SALTLEN+1];
  char old_pass[PASSLEN+1];
  char old_salt[SALTLEN+1];
  char *tmp_pass;
  char *cry_pass;
  int ret = TRUE;

  strlcpy(old_pass, this->pass, sizeof(old_pass));
  strlcpy(old_salt, this->salt, sizeof(old_salt));

  make_random_string(new_pass, sizeof(new_pass));
  make_random_string(new_salt, sizeof(new_salt));

  tmp_pass = MyMalloc(strlen(new_pass) + SALTLEN + 1);
  snprintf(tmp_pass, strlen(new_pass) + SALTLEN + 1, "%s%s", new_pass, new_salt);

  cry_pass = crypt_pass(tmp_pass, TRUE);

  db_begin_transaction();

  if(!nickname_set_salt(this, new_salt))
    ret = FALSE;

  if(!nickname_set_pass(this, cry_pass))
    ret = FALSE;

  if(ret == FALSE)
  {
    db_rollback_transaction();
    strlcpy(this->pass, old_pass, PASSLEN);
    strlcpy(this->salt, old_salt, SALTLEN);
    ilog(L_DEBUG, "Failed to reset pass");
  }
  else
  {
    db_commit_transaction();
    *clear_pass = MyMalloc(strlen(new_pass) + 1);
    strlcpy(*clear_pass, new_pass, sizeof(new_pass));
  }

  MyFree(tmp_pass);
  MyFree(cry_pass);

  return ret;
}

/* Nickname getters */
inline dlink_node
nickname_get_node(Nickname *this)
{
  return this->node;
}


inline unsigned int
nickname_get_id(Nickname *this)
{
  return this->id;
}

inline unsigned int
nickname_get_nickid(Nickname *this)
{
  return this->nickid;
}

inline unsigned int
nickname_get_pri_nickid(Nickname *this)
{
  return this->pri_nickid;
}

inline const char *
nickname_get_nick(Nickname *this)
{
  return this->nick;
}

inline const char *
nickname_get_pass(Nickname *this)
{
  return this->pass;
}

inline const char *
nickname_get_salt(Nickname *this)
{
  return this->salt;
}

inline const char *
nickname_get_cloak(Nickname *this)
{
  return this->cloak;
}

inline const char *
nickname_get_email(Nickname *this)
{
  return this->email;
}

inline const char *
nickname_get_url(Nickname *this)
{
  return this->url;
}

inline const char *
nickname_get_last_realname(Nickname *this)
{
  return this->last_realname;
}

inline const char *
nickname_get_last_host(Nickname *this)
{
  return this->last_host;
}

inline const char *
nickname_get_last_quit(Nickname *this)
{
  return this->last_quit;
}

inline unsigned int
nickname_get_status(Nickname *this)
{
  return this->status;
}

inline unsigned int
nickname_get_language(Nickname *this)
{
  return this->language;
}

inline unsigned char
nickname_get_enforce(Nickname *this)
{
  return this->enforce;
}

inline unsigned char
nickname_get_secure(Nickname *this)
{
  return this->secure;
}

inline unsigned char
nickname_get_verified(Nickname *this)
{
  return this->verified;
}

inline unsigned char
nickname_get_cloak_on(Nickname *this)
{
  return this->cloak_on;
}

inline unsigned char
nickname_get_admin(Nickname *this)
{
  return this->admin;
}

inline unsigned char
nickname_get_email_verified(Nickname *this)
{
  return this->email_verified;
}

inline unsigned char
nickname_get_priv(Nickname *this)
{
  return this->priv;
}

inline time_t
nickname_get_reg_time(Nickname *this)
{
  return this->reg_time;
}

inline time_t
nickname_get_last_seen(Nickname *this)
{
  return this->last_seen;
}

inline time_t
nickname_get_last_quit_time(Nickname *this)
{
  return this->last_quit_time;
}

/* Nickname setters */
inline int
nickname_set_id(Nickname *this, unsigned int value)
{
  this->id = value;
  return TRUE;
}

inline int
nickname_set_nickid(Nickname *this, unsigned int value)
{
  this->nickid = value;
  return TRUE;
}

inline int
nickname_set_pri_nickid(Nickname *this, unsigned int value)
{
  this->pri_nickid = value;
  return TRUE;
}

inline int
nickname_set_nick(Nickname *this, const char *value)
{
  strlcpy(this->nick, value, sizeof(this->nick));
  return TRUE;
}

inline int
nickname_set_pass(Nickname *this, const char *value)
{
  /* on registration we need to set before the DB knows it */
  if(this->id == 0 || db_execute_nonquery(SET_NICK_PASSWORD, "si", value, &this->id) > 0)
  {
    if(value != NULL)
      strlcpy(this->pass, value, sizeof(this->pass));
    else
      this->pass[0] = '\0';

    return TRUE;
  }
  else
    return FALSE;
}

inline int
nickname_set_salt(Nickname *this, const char *value)
{
  if(this->id == 0 || db_execute_nonquery(SET_NICK_SALT, "si", value,
        &this->id) > 0)
  {
    if(value != NULL)
      strlcpy(this->salt, value, sizeof(this->salt));
    else
      this->salt[0] = '\0';

    return TRUE;
  }
  else
    return FALSE;
}

inline int
nickname_set_cloak(Nickname *this, const char *value)
{
  if(db_execute_nonquery(SET_NICK_CLOAK, "si", value, &this->id) > 0)
  {
    if(value != NULL)
      strlcpy(this->cloak, value, sizeof(this->cloak));
    else
      this->cloak[0] = '\0';

    return TRUE;
  }
  else
    return FALSE;
}

inline int
nickname_set_email(Nickname *this, const char *value)
{
  /* on registration we need to set before the DB knows it */
  if(this->id == 0 || db_execute_nonquery(SET_NICK_EMAIL, "si", value, &this->id) > 0)
  {
    MyFree(this->email);
    if(value != NULL)
      DupString(this->email, value);
    else 
      this->email = NULL;
    return TRUE;
  }
  else
    return FALSE;
}

inline int
nickname_set_url(Nickname *this, const char *value)
{
  if(db_execute_nonquery(SET_NICK_URL, "si", value, &this->id) > 0)
  {
    MyFree(this->url);
    if(value != NULL)
      DupString(this->url, value);
    else
      this->url = NULL;
    return TRUE;
  }
  else
    return FALSE;
}

inline int
nickname_set_last_realname(Nickname *this, const char *value)
{
  if(db_execute_nonquery(SET_NICK_LAST_REALNAME, "si", value, &this->id) > 0)
  {
    MyFree(this->last_realname);
    DupString(this->last_realname, value);
    return TRUE;
  }
  else
    return FALSE;
}

inline int
nickname_set_last_host(Nickname *this, const char *value)
{
  if(db_execute_nonquery(SET_NICK_LAST_HOST, "si", value, &this->id) > 0)
  {
    MyFree(this->last_host);
    DupString(this->last_host, value);
    return TRUE;
  }
  else
    return FALSE;
}

inline int
nickname_set_last_quit(Nickname *this, const char *value)
{
  if(db_execute_nonquery(SET_NICK_LAST_QUIT, "si", value, &this->id) > 0)
  {
    MyFree(this->last_quit);
    DupString(this->last_quit, value);
    return TRUE;
  }
  else
    return FALSE;
}

inline int
nickname_set_status(Nickname *this, unsigned int value)
{
  this->status = value;
  return TRUE;
}

inline int
nickname_set_language(Nickname *this, unsigned int value)
{
  if(db_execute_nonquery(SET_NICK_LANGUAGE, "ii", &value, &this->id) > 0)
  {
    this->language = value;
    return TRUE;
  }
  else
    return FALSE;
}

inline int
nickname_set_enforce(Nickname *this, unsigned char value)
{
  if(db_execute_nonquery(SET_NICK_ENFORCE, "bi", &value, &this->id) > 0)
  {
    this->enforce = value;
    return TRUE;
  }
  else
    return FALSE;
}

inline int
nickname_set_secure(Nickname *this, unsigned char value)
{
  if(db_execute_nonquery(SET_NICK_SECURE, "bi", &value, &this->id) > 0)
  {
    this->secure = value;
    return TRUE;
  }
  else
    return FALSE;
}

inline int
nickname_set_verified(Nickname *this, unsigned char value)
{
  this->verified = value;
  return TRUE;
}

inline int
nickname_set_cloak_on(Nickname *this, unsigned char value)
{
  if(db_execute_nonquery(SET_NICK_CLOAKON, "bi", &value, &this->id) > 0)
  {
    this->cloak_on = value;
    return TRUE;
  }
  else
    return FALSE;
}

inline int
nickname_set_admin(Nickname *this, unsigned char value)
{
  if(db_execute_nonquery(SET_NICK_ADMIN, "bi", &value, &this->id) > 0)
  {
    this->admin = value;
    return TRUE;
  }
  else
    return FALSE;
}

inline int
nickname_set_email_verified(Nickname *this, unsigned char value)
{
  this->email_verified = value;
  return TRUE;
}

inline int
nickname_set_priv(Nickname *this, unsigned char value)
{
  if(db_execute_nonquery(SET_NICK_PRIVATE, "bi", &value, &this->id) > 0)
  {
    this->priv = value;
    return TRUE;
  }
  else
    return FALSE;
}

inline int
nickname_set_reg_time(Nickname *this, time_t value)
{
  this->reg_time = value;
  return TRUE;
}

inline int
nickname_set_last_seen(Nickname *this, time_t value)
{
  if(db_execute_nonquery(SET_NICK_LAST_SEEN, "ii", &value, &this->nickid) > 0)
  {
    this->last_seen = value;
    return TRUE;
  }
  else
    return FALSE;
}

inline int
nickname_set_last_quit_time(Nickname *this, time_t value)
{
  if(db_execute_nonquery(SET_NICK_LAST_QUITTIME, "ii", &value, &this->id) > 0)
  {
    this->last_quit_time = value;
    return TRUE;
  }
  else
    return FALSE;
}

