/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  dbm.c: The database manager
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
 *  $Id$
 */

#include <string>
#include <vector>
#include <tr1/unordered_map>

#include "stdinc.h"
#include "dbm.h"
#include "language.h"
#include "interface.h"
#include "chanserv.h"
#include "nickserv.h"
#include "msg.h"
#include "conf/database.h"
#include <yada.h>

struct Callback *on_nick_drop_cb;

query_t queries[QUERY_COUNT] = { 
  { "SELECT account.id, (SELECT nick FROM nickname WHERE id=account.primary_nick), "
    "password, salt, url, email, cloak, flag_enforce, "
    "flag_secure, flag_verified, flag_cloak_enabled, "
    "flag_admin, flag_email_verified, language, last_host, last_realname, "
    "last_quit_msg, last_quit_time, account.reg_time, nickname.reg_time, "
    "last_seen FROM account, nickname WHERE account.id = nickname.user_id AND "
    "lower(nick) = lower(?v)", NULL, QUERY },
  { "SELECT nick from account, nickname WHERE account.id=?d AND "
    "account.primary_nick=nickname.id", NULL, QUERY },
  { "SELECT user_id from nickname WHERE lower(nick)=lower(?v)", NULL, QUERY },
  { "INSERT INTO account (password, salt, email, reg_time) VALUES "
    "(?v, ?v, ?v, ?d)", NULL, EXECUTE },
  { "INSERT INTO nickname (nick, user_id, reg_time, last_seen) VALUES "
    "(?v, ?d, ?d, ?d)", NULL, EXECUTE },
  { "DELETE FROM nickname WHERE lower(nick)=lower(?v)", NULL, EXECUTE },
  { "DELETE FROM account WHERE id=?d", NULL, EXECUTE },
  { "INSERT INTO account_access (parent_id, entry) VALUES(?d, ?v)", 
    NULL, EXECUTE },
  { "SELECT id, entry FROM account_access WHERE parent_id=?d", NULL, QUERY },
  { "SELECT nick FROM account,nickname WHERE flag_admin=true AND "
    "account.primary_nick = nickname.id", NULL, QUERY },
  { "SELECT akill.id, setter, mask, reason, time, duration FROM akill", 
    NULL, QUERY },
  { "SELECT id, channel_id, account_id, level FROM channel_access WHERE "
    "channel_id=?d", NULL, QUERY },
  { "SELECT id from channel WHERE lower(channel)=lower(?v)", NULL, QUERY },
  { "SELECT id, channel, description, entrymsg, flag_forbidden, flag_private, "
    "flag_restricted, flag_topic_lock, flag_verbose, flag_autolimit, "
      "flag_expirebans, url, email, topic, mlock FROM channel WHERE "
      "lower(channel)=lower(?v)", NULL, QUERY },
  { "INSERT INTO channel (channel, description) VALUES(?v, ?v)", NULL, EXECUTE },
  { "INSERT INTO channel_access (account_id, channel_id, level) VALUES "
    "(?d, ?d, ?d)", NULL, EXECUTE } ,
  { "UPDATE channel_access SET level=?d WHERE account_id=?d", NULL, EXECUTE },
  { "DELETE FROM channel_access WHERE channel_id=?d AND account_id=?d", 
    NULL, EXECUTE },
  { "SELECT id, channel_id, account_id, level FROM channel_access WHERE "
    "channel_id=?d AND account_id=?d", NULL, QUERY },
  { "DELETE FROM channel WHERE lower(channel)=lower(?v)", NULL, EXECUTE },
  { "SELECT id, mask, reason, setter, time, duration FROM akill WHERE mask=?v",
    NULL, QUERY },
  { "INSERT INTO akill (mask, reason, setter, time, duration) "
      "VALUES(?v, ?v, ?d, ?d, ?d)", NULL, EXECUTE },
  { "UPDATE account SET password=?v WHERE id=?d", NULL, EXECUTE },
  { "UPDATE account SET url=?v WHERE id=?d", NULL, EXECUTE },
  { "UPDATE account SET email=?v WHERE id=?d", NULL, EXECUTE },
  { "UPDATE account SET cloak=?v WHERE id=?d", NULL, EXECUTE },
  { "UPDATE account SET last_quit_msg=?v WHERE id=?d", NULL, EXECUTE },
  { "UPDATE account SET last_host=?v WHERE id=?d", NULL, EXECUTE },
  { "UPDATE account SET last_realname=?v where id=?d", NULL, EXECUTE },
  { "UPDATE account SET language=?d WHERE id=?d", NULL, EXECUTE },
  { "UPDATE account SET last_quit_time=?d WHERE id=?d", NULL, EXECUTE },
  { "UPDATE nickname SET last_seen=?d WHERE user_id=?d", NULL, EXECUTE },
  { "UPDATE account SET flag_cloak_enabled=?B WHERE id=?d", NULL, EXECUTE },
  { "UPDATE account SET flag_secure=?B WHERE id=?d", NULL, EXECUTE },
  { "UPDATE account SET flag_enforce=?B WHERE id=?d", NULL, EXECUTE },
  { "UPDATE account SET flag_admin=?B WHERE id=?d", NULL, EXECUTE },
  { "DELETE FROM account_access WHERE parent_id=?d AND entry=?v", NULL,
    EXECUTE },
  { "DELETE FROM account_access WHERE parent_id=?d", NULL, EXECUTE },
  { "DELETE FROM account_access WHERE id = "
          "(SELECT id FROM account_access AS a WHERE ?d = "
          "(SELECT COUNT(id)+1 FROM account_access AS b WHERE b.id < a.id AND "
          "b.parent_id = ?d) AND parent_id = ?d)", NULL, EXECUTE },
  { "UPDATE nickname SET user_id=?d WHERE user_id=?d", NULL, EXECUTE },
  { "INSERT INTO account (password, salt, url, email, cloak, flag_enforce, "
    "flag_secure, flag_verified, flag_cloak_enabled, "
    "flag_admin, flag_email_verified, language, last_host, last_realname, "
    "last_quit_msg, last_quit_time, reg_time) SELECT password, salt, url, "
    "email, cloak, flag_enforce, flag_secure, flag_verified, "
    "flag_cloak_enabled, flag_admin, flag_email_verified, language, last_host, "
    "last_realname, last_quit_msg, last_quit_time, reg_time FROM account "
    "WHERE id=?d", NULL, EXECUTE },
  { "SELECT nick FROM nickname WHERE user_id=?d", NULL, QUERY },
  { "UPDATE channel SET description=?v WHERE id=?d", NULL, EXECUTE },
  { "UPDATE channel SET url=?v WHERE id=?d", NULL, EXECUTE },
  { "UPDATE channel SET email=?v WHERE id=?d", NULL, EXECUTE },
  { "UPDATE channel SET entrymsg=?v WHERE id=?d", NULL, EXECUTE },
  { "UPDATE channel SET topic=?v WHERE id=?d", NULL, EXECUTE },
  { "UPDATE channel SET mlock=?v WHERE id=?d", NULL, EXECUTE },
  { "UPDATE channel SET flag_forbidden=?B WHERE id=?d", NULL, EXECUTE },
  { "UPDATE channel SET flag_private=?B WHERE id=?d", NULL, EXECUTE },
  { "UPDATE channel SET flag_restricted=?B WHERE id=?d", NULL, EXECUTE },
  { "UPDATE channel SET flag_topic_lock=?B WHERE id=?d", NULL, EXECUTE },
  { "UPDATE channel SET flag_verbose=?B WHERE id=?d", NULL, EXECUTE },
  { "UPDATE channel SET flag_autolimit=?B WHERE id=?d", NULL, EXECUTE },
  { "UPDATE channel SET flag_expirebans=?B WHERE id=?d", NULL, EXECUTE },
  { "INSERT INTO forbidden_nickname (nick) VALUES (?v)", NULL, EXECUTE },
  { "SELECT nick FROM forbidden_nickname WHERE lower(nick)=lower(?v)",
    NULL, QUERY },
  { "DELETE FROM forbidden_nickname WHERE lower(nick)=lower(?v)", 
    NULL, EXECUTE },
  { "INSERT INTO channel_akick (channel_id, target, setter, reason, "
    "time, duration) VALUES (?d, ?d, ?d, ?v, ?d, ?d)", NULL, EXECUTE },
  { "INSERT INTO channel_akick (channel_id, setter, reason, mask, "
    "time, duration) VALUES (?d, ?d, ?v, ?v, ?d, ?d)", NULL, EXECUTE },
  { "SELECT channel_akick.id, channel.channel, target, setter, mask, reason, time, duration FROM "
    "channel_akick, channel WHERE channel_id=?d AND channel.id=channel_id", 
    NULL, QUERY },
  { "DELETE FROM channel_akick WHERE id = "
          "(SELECT id FROM channel_akick AS a WHERE ?d = "
          "(SELECT COUNT(id)+1 FROM channel_akick AS b WHERE b.id < a.id AND "
          "b.channel_id = ?d) AND channel_id = ?d)", NULL, EXECUTE },
  { "DELETE FROM channel_akick WHERE channel_id=?d AND mask=?v", NULL, 
    EXECUTE },
  { "DELETE FROM channel_akick WHERE channel_id=?d AND target IN (SELECT user_id "
    "FROM nickname WHERE lower(nick)=lower(?v))", NULL, EXECUTE },
  { "UPDATE account SET primary_nick=(SELECT id FROM nickname WHERE "
    "lower(nick)=lower(?v)) WHERE id=?d", NULL, EXECUTE },
  { "DELETE FROM akill WHERE mask=?v", NULL, EXECUTE },
  { "SELECT COUNT(id) FROM channel_access WHERE channel_id=?d AND level=?d",
    NULL, QUERY },
};

