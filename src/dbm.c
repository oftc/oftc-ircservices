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

#include "stdinc.h"
#include "conf/conf.h"
#include <yada.h>

struct Callback *on_nick_drop_cb;

query_t queries[QUERY_COUNT] = { 
  { "SELECT id, nick, password, salt, url, email, cloak, flag_enforce, "
    "flag_secure, flag_verified, flag_forbidden, flag_cloak_enabled, "
    "flag_admin, flag_email_verified, language, last_host, last_realname, "
    "last_quit_msg, last_quit_time, account.reg_time, nickname.reg_time, "
    "last_seen FROM account, nickname WHERE account.id = nickname.user_id AND "
    "lower(nick) = lower(?v)", NULL, QUERY },
  { "SELECT nick from nickname WHERE user_id=?d", NULL, QUERY },
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
  { "SELECT nick FROM account, nickname WHERE account.id = nickname.user_id AND "
    "flag_admin=true", NULL, QUERY },
  { "SELECT akill.id, nick, mask, reason, time, duration FROM nickname, akill "
    "WHERE setter = nickname.user_id", NULL, QUERY },
  { "SELECT id, channel_id, nick_id, level FROM channel_access WHERE "
    "channel_id=?d", NULL, QUERY },
  { "SELECT id from channel WHERE lower(channel)=lower(?v)", NULL, QUERY },
  { "SELECT id, channel, description, entrymsg, flags, url, email, topic, "
    "founder, successor FROM channel WHERE lower(channel)=lower(?v)", 
    NULL, QUERY },
  { "INSERT INTO channel (channel, founder) VALUES(?d, "
          "(SELECT id FROM nickname WHERE lower(nick)=lower(?v)))", 
          NULL, EXECUTE },
  { "INSERT INTO channel_access (nick_id, channel_id, level) VALUES (?d, ?d, ?d)", 
    NULL, EXECUTE } ,
  { "UPDATE channel_access SET level=?d WHERE id=?d", NULL, EXECUTE },
  { "DELETE FROM channel_access WHERE channel_id=?d AND nick_id=?d", 
    NULL, EXECUTE },
  { "SELECT id, channel_id, nick_id, level FROM channel_access WHERE "
    "channel_id=?d AND nick_id=?d", NULL, QUERY },
  { "DELETE FROM channel WHERE lower(channel)=lower(?v)", NULL, EXECUTE },
  { "UPDATE channel SET founder="
    "(SELECT id FROM nickname WHERE lower(nick)=lower(?s)) WHERE "
    "lower(channel)=lower(?s)", NULL, EXECUTE },
  { "UPDATE channel SET founder=successor, successor=0 WHERE successor=?d", 
    NULL, EXECUTE },
  { "UPDATE channel SET successor=(SELECT id FROM nickname WHERE "
    "lower(nick)=lower(?v) WHERE channel=?v", NULL, EXECUTE },
  { "SELECT id, mask, reason, setter, time, duration FROM akill WHERE ?v "
    "ILIKE mask", NULL, QUERY },
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
  { "UPDATE account SET flag_cloak_enabled=?s WHERE id=?d", NULL, EXECUTE },
  { "UPDATE account SET flag_secure=?s WHERE id=?d", NULL, EXECUTE },
  { "UPDATE account SET flag_enforce=?s WHERE id=?d", NULL, EXECUTE },
  { "UPDATE account SET flag_forbidden=?s WHERE id=?d", NULL, EXECUTE },
  { "DELETE FROM account_access WHERE parent_id=?d AND entry=?v", NULL,
    EXECUTE },
  { "DELETE FROM account_access WHERE parent_id=?d", NULL, EXECUTE },
  { "DELETE FROM account_access WHERE id = "
          "(SELECT id FROM account_access AS a WHERE ?d = "
          "(SELECT COUNT(id)+1 FROM account_access AS b WHERE b.id < a.id AND "
          "b.parent_id = ?d) AND parent_id = ?d)", NULL, EXECUTE },
  { "UPDATE nickname SET user_id=?d WHERE user_id=?d", NULL, EXECUTE },
  { "INSERT INTO account (password, salt, url, email, cloak, flag_enforce, "
    "flag_secure, flag_verified, flag_forbidden, flag_cloak_enabled, "
    "flag_admin, flag_email_verified, language, last_host, last_realname, "
    "last_quit_msg, last_quit_time, reg_time) SELECT password, salt, url, "
    "email, cloak, flag_enforce, flag_secure, flag_verified, flag_forbidden, "
    "flag_cloak_enabled, flag_admin, flag_email_verified, language, last_host, "
    "last_realname, last_quit_msg, last_quit_time, reg_time FROM account "
    "WHERE id=?d", NULL, EXECUTE },
  { "SELECT count(*) FROM nickname WHERE user_id=?d", NULL, QUERY},
};

