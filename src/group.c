/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  group.c - group related functions
 *
 *  Copyright (C) 2009 Stuart Walsh and the OFTC Coding department
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
#include "group.h"
#include "parse.h"
#include "language.h"
#include "chanserv.h"
#include "interface.h"
#include "msg.h"
#include "crypt.h"

/*
 * row_to_group:
 *
 * Converts a database row to a group struct, allocating the required
 * memory in the process.  Null database fields are left as NULL pointers. 
 *
 * See the query GET_FULL_GROUP for details of the field mappings.
 *
 * Returns an allocated group.
 *
 */
static Group*
row_to_group(row_t *row)
{
  Group *group = MyMalloc(sizeof(Group));
  memset(group, 0, sizeof(Group));

  group->id = atoi(row->cols[0]);
  strlcpy(group->name, row->cols[1], sizeof(group->name));
  if(row->cols[2] != NULL)
    DupString(group->desc, row->cols[2]);
  DupString(group->email, row->cols[3]);
  DupString(group->url, row->cols[4]);
  group->priv = atoi(row->cols[5]);
  group->reg_time = atoi(row->cols[6]);

  return group;
}

/*
 * group_find:
 *
 * Searches the database for a group.  Returns a group structure for
 * the group if one is found.  Allocates the storage for it, which must be
 * freed with group_freename.
 *
 * Returns the group structure on success, NULL on error or if the group
 * was not found.
 *
 */
Group*
group_find(const char *grp)
{
  result_set_t *results;
  Group *group;
  int error;

  results = db_execute(GET_FULL_GROUP, &error, "s", grp);
  if(error)
  {
    ilog(L_CRIT, "Database error %d trying to find group %s", error,
        grp);
    return NULL;
  }

  if(results->row_count == 0)
  {
    ilog(L_DEBUG, "Group %s not found.", grp);
    db_free_result(results);
    return NULL;
  }

  group = row_to_group(&results->rows[0]);
  db_free_result(results);

  return group;
}

/*
 * group_register:
 *
 * Registers a group in the database.  Will create a new account for it.
 *
 * Returns TRUE on success or FALSE otherwise. 
 *
 * Will not update the database unless successful.
 *
 */
int
group_register(Group *group, Nickname *master)
{
  int ret;
  unsigned int id, flag;

  db_begin_transaction();

  ret = db_execute_nonquery(INSERT_GROUP, "ssi", group->name, group->desc,
          &CurrentTime);

  if(ret == -1)
    goto failure;

  group->id = db_insertid("group", "id");
  if(group->id == -1)
    goto failure;

  id = nickname_get_id(master);
  flag = GRPMASTER_FLAG;
  ret = db_execute_nonquery(INSERT_GROUPACCESS, "iii", &id, &group->id, &flag);

  if(ret == -1)
    goto failure;

  if(!db_commit_transaction())
    goto failure;

  group->reg_time = CurrentTime;

  return TRUE;
 
failure:
  db_rollback_transaction();
  return FALSE;
}

/*
 * group_delete:
 *
 * Deletes the specified group from the database. 
 *
 * Returns TRUE on success and FALSE on failure. 
 *
 * Will not update the database unless successful.
 *
 * If the account has no more groups on it after this one, the account will
 * be deleted as well.  If a master is deleted, it will attempt to find a new
 * master from the remaining links.
 *
 * Raises on_group_drop callback.
 *
 */
int
group_delete(Group *group)
{
  int ret;

  ret = db_execute_nonquery(DELETE_GROUP, "i", &group->id);
  if(ret == -1)
      return FALSE;

  execute_callback(on_group_drop_cb, group->id);
  return TRUE;
}

#if 0
/*
 * group_is_forbid:
 *
 * Tests a string group for a match in the forbid list.
 *
 * Returns FALSE if not found OR if a database error occured. 
 *
 * Returns TRUE if a match was found and the group is forbidden.
 *
 */
int
group_is_forbid(const char *group)
{
  char *group;
  int error;

  group = db_execute_scalar(GET_FORBID, &error, "s", group);
  if(error)
  {
    ilog(L_CRIT, "Database error %d trying to test forbidden group %s",
        error, group);
    MyFree(group);
    return FALSE;
  }

  if(group == NULL)
  {
    MyFree(group);
    return FALSE;
  }

  MyFree(group);
  return TRUE;
}