void
init_db()
{
  char *dbstr;

  dbstr = (char *)MyMalloc(strlen(Database.driver) + strlen(Database.hostname) + strlen(Database.dbname) +
      3 /* ::: */ + 1);
  sprintf(dbstr, "%s:::%s", Database.driver, Database.dbname);

  Database.yada = yada_init(dbstr, 0);
  MyFree(dbstr);
  on_nick_drop_cb = register_callback("Nick DROP Callback", NULL);
}

void
cleanup_db()
{
  Database.yada->disconnect(Database.yada);
  Database.yada->destroy(Database.yada);
}

void
db_load_driver()
{
  int i;

  if(Database.yada->connect(Database.yada, Database.username, 
        Database.password) == 0)
    printf("db: Failed to connect to database %s\n", Database.yada->errmsg);
  else
    printf("db: Database connection succeeded.\n");

  for(i = 0; i < QUERY_COUNT; i++)
  {
    query_t *query = &queries[i];
    if(query->name == NULL)
      continue;
    query->rc = Prepare((char*)query->name, 0);
  }
}

#define db_query(ret, query_id, args...)                              \
{                                                                     \
  int __id = query_id;                                                \
  yada_rc_t *__result;                                                \
  query_t *__query;                                                   \
                                                                      \
  __query = &queries[__id];                                           \
  printf("db_query: %d %s\n", __id, __query->name);                   \
  assert(__query->type == QUERY);                                     \
  assert(__query->rc);                                                \
                                                                      \
  __result = Query(__query->rc, args);                                \
  if(__result == NULL)                                                \
    printf("db_query: %d Failed: %s\n", __id, Database.yada->errmsg); \
                                                                      \
  ret = __result;                                                     \
}