void
init_db()
{
  char *dbstr;

  dbstr = MyMalloc(strlen(Database.driver) + strlen(Database.hostname) + strlen(Database.dbname) +
      3 /* ::: */ + 1);
  sprintf(dbstr, "%s:::%s", Database.driver, Database.dbname);

  Database.yada = yada_init(dbstr, 0);
  MyFree(dbstr);
  on_nick_drop_cb = register_callback("Nick DROP Callback", NULL);
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
 
  nick_p = MyMalloc(sizeof(struct Nick));
 
  brc = Bind("?d?ps?ps?ps?ps?ps?ps?B?B?B?B?B?B?B?d?ps?ps?ps?d?d?d?d",
    &nick_p->id, &retnick, &retpass, &retsalt, &nick_p->url, &nick_p->email,
    &retcloak, &nick_p->enforce, &nick_p->secure, &nick_p->verified, 
    &nick_p->forbidden, &nick_p->cloak_on, &nick_p->admin, 
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

  if(nick_p->url != NULL)
    DupString(nick_p->url, nick_p->url);
  if(nick_p->email != NULL)
    DupString(nick_p->email, nick_p->email);
  if(nick_p->last_host != NULL)
    DupString(nick_p->last_host, nick_p->last_host);
  if(nick_p->last_realname != NULL)
    DupString(nick_p->last_realname, nick_p->last_realname);
  if(nick_p->last_quit != NULL)
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
db_get_id_from_nick(const char *nick)
{
  yada_rc_t *rc, *brc;
  int ret;

  db_query(rc, GET_NICKID_FROM_NICK, nick);

  if(rc == NULL)
    return 0;

  brc = Bind("?d", &ret);
  if(Fetch(rc, brc) == 0)
  {
    printf("db_get_id_from_nick: '%s' not found.\n", nick);
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

static int
db_get_num_nicks(unsigned int id)
{
  int count;
  yada_rc_t *rc, *brc;

  brc = Bind("?d", &count);
  db_query(rc, GET_NICK_COUNT, id);

  if(rc == NULL)
    return 0;

  if(Fetch(rc, brc) == 0)
    return 0;

  return count;
}

int
db_delete_nick(const char *nick)
{
  int ret;
  unsigned int id;

  /* TODO Do some stuff with constraints and transactions */
  TransBegin();

  id = db_get_id_from_nick(nick);
  if(id <= 0)
  {
    TransRollback();
    return FALSE;
  }

  db_exec(ret, DELETE_NICK, nick);

  if(ret != -1 && db_get_num_nicks(id) == 0)
    db_exec(ret, DELETE_ACCOUNT, id);

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

  db_exec(ret, key, value ? "true": "false", id);

  if(ret == -1)
    return FALSE;

  return 1;
}

int
db_list_add(unsigned int type, const void *value)
{
  int ret = 0;

  switch(type)
  {
    case ACCESS_LIST:
    {
      struct AccessEntry *entry = (struct AccessEntry *)value;
      
      db_exec(ret, INSERT_NICKACCESS, entry->id, entry->value);
      break;
    }
    case AKILL_LIST:
    {
      struct AKill *akill = (struct AKill *)value;

      db_exec(ret, INSERT_AKILL, akill->mask, akill->reason, 
          db_get_id_from_nick(akill->setter), akill->time_set, 
          akill->duration);
      break;
    }
    case CHACCESS_LIST:
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
  unsigned int query;
  struct AccessEntry *aeval;
  struct AKill *akillval;
  char *strval = (char*)*entry; 
  struct DBResult *result;
  
  switch(type)
  {
    case ACCESS_LIST:
      query = GET_NICKACCESS;
      aeval = MyMalloc(sizeof(struct AccessEntry));
      *entry = aeval;
      brc = Bind("?d?ps", &aeval->id, &aeval->value);
      break;
    case ADMIN_LIST:
      query = GET_ADMINS;

      strval = MyMalloc(255+1); /* XXX constant? */
      *entry = strval;
      brc = Bind("?s", strval);
      break;
    case AKILL_LIST:
      query = GET_AKILLS;

      akillval = MyMalloc(sizeof(struct AKill));
      *entry = akillval;
      brc = Bind("?d?ps?ps?ps?d?d?d", &akillval->id, &akillval->mask, 
          &akillval->reason, &akillval->setter, &akillval->time_set, 
          &akillval->duration);
      break;
    case CHACCESS_LIST:
      query = GET_CHAN_ACCESS;
      break;
  }

  db_query(rc, query, param);

  if(rc == NULL || brc == NULL)
    return NULL;

  if(Fetch(rc, brc) == 0)
    return NULL;

  result = MyMalloc(sizeof(struct DBResult));

  result->rc = rc;
  result->brc = brc;
  
  if(type == AKILL_LIST)
  {
    DupString(akillval->mask, akillval->mask);
    DupString(akillval->reason, akillval->reason);
    DupString(akillval->setter, akillval->setter);
  }

  return (void*)result;
}

void *
db_list_next(void *result, unsigned int type, void **entry)
{
  struct DBResult *res = (struct DBResult *)result;
  struct AccessEntry *aeval;
  struct AKill *akillval;
  char *strval = (char*)*entry; 
 
  switch(type)
  {
    case ACCESS_LIST:
      aeval = MyMalloc(sizeof(struct AccessEntry));
      *entry = aeval;
      break;
    case ADMIN_LIST:
      strval = MyMalloc(255+1); /* XXX constant? */
      *entry = strval;
      break;
    case AKILL_LIST:
      akillval = MyMalloc(sizeof(struct AKill));
      *entry = akillval;
     break;
    case CHACCESS_LIST:
      break;
    default:
      assert(0 == 1);
  }

  if(Fetch(res->rc, res->brc) == 0)
    return NULL;

  if(type == AKILL_LIST)
  {
    DupString(akillval->mask, akillval->mask);
    DupString(akillval->reason, akillval->reason);
    DupString(akillval->setter, akillval->setter);
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

  if(type == DELETE_NICKACCESS)
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

#if 0
unsigned int
db_get_id_from_chan(const char *chan)
{
  yada_rc_t * result;
  char *escchan;
  int id;

  if(dbi_driver_quote_string_copy(Database.driv, chan, &escchan) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    return 0;
  }

  MyFree(escchan);
  
  if(result == NULL)
    return 0;

  if(yada_rc_t *_get_numrows(result) == 0)
  {
    printf("db: WTF. Didn't find channel entry for %s\n", chan);
    yada_rc_t *_free(result);
    return 0;
  }

  yada_rc_t *_first_row(result);
  yada_rc_t *_get_fields(result, "id.%ui", &id);
  yada_rc_t *_free(result);

  return id;
}

struct RegChannel *
db_find_chan(const char *channel)
{
  yada_rc_t * result;
  char *escchannel = NULL;
  char *findchannel;
  struct RegChannel *channel_p;
  
  assert(channel != NULL);

  if(dbi_driver_quote_string_copy(Database.driv, channel, &escchannel) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    return NULL;
  }

  MyFree(escchannel);
  
  if(result == NULL)
    return NULL;
  
  if(yada_rc_t *_get_numrows(result) == 0)
  {
    printf("db: Channel %s not found\n", channel);
    yada_rc_t *_free(result);
    return NULL;
  }

  channel_p = MyMalloc(sizeof(struct RegChannel));
  yada_rc_t *_first_row(result);
  yada_rc_t *_get_fields(result, 
      "id.%ui channel.%S description.%S entrymsg.%S flags.%ui "
      "url.%S email.%S topic.%S founder.%ui successor.%ui",
      &channel_p->id, &findchannel, &channel_p->description, 
      &channel_p->entrymsg, &channel_p->flags, &channel_p->url,
      &channel_p->email, &channel_p->topic, &channel_p->founder, 
      &channel_p->successor);

  strlcpy(channel_p->channel, findchannel, sizeof(channel_p->channel));

  MyFree(findchannel);

  yada_rc_t *_free(result);

  return channel_p;
}

int
db_register_chan(struct Client *client, char *channelname)
{
  char *escchannel = NULL;
  char *escfounder = NULL;
  yada_rc_t * result;
  
  if(dbi_driver_quote_string_copy(Database.driv, channelname, &escchannel) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    MyFree(escchannel);
    return -1;
  }

  if(dbi_driver_quote_string_copy(Database.driv, client->name, &escfounder) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    MyFree(escchannel);
    MyFree(escfounder);
    return -1;
  }
  
  MyFree(escchannel);
  MyFree(escfounder);
 
  if(result == NULL)
    return 0;

  yada_rc_t *_free(result);

  return 0;
}

int
db_chan_access_add(struct ChannelAccessEntry *accessptr)
{
  yada_rc_t * result;

  if (accessptr->id == 0)
  {
  }
  else
  {
  }

  if(result == NULL)
    return -1;

  yada_rc_t *_free(result);
  return 0;
}

int
db_chan_access_del(struct RegChannel *regchptr, int nickid)
{
  yada_rc_t * result;
  
  if(result == NULL)
    return -1;

  yada_rc_t *_free(result);
  return 0;
}

struct ChannelAccessEntry *
db_chan_access_get(int channel_id, int nickid)
{
  yada_rc_t * result;
  struct ChannelAccessEntry *cae;
  
  result = db_query("",
    "channel_access", channel_id, nickid);
  
  if(result == NULL)
    return NULL;

  if(yada_rc_t *_get_numrows(result) == 0)
  {
    yada_rc_t *_free(result);
    return NULL;
  }

  cae = MyMalloc(sizeof(struct ChannelAccessEntry));
  yada_rc_t *_first_row(result);
  yada_rc_t *_get_fields(result, "id.%i channel_id.%i nick_id.%i level.%i", 
    &cae->id, &cae->channel_id, &cae->nick_id, &cae->level);

  yada_rc_t *_free(result);
  return cae;
}

int
db_delete_chan(const char *chan)
{
  yada_rc_t * result;
  char *escchan;

  if(dbi_driver_quote_string_copy(Database.driv, chan, &escchan) == 0)
  {
    printf("db: Failed to delete channel: dbi_driver_quote_string_copy\n");
    return -1;
  }

  {
    MyFree(escchan);
    return 0;
  }  
  
  MyFree(escchan);
  yada_rc_t *_free(result);

  return 0;
}

int
db_set_founder(const char *channel, const char *nickname)
{
  yada_rc_t * result;

  char *escchannel, *escnick;

  if(dbi_driver_quote_string_copy(Database.driv, nickname, &escnick) == 0)
  {
    printf("db: Failed to delete channel: dbi_driver_quote_string_copy\n");
    return -1;
  }

  if(dbi_driver_quote_string_copy(Database.driv, channel, &escchannel) == 0)
  {
    printf("db: Failed to delete channel: dbi_driver_quote_string_copy\n");
    MyFree(escnick);
    return -1;
  }

  if (result != NULL)
    yada_rc_t *_free(result);

  MyFree(escnick);
  MyFree(escchannel);

  return 0;
}

int
db_chan_success_founder(const char *nickname)
{
  yada_rc_t * result;

  int successor_id;

  successor_id = db_get_id_from_nick(nickname);


  result = db_query("", "channel", successor_id);
  if (result != NULL)
    yada_rc_t *_free(result);

  return 0;
}

int
db_set_successor(const char *channel, const char *nickname)
{
  yada_rc_t * result;

  char *escchannel, *escnick;

  if(dbi_driver_quote_string_copy(Database.driv, nickname, &escnick) == 0)
  {
    printf("db: Failed to delete channel: dbi_driver_quote_string_copy\n");
    return -1;
  }

  if(dbi_driver_quote_string_copy(Database.driv, channel, &escchannel) == 0)
  {
    printf("db: Failed to delete channel: dbi_driver_quote_string_copy\n");
    MyFree(escnick);
    return -1;
  }

  if (result != NULL)
    yada_rc_t *_free(result);

  MyFree(escnick);
  MyFree(escchannel);

  return 0;
}

struct AKill *
db_find_akill(const char *userhost)
{
  yada_rc_t * result;
  char *escuserhost = NULL;
  struct AKill *akill;
  unsigned int id;
  
  assert(userhost != NULL);

  if(dbi_driver_quote_string_copy(Database.driv, userhost, &escuserhost) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    return NULL;
  }

  MyFree(escuserhost);
  
  if(result == NULL)
    return NULL;
  
  if(yada_rc_t *_get_numrows(result) == 0)
  {
    yada_rc_t *_free(result);
    return NULL;
  }

  akill = MyMalloc(sizeof(struct AKill));
  yada_rc_t *_first_row(result);
  yada_rc_t *_get_fields(result, "id.%ui mask.%S reason.%S setter.%ui time.%ui "
      "duration.%ui", &akill->id, &akill->mask, &akill->reason, &id,
      &akill->time_set, &akill->duration);

  akill->setter = db_find_nick(db_get_nickname_from_id(id));

  yada_rc_t *_free(result);
  return akill;
}

struct AKill *
db_add_akill(struct AKill *akill)
{
  char *escmask = NULL;
  char *escreason = NULL;
  char *mask;
  yada_rc_t * result;
  
  DupString(mask, akill->mask);
  
  if(dbi_driver_quote_string_copy(Database.driv, akill->mask, &escmask) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    return NULL;
  }

  if(dbi_driver_quote_string_copy(Database.driv, akill->reason, &escreason) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    MyFree(escmask);
    return NULL;
  }

  MyFree(escmask);
  MyFree(escreason);
 
  if(result == NULL)
    return NULL;
  
  yada_rc_t *_free(result);

  free_akill(akill);

  akill = db_find_akill(mask);
  MyFree(mask);
  return akill; 
}
#endif
