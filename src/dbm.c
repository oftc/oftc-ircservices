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
  { "INSERT INTO nickname_access (parent_id, entry) VALUES(?d, ?v)", 
    NULL, EXECUTE },
  { "SELECT id, entry FROM nickname_access WHERE parent_id=?d", NULL, QUERY },
  { "SELECT id, entry FROM nickname_access", NULL, QUERY },
  { "SELECT id, nick FROM nickname WHERE (flags & ?d > 0)", NULL, QUERY },
  { "SELECT id, mask, reason, setter, time, duration FROM akill", NULL, QUERY },
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
      "VALUES(?v, ?v, ?d, ?d, ?d)", NULL, EXECUTE }
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
  struct Nick *nick;

  if(Database.yada->connect(Database.yada, Database.username, 
        Database.password) == 0)
  {
    printf("db: Failed to connect to database %s\n", Database.yada->errmsg);
  }
  else
    printf("db: Database connection succeeded.\n");

  for(i = 0; i < QUERY_COUNT; i++)
  {
    query_t *query = &queries[i];
    query->rc = Database.yada->prepare(Database.yada, (char*)query->name, 0);
  }

  nick = db_find_nick("test");
  db_register_nick("test", "moo", "moomoo", "moo@foo.org");
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
  __result = Database.yada->query(Database.yada, __query->rc, args);  \
  if(__result == NULL)                                                \
    printf("db_query: %d Failed: %s\n", __id, Database.yada->errmsg); \
                                                                      \
  ret = __result;                                                     \
};

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
  __result = Database.yada->execute(Database.yada, __query->rc, args);\
  if(__result == -1)                                                  \
    printf("db_exec: %d Failed: %s\n", __id, Database.yada->errmsg);  \
                                                                      \
  ret = __result;                                                     \
};

struct Nick *
db_find_nick(const char *nick)
{
  yada_rc_t *result, *brc;
  struct Nick *nick_p;
  char *retnick, *retpass, *retcloak, *retsalt;
  unsigned int id;

  assert(nick != NULL);

  db_query(result, GET_FULL_NICK, nick);

  if(result == NULL)
    return NULL;
 
  nick_p = MyMalloc(sizeof(struct Nick));
 
  brc = Database.yada->bind(Database.yada, 
      "?d?ps?ps?ps?ps?ps?ps?d?d?d?d?d?d?d?d?ps?ps?ps?d?d?d?d",
    &nick_p->id, &retnick, &retpass, &retsalt, &nick_p->url, &nick_p->email,
    &retcloak, &nick_p->enforce, &nick_p->secure, &nick_p->verified, 
    &nick_p->forbidden, &nick_p->cloak_on, &nick_p->admin, 
    &nick_p->email_verified, &nick_p->language, &nick_p->last_host,
    &nick_p->last_realname, &nick_p->last_quit,
    &nick_p->last_quit_time, &nick_p->reg_time, &nick_p->nick_reg_time,
    &nick_p->last_seen);

  if(Database.yada->fetch(Database.yada, result, brc) == 0)
  {
    printf("db_find_nick: '%s' not found.\n", nick);
    return NULL;
  }

  strlcpy(nick_p->nick, retnick, sizeof(nick_p->nick));
  strlcpy(nick_p->pass, retpass, sizeof(nick_p->pass));
  strlcpy(nick_p->salt, retsalt, sizeof(nick_p->salt));
  if(retcloak)
    strlcpy(nick_p->cloak, retcloak, sizeof(nick_p->cloak));

  printf("db_find_nick: Found nick %s(asked for %s)\n", nick_p->nick, nick);

  return nick_p;
}

#if 0
char *    
db_get_nickname_from_id(unsigned int id)    
{   
  yada_rc_t * result;    
  char *retnick;    

  if(result == NULL)    
    return NULL;    

  if(yada_rc_t *_get_numrows(result) == 0)   
  {   
    yada_rc_t *_free(result);    
    return NULL;    
  }   

  yada_rc_t *_first_row(result);   
  yada_rc_t *_get_fields(result, "nick.%S", &retnick);   
  yada_rc_t *_free(result);    

  return retnick;   
}