#define db_exec(ret, query_id, args...)                               \
{                                                                     \
  int __id = query_id;                                                \
  int __result;                                                       \
  query_t *__query;                                                   \
                                                                      \
  __query = &queries[__id];                                           \
  printf("db_exec: %d %s\n", __id, __query->name);                    \
  assert(__query->type == EXECUTE);                                   \
  assert(__query->rc);                                                \
                                                                      \
  __result = Execute(__query->rc, args);                              \
  if(__result == -1)                                                  \
    printf("db_exec: %d Failed: %s\n", __id, Database.yada->errmsg);  \
                                                                      \
  ret = __result;                                                     \
}

struct Nick *
db_find_nick(const char *nick)
{
  yada_rc_t *rc, *brc;
  struct Nick *nick_p;
  char *retnick, *retpass, *retcloak, *retsalt;

  assert(nick != NULL);

  db_query(rc, GET_FULL_NICK, nick);

  if(rc == NULL)
    return NULL;
 
  nick_p = (struct Nick *)MyMalloc(sizeof(struct Nick));
 
  brc = Bind("?d?ps?ps?ps?ps?ps?ps?B?B?B?B?B?B?d?ps?ps?ps?d?d?d?d",
    &nick_p->id, &retnick, &retpass, &retsalt, &nick_p->url, &nick_p->email,
    &retcloak, &nick_p->enforce, &nick_p->secure, &nick_p->verified, 
    &nick_p->cloak_on, &nick_p->admin, 
    &nick_p->email_verified, &nick_p->language, &nick_p->last_host,
    &nick_p->last_realname, &nick_p->last_quit,
    &nick_p->last_quit_time, &nick_p->reg_time, &nick_p->nick_reg_time,
    &nick_p->last_seen);

  if(Fetch(rc, brc) == 0)
  {
    printf("db_find_nick: '%s' not found.\n", nick);
    Free(brc);
    Free(rc);
    return NULL;
  }

  strlcpy(nick_p->nick, retnick, sizeof(nick_p->nick));
  strlcpy(nick_p->pass, retpass, sizeof(nick_p->pass));
  strlcpy(nick_p->salt, retsalt, sizeof(nick_p->salt));
  if(retcloak)
    strlcpy(nick_p->cloak, retcloak, sizeof(nick_p->cloak));

  DupString(nick_p->url, nick_p->url);
  DupString(nick_p->email, nick_p->email);
  DupString(nick_p->last_host, nick_p->last_host);
  DupString(nick_p->last_realname, nick_p->last_realname);
  DupString(nick_p->last_quit, nick_p->last_quit);

  printf("db_find_nick: Found nick %s(asked for %s)\n", nick_p->nick, nick);

  Free(brc);
  Free(rc);

  return nick_p;
}