/*
 * group_forbid:
 *
 * Adds the string group to the forbidden list. 
 *
 * Returns TRUE if successful, FALSE otherwise. 
 *
 * If successful, this will also result in the specified group being
 * deleted, following the same rules as group_delete.
 *
 */
int
group_forbid(const char *group)
{
  Group *group;
  int ret;

  db_begin_transaction();

  if((group = group_find(group)) != NULL)
  {
    group_delete(group);
    group_free(group);
  }

  ret = db_execute_nonquery(INSERT_FORBID, "s", group);

  if(ret == -1)
  {
    db_rollback_transaction();
    return FALSE;
  }

  return db_commit_transaction();
}

/*
 * group_delete_forbid:
 *
 * Deletes the string group from the forbidden list. 
 *
 * Returns TRUE on success, FALSE otherwise. 
 *
 * The database is not updated unless successful.
 *
 */
int
group_delete_forbid(const char *group)
{
  int ret;

  ret = db_execute_nonquery(DELETE_FORBID, "s", group);
  if(ret == -1)
    return FALSE;

  return TRUE;
}
#endif

/*
 * group_name_from_id:
 *
 * Retrieve a group from the database based on its id. 
 *
 * Returns the group if successful.  Returns NULL on error, or if the
 * group was actually null, which shouldnt be possible.
 *
 * The group returned is allocated and must be freed.
 *
 */
char *
group_name_from_id(int id)
{
  char *group;
  int error;

  group = db_execute_scalar(GET_GROUP_FROM_GROUPID, &error, "i", &id);
  if(error || group == NULL)
    return NULL;

  return group;
}

/*
 * group_id_from_name:
 *
 * Looks up the database id for the specified group.
 *
 * Returns -1 on error, or the id of the specified group.  
 *
 */
int
group_id_from_name(const char *group)
{
  int id, error;
  char *ret;

  ret = db_execute_scalar(GET_GROUPID_FROM_GROUP, &error, "s", group);

  if(error)
  {
    ilog(L_CRIT, "Database error %d trying to find id for group %s", error,
        group);
    return -1;
  }

  if(ret == NULL)
  {
    ilog(L_DEBUG, "Group %s not found(id lookup)", group);
    return 0;
  }

  id = atoi(ret);
  MyFree(ret);

  return id;
}

int
group_masters_list(unsigned int id, dlink_list *list)
{
  return db_string_list_by_id(GET_GROUP_MASTERS, list, id);
}

void
group_masters_list_free(dlink_list *list)
{
  db_string_list_free(list);
}

int
group_masters_count(unsigned int id, int *count)
{
  int error;

  *count = atoi(db_execute_scalar(GET_GROUP_MASTER_COUNT, &error, "i", &id));
  if(error)
  {
    *count = -1;
    return FALSE;
  }

  return TRUE;
}

int
group_list_all(dlink_list *list)
{
  return db_string_list(GET_GROUPS_OPER, list);
}

int
group_list_regular(dlink_list *list)
{
  return db_string_list(GET_GROUPS, list);
}

#if 0
/*
 * group_save:
 *
 * Saves the specified group to the database.
 *
 * Returns TRUE on success or FALSE otherwise.
 *
 * Mainly intended to be used by scripts or external modules.
 *
 */
int
group_save(Group *group)
{
  int ret;

  ret = db_execute_nonquery(SAVE_GROUP, "sssbbbbbbbisssii", group->desc, group->url,
      group->email, group->cloak, &group->enforce, &group->secure,
      &group->verified, &group->cloak_on, &group->admin, &group->email_verified,
      &group->priv, &group->language, group->last_host, group->last_realname,
      group->last_quit, &group->last_quit_time, &group->id);
  if(ret == -1)
    return FALSE;

  return TRUE;
}

/*
 * group_accesslist_add:
 *
 * Adds the mask specified to the database attached to the specified group.
 *
 * Returns TRUE on success, FALSE otherwise.
 *
 */
int
group_accesslist_add(struct AccessEntry *entry)
{
  int ret;

  ret = db_execute_nonquery(INSERT_GROUPACCESS, "is", &entry->id, entry->value);

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
    entry->group_id = atoi(row->cols[2]);
  else
    entry->group_id = 0;

  return entry;
}