unsigned int
db_get_id_from_nick(const char *nick)
{
  yada_rc_t * result;
  char *escnick;
  int id;

  if(dbi_driver_quote_string_copy(Database.driv, nick, &escnick) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    return 0;
  }

  MyFree(escnick);
  
  if(result == NULL)
    return 0;

  if(yada_rc_t *_get_numrows(result) == 0)
  {
    printf("db: WTF. Didn't find nickname entry for %s\n", nick);
    yada_rc_t *_free(result);
    return 0;
  }

  yada_rc_t *_first_row(result);
  yada_rc_t *_get_fields(result, "id.%ui", &id);
  yada_rc_t *_free(result);

  return id;
}

#endif

struct Nick *
db_register_nick(const char *nick, const char *password, const char *salt,
    const char *email)
{
  int exec, id;

  assert(nick != NULL);

  TRANS_BEGIN;

  db_exec(exec, INSERT_ACCOUNT, password, salt, email, CurrentTime);
  if(exec != -1)
  {
    id = Database.yada->insert_id(Database.yada, "account", "id");
    db_exec(exec, INSERT_NICK, nick, id, CurrentTime, CurrentTime);
  }

  if(exec != -1)
  {
    TRANS_COMMIT;
  }
  else
  {
    TRANS_ROLLBACK;
    return NULL;
  }
}

#if 0

int
db_delete_nick(const char *nick)
{
  yada_rc_t * result;
  char *escnick;

  if(dbi_driver_quote_string_copy(Database.driv, nick, &escnick) == 0)
  {
    printf("db: Failed to delete nick: dbi_driver_quote_string_copy\n");
    return -1;
  }

  execute_callback(on_nick_drop_cb, nick);
 
  MyFree(escnick);
  
  if(result == NULL)
    return 0;
 
  yada_rc_t *_free(result);

  return 0;
}

int
db_set_string(const char *table, unsigned int id, const char *key, 
    const char *value)
{
  yada_rc_t * result;
  char *escvalue;

  if(dbi_driver_quote_string_copy(Database.driv, value, &escvalue) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    return -1;
  }
  
  MyFree(escvalue);

  if(result == NULL)
    return -1;

  yada_rc_t *_free(result);

  return 0;
}

int
db_set_number(const char *table, unsigned int id, const char *key, 
    const unsigned long value)
{
  yada_rc_t * result;

  if(result == NULL)
    return -1;

  yada_rc_t *_free(result);

  return 0;
}

int
db_list_add(const char *table, unsigned int id, const char *value)
{
  char *escvalue;
  yada_rc_t * result;
  
  if(dbi_driver_quote_string_copy(Database.driv, value, &escvalue) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    return -1;
  }
  
  MyFree(escvalue);
  
  if(result == NULL)
    return -1;

  yada_rc_t *_free(result);
  
  return 0;
}