char *
db_get_nickname_from_id(unsigned int id)
{
  yada_rc_t *rc, *brc; 
  char *retnick; 

  db_query(rc, GET_NICK_FROM_ID, id);

  if(rc == NULL)
    return NULL;

  brc = Bind("?ps", &retnick);
  if(Fetch(rc, brc) == 0)
  {
    printf("db_get_nickname_from_id: %d not found.\n", id);
    Free(brc);
    Free(rc);
    return NULL;
  }

  DupString(retnick, retnick);

  Free(brc);
  Free(rc);

  return retnick;
}

unsigned int
db_get_id_from_name(const char *name, unsigned int type)
{
  yada_rc_t *rc, *brc;
  int ret;

  db_query(rc, type, name);

  if(rc == NULL)
    return 0;

  brc = Bind("?d", &ret);
  if(Fetch(rc, brc) == 0)
  {
    printf("db_get_id_from_name: '%s' not found.\n", name);
    Free(brc);
    Free(rc);
    return 0;
  }

  Free(brc);
  Free(rc);
  
  return ret;  
}

int
db_register_nick(struct Nick *nick)
{
  int exec, id;

  assert(nick != NULL);

  TransBegin();

  db_exec(exec, INSERT_ACCOUNT, nick->pass, nick->salt, nick->email, 
      CurrentTime);

  if(exec != -1)
  {
    id = InsertID("account", "id");
    db_exec(exec, INSERT_NICK, nick->nick, id, CurrentTime, CurrentTime);
  }

  if(exec != -1)
    db_exec(exec, SET_NICK_MASTER, nick->nick, id);

  if(exec != -1)
  {
    TransCommit();
    nick->id = id;
    return TRUE;
  }
  else
  {
    TransRollback();
    return FALSE;
  }
}