/*
 * group_accesslist_list:
 *
 * Return a list containing all of the access list entries for the group
 * specified.
 *
 * Returns the number of entries on success, -1 on error.
 *
 * The list paramter is populated with a list of AccessEntry structures.
 *
 */
int
group_accesslist_list(Group *group, dlink_list *list)
{
  result_set_t *results;
  int error, i;

  results = db_execute(GET_GROUPACCESS, &error, "i", &group->id);
  if(results == NULL && error != 0)
  {
    ilog(L_CRIT, "group_accesslist_list: database error %d", error);
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
group_accesslist_delete(Group *group, const char *value)
{
  return db_execute_nonquery(DELETE_GROUPACCESS, "is", &group->id, value);
}

void
group_accesslist_free(dlink_list *list)
{
  dlink_node *ptr, *next;
  struct AccessEntry *entry;

  ilog(L_DEBUG, "Freeing group access list %p of length %lu", list,
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
group_accesslist_check(Group *group, const char *value)
{
  dlink_list list = { 0 };
  dlink_node *ptr;
  int found = FALSE;

  group_accesslist_list(group, &list);

  DLINK_FOREACH(ptr, list.head)
  {
    struct AccessEntry *entry = (struct AccessEntry *)ptr->data;

    if(match(entry->value, value))
    {
      found = TRUE;
      break;
    }
  }

  group_accesslist_free(&list);

  return found;
}

int
group_cert_list(Group *group, dlink_list *list)
{
  result_set_t *results;
  int error, i;

  results = db_execute(GET_GROUPCERTS, &error, "i", &group->id);
  if(results == NULL && error != 0)
  {
    ilog(L_CRIT, "group_cert_list: database error %d", error);
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
group_cert_add(struct AccessEntry *access)
{
  int ret = db_execute_nonquery(INSERT_GROUPCERT, "isi", &access->id, access->value,
    access->group_id > 0 ? &access->group_id : NULL);

  if(ret == -1)
    return FALSE;

  return TRUE;
}

int
group_cert_delete(Group *group, const char *value)
{
  return db_execute_nonquery(DELETE_GROUPCERT, "is", &group->id, value);
}

void
group_certlist_free(dlink_list *list)
{
  dlink_node *ptr, *next;
  struct AccessEntry *entry;

  ilog(L_DEBUG, "Freeing group cert list %p of length %lu", list,
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
group_cert_check(Group *group, const char *value, 
    struct AccessEntry **retentry)
{
  dlink_list list = { 0 };
  dlink_node *ptr;
  int found = FALSE;

  group_cert_list(group, &list);

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

  group_certlist_free(&list);

  return found;
}

int
group_link_list(unsigned int id, dlink_list *list)
{
  return db_string_list_by_id(GET_GROUP_LINKS, list, id);
}

void
group_link_list_free(dlink_list *list)
{
  db_string_list_free(list);
}
#endif

static struct InfoList *
row_to_infochanlist(row_t *row)
{
  struct InfoList *chan = MyMalloc(sizeof(struct InfoList));
  chan->id = atoi(row->cols[0]);
  DupString(chan->name, row->cols[1]);
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
group_chan_list(unsigned int id, dlink_list *list)
{
  int error, i;
  result_set_t *results;

  results = db_execute(GET_GROUP_CHAN_INFO, &error, "i", &id);

  if(results == NULL && error != 0)
  {
    ilog(L_CRIT, "group_chan_list: database error %d", error);
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
    chan = row_to_infochanlist(row);
    dlinkAddTail(chan, make_dlink_node(), list);
  }

  db_free_result(results);

  return dlink_list_length(list);
}

void
group_chan_list_free(dlink_list *list)
{
  dlink_node *ptr, *next;
  struct InfoList *chan;

  ilog(L_DEBUG, "Freeing group chan list %p of length %lu", list,
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

#if 0
void
group_list_all_free(dlink_list *list)
{
  db_string_list_free(list);
}


void
group_list_regular_free(dlink_list *list)
{
  db_string_list_free(list);
}

int
group_list_forbid(dlink_list *list)
{
  return db_string_list(GET_FORBIDS, list);
}

void
group_list_forbid_free(dlink_list *list)
{
  db_string_list_free(list);
}

int
group_list_admins(dlink_list *list)
{
  return db_string_list(GET_ADMINS, list);
}

void
group_list_admins_free(dlink_list *list)
{
  db_string_list_free(list);
}
#endif

Group *
group_new()
{
  return MyMalloc(sizeof(Group));
}
void
group_free(Group *group)
{
  ilog(L_DEBUG, "Freeing group %p for %s", group, group_get_name(group));
  MyFree(group->email);
  MyFree(group->url);
  MyFree(group->desc);
  MyFree(group);
}

#if 0
unsigned int
group_reset_pass(Group *this, char **clear_pass)
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

  if(!group_set_salt(this, new_salt))
    ret = FALSE;

  if(!group_set_pass(this, cry_pass))
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

/* Group getters */
dlink_node
group_get_node(Group *this)
{
  return this->node;
}
#endif

unsigned int
group_get_id(Group *this)
{
  return this->id;
}

#if 0
unsigned int
group_get_nameid(Group *this)
{
  return this->groupid;
}

unsigned int
group_get_pri_groupid(Group *this)
{
  return this->pri_groupid;
}
#endif

const char *
group_get_name(Group *this)
{
  return this->name;
}

#if 0
const char *
group_get_pass(Group *this)
{
  return this->pass;
}

const char *
group_get_salt(Group *this)
{
  return this->salt;
}

const char *
group_get_cloak(Group *this)
{
  return this->cloak;
}
#endif

const char *
group_get_desc(Group *this)
{
  return this->desc;
}

const char *
group_get_email(Group *this)
{
  return this->email;
}

const char *
group_get_url(Group *this)
{
  return this->url;
}

#if 0
const char *
group_get_last_realname(Group *this)
{
  return this->last_realname;
}

const char *
group_get_last_host(Group *this)
{
  return this->last_host;
}

const char *
group_get_last_quit(Group *this)
{
  return this->last_quit;
}

unsigned int
group_get_status(Group *this)
{
  return this->status;
}

unsigned int
group_get_language(Group *this)
{
  return this->language;
}

unsigned char
group_get_enforce(Group *this)
{
  return this->enforce;
}

unsigned char
group_get_secure(Group *this)
{
  return this->secure;
}

unsigned char
group_get_verified(Group *this)
{
  return this->verified;
}

unsigned char
group_get_cloak_on(Group *this)
{
  return this->cloak_on;
}

unsigned char
group_get_admin(Group *this)
{
  return this->admin;
}

unsigned char
group_get_email_verified(Group *this)
{
  return this->email_verified;
}
#endif

unsigned char
group_get_priv(Group *this)
{
  return this->priv;
}

time_t
group_get_regtime(Group *this)
{
  return this->reg_time;
}

/* Group setters */
int
group_set_id(Group *this, unsigned int value)
{
  this->id = value;
  return TRUE;
}

int
group_set_name(Group *this, const char *value)
{
  strlcpy(this->name, value, sizeof(this->name));
  return TRUE;
}

int
group_set_email(Group *this, const char *value)
{
  /* on registration we need to set before the DB knows it */
  if(this->id == 0 || db_execute_nonquery(SET_GROUP_EMAIL, "si", value, &this->id) > 0)
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

int
group_set_url(Group *this, const char *value)
{
  if(db_execute_nonquery(SET_GROUP_URL, "si", value, &this->id) > 0)
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

int
group_set_desc(Group *this, const char *value)
{
  if(this->id == 0 ||
      db_execute_nonquery(SET_GROUP_DESC, "si", value, &this->id) > 0)
  {
    MyFree(this->desc);
    if(value != NULL)
      DupString(this->desc, value);
    else
      this->desc = NULL;
    return TRUE;
  }
  else
    return FALSE;
}

int
group_set_priv(Group *this, unsigned char value)
{
  if(db_execute_nonquery(SET_GROUP_PRIVATE, "bi", &value, &this->id) > 0)
  {
    this->priv = value;
    return TRUE;
  }
  else
    return FALSE;
}

int
group_set_regtime(Group *this, time_t value)
{
  this->reg_time = value;
  return TRUE;
}