void *
db_list_first(const char *table, unsigned int type, unsigned int param, 
    void **entry)
{
  yada_rc_t * result;
  char querybuf[1024+1];
  
  switch(type)
  {
    case ACCESS_LIST:
      if(param > 0)
        ;
      else
        ;
      break;
    case NICK_FLAG_LIST:
        ;
      break;
    case AKILL_LIST:
      ;
      break;
    case CHACCESS_LIST:
      ;
      break;
  }
  
  result = db_query("%s", querybuf);

  if(result == NULL)
    return NULL;

  if(yada_rc_t *_get_numrows(result) == 0)
    return NULL;

  if(yada_rc_t *_first_row(result))
  {
    unsigned int id;
    
    switch(type)
    {
      case ACCESS_LIST:
      {
        struct AccessEntry *retentry = MyMalloc(sizeof(struct AccessEntry));
        
        yada_rc_t *_get_fields(result, "id.%ui entry.%S", &retentry->id, 
            &retentry->value);
        *entry = (void*)retentry;
        break;
      }
      case NICK_FLAG_LIST:
      {
        char *retnick;
        struct Nick *nick;
        
        yada_rc_t *_get_fields(result, "id.%ui nick.%S", &id, &retnick);
        /* Possibly a little inefficient, but ensures we maintain
         * links */
        nick = db_find_nick(retnick);
        *entry = (void *)nick;
        MyFree(retnick);
        break;
      }
      case AKILL_LIST:
      {
        struct AKill *akill;

        akill = MyMalloc(sizeof(struct AKill));

        yada_rc_t *_get_fields(result, "id.%ui mask.%S reason.%S setter.%ui "
            "time.%ui duration.%ui", &akill->id, &akill->mask, &akill->reason,
            &id, &akill->time_set, &akill->duration);
        akill->setter = db_find_nick(db_get_nickname_from_id(id));
        *entry = (void*)akill;
        break;
      }
      case CHACCESS_LIST:
      {
        struct ChannelAccessEntry *cae;
        
        cae = MyMalloc(sizeof(struct ChannelAccessEntry));
        
        yada_rc_t *_get_fields(result, "id.%ui channel_id.%ui nick_id.%ui level.%ui",
          &cae->id, &cae->channel_id, &cae->nick_id, &cae->level);
        *entry = (void *)cae;
        break;
      }
    }
    return result;
  }

  return NULL;
}

void *
db_list_next(void *result, unsigned int type, void **entry)
{
  if(yada_rc_t *_next_row(result))
  {
    switch(type)
    {
      case ACCESS_LIST:
      {
        struct AccessEntry *retentry = MyMalloc(sizeof(struct AccessEntry));
        yada_rc_t *_get_fields(result, "id.%ui entry.%S", &retentry->id, 
            &retentry->value);
        *entry = (void*)retentry;
        break;
      }
      case NICK_FLAG_LIST:
      {
        struct Nick *nick;
        unsigned int id;
        char *retnick;

        yada_rc_t *_get_fields(result, "id.%ui nick.%S", &id, &retnick);
        nick = db_find_nick(retnick);
        MyFree(retnick);
        *entry = (void*)nick;
        break;
      }
      case AKILL_LIST:
        break;
      case CHACCESS_LIST:
      {
        struct ChannelAccessEntry *cae;

        cae = MyMalloc(sizeof(struct ChannelAccessEntry));

        yada_rc_t *_get_fields(result, "id.%ui channel_id.%ui nick_id.%ui level.%ui",
          &cae->id, &cae->channel_id, &cae->nick_id, &cae->level);
        *entry = (void *)cae;
        break;
      }
    }
    return result;
  }
  return NULL;
}

void
db_list_done(void *result)
{
  yada_rc_t *_free(result);
}

int
db_list_del(const char *table, unsigned int id, const char *entry)
{
  yada_rc_t * result;
  char *escentry;
  int numrows;
    
  if(dbi_driver_quote_string_copy(Database.driv, entry, &escentry) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    return 0;
  }

  if((result = db_query("DELETE FROM %s WHERE parent_id=%d AND entry=%s", 
          table, id, escentry)) == NULL)
  {
    MyFree(escentry);
    return 0;
  }
    
  MyFree(escentry);

  numrows = yada_rc_t *_get_numrows_affected(result);

  yada_rc_t *_free(result);
  return numrows;
}

int 
db_list_del_index(const char *table, unsigned int id, unsigned int index)
{
  yada_rc_t * result;

  if((result = db_query("DELETE FROM %s WHERE id = "
          "(SELECT id FROM %s AS a WHERE %d = "
          "(SELECT COUNT(id)+1 FROM %s AS b WHERE b.id < a.id AND b.parent_id = %d) "
          "AND parent_id = %d)", table, table, index, table, id, id)) == NULL)
  {
    printf("db: Failed to delete list index\n");
    return 0;
  }

  yada_rc_t *_free(result);
  return 1;
}

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