int 
db_forbid_nick(const char *nick)
{
  int ret;

  db_exec(ret, INSERT_FORBID, nick);

  if(ret == -1)
    return FALSE;

  return TRUE;
}

int 
db_is_forbid(const char *nick)
{
  yada_rc_t *rc, *brc;
  char *n;
  int ret;

  brc = Bind("?ps", &n);
  db_query(rc, GET_FORBID, nick);

  if(rc == NULL)
    return FALSE;

  ret = Fetch(rc, brc);
  
  Free(rc);
  Free(brc);

  return ret;
}

int 
db_delete_forbid(const char *nick)
{
  int ret;

  db_exec(ret, DELETE_FORBID, nick);

  if(ret == -1)
    return FALSE;

  return TRUE;
}

static char *
db_fix_link(unsigned int id)
{
  char *nick;
  yada_rc_t *rc, *brc;

  brc = Bind("?ps", &nick);
  db_query(rc, GET_NEW_LINK, id);

  if(rc == NULL)
    return NULL;

  if(Fetch(rc, brc) == 0)
  {
    Free(rc);
    Free(brc);
    return NULL;
  }

  Free(rc);
  Free(brc);

  return nick;
}

int
db_delete_nick(const char *nick)
{
  int ret;
  unsigned int id;

  /* TODO Do some(more?) stuff with constraints and transactions */
  TransBegin();

  id = db_get_id_from_name(nick, GET_NICKID_FROM_NICK);
  if(id <= 0)
  {
    TransRollback();
    return FALSE;
  }

  db_exec(ret, DELETE_NICK, nick);

  if(ret != -1)
  {
    char *newnick = db_fix_link(id);

    if(newnick == NULL)
    {
      db_exec(ret, DELETE_ACCOUNT, id);
    }
    else
    {
      db_exec(ret, SET_NICK_MASTER, newnick, id);
    }
  }

  if(ret == -1)
  {
    TransRollback();
    return FALSE;
  }

  TransCommit();
  execute_callback(on_nick_drop_cb, nick);
 
  return TRUE;
}

int
db_set_string(unsigned int key, unsigned int id, const char *value)
{
  int ret;

  db_exec(ret, key, value, id);

  if(ret == -1)
    return FALSE;

  return 1;
}

int
db_set_number(unsigned int key, unsigned int id, unsigned long value)
{
  int ret;

  db_exec(ret, key, value, id);

  if(ret == -1)
    return FALSE;

  return 1;
}

int
db_set_bool(unsigned int key, unsigned int id, unsigned char value)
{
  int ret;

  db_exec(ret, key, value, id);

  if(ret == -1)
    return FALSE;

  return 1;
}

int
db_list_add(unsigned int type, const void *value)
{
  struct AccessEntry *aeval = (struct AccessEntry *)value;
  struct ChanAccess *caval  = (struct ChanAccess *)value;
  struct ServiceBan *banval = (struct ServiceBan *)value;
  unsigned int id;
  int ret = 0;

  switch(type)
  {
    case ACCESS_LIST:
      db_exec(ret, INSERT_NICKACCESS, aeval->id, aeval->value);
      break;
    case AKILL_LIST:
      db_exec(ret, INSERT_AKILL, banval->mask, banval->reason, 
          banval->setter, banval->time_set, banval->duration);
      break;
    case AKICK_LIST:
      id = db_get_id_from_name(banval->channel, GET_CHANID_FROM_CHAN);
      if(banval->target != 0)
      {
        db_exec(ret, INSERT_AKICK_ACCOUNT, id, banval->target,
            banval->setter, banval->reason, banval->time_set, 
            banval->duration);
      }
      else if(banval->mask != NULL)
      {
        db_exec(ret, INSERT_AKICK_MASK, id, banval->setter, 
            banval->reason, banval->mask, banval->time_set, 
            banval->duration);
      }
      else
        assert(0 == 1);
      break;
    case CHACCESS_LIST:
      db_exec(ret, INSERT_CHANACCESS, caval->account, caval->channel,
          caval->level);
      break;
    default:
      assert(1 == 0);
      break;
  }
    
  if(ret == -1)
    return FALSE;
  else
    return TRUE;
}

void *
db_list_first(unsigned int type, unsigned int param, void **entry)
{
  yada_rc_t *rc, *brc;
  char *strval = (char*)*entry; 
  struct AccessEntry *aeval;
  struct ChanAccess *caval;
  struct ServiceBan *banval;
  struct DBResult *result;
  unsigned int query;
  
  switch(type)
  {
    case ACCESS_LIST:
      query = GET_NICKACCESS;
      aeval = (struct AccessEntry *)MyMalloc(sizeof(struct AccessEntry));
      *entry = aeval;
      brc = Bind("?d?ps", &aeval->id, &aeval->value);
      break;
    case ADMIN_LIST:
      query = GET_ADMINS;
      *entry = strval;
      brc = Bind("?ps", entry);
      break;
    case AKILL_LIST:
      query = GET_AKILLS;

      banval = (struct ServiceBan *)MyMalloc(sizeof(struct ServiceBan));
      *entry = banval;
      brc = Bind("?d?d?ps?ps?d?d", &banval->id, &banval->setter, &banval->mask, 
          &banval->reason, &banval->time_set, &banval->duration);
      break;
    case AKICK_LIST:
      query = GET_AKICKS;

      banval = (struct ServiceBan *)MyMalloc(sizeof(struct ServiceBan));
      *entry = banval;
      brc = Bind("?d?ps?d?d?ps?ps?d?d", &banval->id, &banval->channel,
          &banval->target, &banval->setter, &banval->mask, 
          &banval->reason, &banval->time_set, &banval->duration);
      break;
    case CHACCESS_LIST:
      query = GET_CHAN_ACCESSES;

      caval = (struct ChanAccess *)MyMalloc(sizeof(struct ChanAccess));
      *entry = caval;
      brc = Bind("?d?d?d?d", &caval->id, &caval->channel, &caval->account,
          &caval->level);
      break;
  }

  db_query(rc, query, param);

  if(rc == NULL || brc == NULL)
    return NULL;

  if(Fetch(rc, brc) == 0)
    return NULL;

  result = (struct DBResult *)MyMalloc(sizeof(struct DBResult));

  result->rc = rc;
  result->brc = brc;
  
  if(type == AKILL_LIST)
  {
    DupString(banval->mask, banval->mask);
    DupString(banval->reason, banval->reason);
    banval->type = AKILL_BAN;
  }
  else if(type == AKICK_LIST)
  {
    DupString(banval->mask, banval->mask);
    DupString(banval->reason, banval->reason);
    DupString(banval->channel, banval->channel);
    banval->type = AKICK_BAN;
  }

  return (void*)result;
}

void *
db_list_next(void *result, unsigned int type, void **entry)
{
  struct DBResult *res = (struct DBResult *)result;
  struct AccessEntry *aeval;
  struct ChanAccess *caval;
  struct ServiceBan *banval;
  char *strval = (char*)*entry; 
 
  switch(type)
  {
    case ACCESS_LIST:
      aeval = (struct AccessEntry *)MyMalloc(sizeof(struct AccessEntry));
      *entry = aeval;
      Free(res->brc);
      res->brc = Bind("?d?ps", &aeval->id, &aeval->value);
      break;
    case ADMIN_LIST:
      *entry = strval;
      Free(res->brc);
      res->brc = Bind("?ps", entry);
      break;
    case AKILL_LIST:
      banval = (struct ServiceBan *)MyMalloc(sizeof(struct ServiceBan));
      *entry = banval;
      Free(res->brc);
      res->brc = Bind("?d?d?ps?ps?d?d", &banval->id, &banval->setter, &banval->mask, 
          &banval->reason, &banval->time_set, &banval->duration);
     break;
    case AKICK_LIST:
      banval = (struct ServiceBan *)MyMalloc(sizeof(struct ServiceBan));
      *entry = banval;
      Free(res->brc);
      res->brc = Bind("?d?ps?d?d?ps?ps?d?d", &banval->id, &banval->channel,
          &banval->target, &banval->setter, &banval->mask, 
          &banval->reason, &banval->time_set, &banval->duration);
   break;
    case CHACCESS_LIST:
      caval = (struct ChanAccess *)MyMalloc(sizeof(struct ChanAccess));
      *entry = caval;
      Free(res->brc);
      res->brc = Bind("?d?d?d?d", &caval->id, &caval->channel, &caval->account,
          &caval->level);
      break;
    default:
      assert(0 == 1);
  }

  if(Fetch(res->rc, res->brc) == 0)
    return NULL;

  if(type == AKILL_LIST)
  {
    banval->type = AKILL_BAN;
    DupString(banval->mask, banval->mask);
    DupString(banval->reason, banval->reason);
  }
  else if(type == AKICK_LIST)
  {
    banval->type = AKICK_BAN;
    DupString(banval->mask, banval->mask);
    DupString(banval->reason, banval->reason);
    DupString(banval->channel, banval->channel);
  }

  return result;
}

void
db_list_done(void *result)
{
  struct DBResult *res = (struct DBResult *)result;

  Free(res->brc);
  Free(res->rc);
}

int
db_list_del(unsigned int type, unsigned int id, const char *param)
{
  int ret;

  if(id > 0)
  {
    db_exec(ret, type, id, param);
  }
  else
  {
    db_exec(ret, type, param);
  }

  if(ret == -1)
    return 0;

  return ret;
}

int 
db_list_del_index(unsigned int type, unsigned int id, unsigned int index)
{
  int ret;

  db_exec(ret, type, index, id, id);

  if(ret == -1)
    return 0;

  return ret;
}

int    
db_link_nicks(unsigned int master, unsigned int child)
{
  int ret;

  TransBegin();

  db_exec(ret, SET_NICK_LINK, master, child);
  if(ret != -1)
    db_exec(ret, DELETE_ACCOUNT, child);

  if(ret == -1)
  {
    TransRollback();
    return FALSE;
  }

  TransCommit();
  return TRUE;
}

unsigned int 
db_unlink_nick(unsigned int id)
{
  int ret;
  unsigned int newid;

  TransBegin();

  db_exec(ret, INSERT_NICK_CLONE, id);
  if(ret != -1)
  {
    newid = InsertID("account", "id");
    db_exec(ret, SET_NICK_LINK, id, newid);
  }

  if(ret == -1)
  {
    TransRollback();
    return 0;
  }
  
  TransCommit();
  return newid;
}

struct RegChannel *
db_find_chan(const char *channel)
{
  yada_rc_t *rc, *brc;
  struct RegChannel *channel_p;
  char *retchan;

  assert(channel != NULL);

  db_query(rc, GET_FULL_CHAN, channel);

  if(rc == NULL)
    return NULL;
 
  channel_p = (struct RegChannel *)MyMalloc(sizeof(struct RegChannel));
 
  brc = Bind("?d?ps?ps?ps?B?B?B?B?B?B?B?ps?ps?ps?ps",
      &channel_p->id, &retchan, &channel_p->description, &channel_p->entrymsg, 
      &channel_p->forbidden, &channel_p->priv, &channel_p->restricted,
      &channel_p->topic_lock, &channel_p->verbose, &channel_p->autolimit,
      &channel_p->expirebans, &channel_p->url, &channel_p->email, 
      &channel_p->topic, &channel_p->mlock); 

  if(Fetch(rc, brc) == 0)
  {
    printf("db_find_chan: '%s' not found.\n", channel);
    Free(brc);
    Free(rc);
    return NULL;
  }

  strlcpy(channel_p->channel, retchan, sizeof(channel_p->channel));

  DupString(channel_p->description, channel_p->description);
  DupString(channel_p->entrymsg, channel_p->entrymsg);
  DupString(channel_p->url, channel_p->url);
  DupString(channel_p->email, channel_p->email);
  DupString(channel_p->topic, channel_p->topic);
  DupString(channel_p->mlock, channel_p->mlock);

  printf("db_find_chan: Found nick %s\n", channel_p->channel);

  Free(brc);
  Free(rc);

  return channel_p;
}

int
db_register_chan(struct RegChannel *chan, unsigned int founder)
{
  struct ChanAccess *access;
  int ret;

  TransBegin();

  db_exec(ret, INSERT_CHAN, chan->channel, chan->description);

  if(ret == -1)
  {
    TransRollback();
    return FALSE;
  }

  chan->id = InsertID("channel", "id");
  if(chan->id <= 0)
  {
    TransRollback();
    return FALSE;
  }

  access = (struct ChanAccess *)MyMalloc(sizeof(struct ChanAccess));
  access->channel = chan->id;
  access->account = founder;
  access->level   = MASTER_FLAG;
 
  if(!db_list_add(CHACCESS_LIST, access))
  {
    TransRollback();
    return FALSE;
  }

  TransCommit();
  return TRUE;
}

int
db_delete_chan(const char *chan)
{
  int ret;

  db_exec(ret, DELETE_CHAN, chan);

  if(ret == -1)
    return FALSE;
  return TRUE;
}

struct ChanAccess *
db_find_chanaccess(unsigned int channel, unsigned int account)
{
  yada_rc_t *rc, *brc;
  struct ChanAccess *access = NULL;
  
  db_query(rc, GET_CHAN_ACCESS, channel, account);
  
  if(rc == NULL)
    return NULL;

  access = (struct ChanAccess *)MyMalloc(sizeof(struct ChanAccess));
  brc = Bind("?d?d?d?d", &access->id, &access->channel, &access->account, 
      &access->level);

  if(Fetch(rc, brc) == 0)
    access = NULL;

  Free(rc);
  Free(brc);

  return access;
}

struct ServiceBan *
db_find_akill(const char *mask)
{
  yada_rc_t *rc, *brc;
  struct ServiceBan *akill;

  assert(mask != NULL);

  db_query(rc, GET_AKILL, mask);

  if(rc == NULL)
    return NULL;
 
  akill = (struct ServiceBan *)MyMalloc(sizeof(struct ServiceBan));
 
  brc = Bind("?d?ps?ps?d?d?d", &akill->id, &akill->mask, &akill->reason,
      &akill->setter, &akill->time_set, &akill->duration);

  if(Fetch(rc, brc) == 0)
  {
    printf("db_find_akill: '%s' not found.\n", mask);
    Free(brc);
    Free(rc);
    return NULL;
  }

  DupString(akill->mask, akill->mask);
  DupString(akill->reason, akill->reason);

  printf("db_find_akill: Found akill %s(asked for %s)\n", akill->mask, mask);

  Free(brc);
  Free(rc);

  return akill;
}


int
db_get_num_masters(unsigned int chanid)
{
  yada_rc_t *rc, *brc;
  int count;

  db_query(rc, GET_CHAN_MASTER_COUNT, chanid, MASTER_FLAG);

  if(rc == NULL)
    return 0;

  brc = Bind("?d", &count);

  if(Fetch(rc, brc) == 0)
    count = 0;

  Free(brc);
  Free(rc);

  return count;
}
