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

#define LOG_BUFSIZE 2048

static FBFILE *db_log_fb;
static database_t *database;

static void expire_sentmail(void *);
static void expire_akills(void *);

void
init_db()
{
  //char *dbstr;
  char logpath[LOG_BUFSIZE];
  char port[128] = {'\0'};
  //int len;
  char foo[128];

  strcpy(foo, "pgsql.la");

  if(Database.port != 0)
    snprintf(port, 127, "%d", Database.port);

  database = load_module(foo);
  if(!database->connect(""))
  {
    ilog(L_CRIT, "Cannot connect to database");
    exit(-1);
  }

  snprintf(logpath, LOG_BUFSIZE, "%s/%s", LOGDIR, Logging.sqllog);
  if(db_log_fb == NULL)
  {
    if(Logging.sqllog[0] != '\0' && (db_log_fb = fbopen(logpath, "r")) != NULL)
    {
      fbclose(db_log_fb);
      db_log_fb = fbopen(logpath, "a");
    }
  }
}

void
cleanup_db()
{
  int i;

  if(Database.yada != NULL)
  {
    Database.yada->disconnect(Database.yada);
    for(i = 0; i < QUERY_COUNT; i++)
    {
      query_t *query = &queries[i];
      Free(query->rc);
    }
    Database.yada->destroy(Database.yada);
  }
  fbclose(db_log_fb);
}

void
db_reopen_log()
{
  char logpath[LOG_BUFSIZE+1];

  if(db_log_fb != NULL)
  {
    fbclose(db_log_fb);
    db_log_fb = NULL;
  }

  snprintf(logpath, LOG_BUFSIZE, "%s/%s", LOGDIR, Logging.sqllog);
  if(db_log_fb == NULL)
  {
    if(Logging.sqllog[0] != '\0' && (db_log_fb = fbopen(logpath, "r")) != NULL)
    {
      fbclose(db_log_fb);
      db_log_fb = fbopen(logpath, "a");
    }
  }
}

void
db_log(const char *format, ...)
{
  char *buf;
  char lbuf[LOG_BUFSIZE];
  va_list args;
  size_t bytes;

  if(db_log_fb == NULL)
    return;

  va_start(args, format);
  vasprintf(&buf, format, args);
  va_end(args);

  bytes = snprintf(lbuf, sizeof(lbuf), "[%s] %s\n", smalldate(CurrentTime), buf);
  MyFree(buf);

  fbputs(lbuf, db_log_fb, bytes);
}

void
db_load_driver()
{
  eventAdd("Expire sent mail", expire_sentmail, NULL, 60); 
  eventAdd("Expire akills", expire_akills, NULL, 60); 

  execute_callback(on_db_init_cb);
}

void
db_try_reconnect()
{
  int num_attempts = 0;

  ilog(L_NOTICE, "Database connection lost! Attempting reconnect.");
  send_queued_all();

  while(num_attempts++ < 30)
  {
    if(Database.yada->connect(Database.yada, Database.username,
          Database.password) != 0)
    {
      int i;

      ilog(L_NOTICE, "Database connection restored after %d seconds",
          num_attempts * 5);
      for(i = 0; i < QUERY_COUNT; i++)
      {
        query_t *query = &queries[i];
        db_log("%d: %s", i, query->name);
        if(query->name == NULL)
          continue;
        query->rc = Prepare((char*)query->name, 0);
        if(query->rc == NULL)
          ilog(L_CRIT, "Prepare: %d Failed: %s", i, Database.yada->errmsg);
      }
      return;
    }
    sleep(5);
    ilog(L_NOTICE, "Database connection still down. Reconnect attempt %d",
        num_attempts);
    send_queued_all();
  }
  ilog(L_ERROR, "Database reconnect failed: %s", Database.yada->errmsg);
  services_die("Could not reconnect to database.", 0);
}

#define db_query(ret, query_id, args...) do                           \
{                                                                     \
} while(0)

#define db_exec(ret, query_id, args...) do                            \
{                                                                     \
} while(0)

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
 
  brc = Bind("?d?d?d?ps?ps?ps?ps?ps?ps?B?B?B?B?B?B?B?d?ps?ps?ps?d?d?d?d",
    &nick_p->id, &nick_p->pri_nickid, &nick_p->nickid, &retnick, 
    &retpass, &retsalt, &nick_p->url, &nick_p->email, &retcloak, 
    &nick_p->enforce, &nick_p->secure, &nick_p->verified, &nick_p->cloak_on, 
    &nick_p->admin, &nick_p->email_verified, &nick_p->priv, &nick_p->language, 
    &nick_p->last_host, &nick_p->last_realname, &nick_p->last_quit, 
    &nick_p->last_quit_time, &nick_p->reg_time, &nick_p->nick_reg_time, 
    &nick_p->last_seen);

  if(Fetch(rc, brc) == 0)
  {
    db_log("db_find_nick: '%s' not found.", nick);
    Free(brc);
    Free(rc);
    MyFree(nick_p);
    return NULL;
  }

  assert(retnick != NULL);
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

  db_log("db_find_nick: Found nick %s(asked for %s)", nick_p->nick, nick);

  Free(brc);
  Free(rc);

  return nick_p;
}

char *
db_get_nickname_from_id(unsigned int id)
{
  yada_rc_t *rc, *brc; 
  char *retnick; 

  db_query(rc, GET_NICK_FROM_ACCID, id);

  if(rc == NULL)
    return NULL;

  brc = Bind("?ps", &retnick);
  if(Fetch(rc, brc) == 0)
  {
    db_log("db_get_nickname_from_id: %d not found.", id);
    Free(brc);
    Free(rc);
    return NULL;
  }

  DupString(retnick, retnick);

  Free(brc);
  Free(rc);

  return retnick;
}

char *
db_get_nickname_from_nickid(unsigned int id)
{
  yada_rc_t *rc, *brc; 
  char *retnick; 

  db_query(rc, GET_NICK_FROM_NICKID, id);

  if(rc == NULL)
    return NULL;

  brc = Bind("?ps", &retnick);
  if(Fetch(rc, brc) == 0)
  {
    db_log("db_get_nickname_from_nickid: %d not found.", id);
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
    db_log("db_get_id_from_name: '%s' not found.", name);
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
  int exec, id, nickid, tmpid;

  assert(nick != NULL);

  TransBegin();

  nickid = NextID("nickname", "id");
  db_exec(exec, INSERT_ACCOUNT, nickid, nick->pass, nick->salt, nick->email, 
      CurrentTime);

  id = InsertID("account", "id");

  if(exec != -1)
    db_exec(exec, INSERT_NICK, nickid, nick->nick, id, CurrentTime, CurrentTime);

  tmpid = InsertID("nickname", "id");
  assert(tmpid == nickid);

  if(exec != -1)
    db_exec(exec, SET_NICK_MASTER, nickid, id);

  if(exec != -1)
  {
    if(TransCommit() != 0)
      return FALSE;

    nick->id = id;
    nick->nickid = nickid;
    nick->pri_nickid = nickid;
    nick->nick_reg_time = nick->reg_time = CurrentTime;
    return TRUE;
  }
  else
  {
    TransRollback();
    return FALSE;
  }
}

int 
db_forbid_chan(const char *c)
{
  int ret;
  struct RegChannel *chan;

  TransBegin();

  if((chan = db_find_chan(c)) != NULL)
  {
    db_delete_chan(c);
    free_regchan(chan);
  }

  db_exec(ret, INSERT_CHAN_FORBID, c);

  if(ret == -1)
  {
    TransRollback();
    return FALSE;
  }

  if(TransCommit() != 0)
    return FALSE;

  return TRUE;
}

int 
db_is_chan_forbid(const char *chan)
{
  yada_rc_t *rc, *brc;
  char *c;
  int ret;

  brc = Bind("?ps", &c);
  db_query(rc, GET_CHAN_FORBID, chan);

  if(rc == NULL)
    return FALSE;

  ret = Fetch(rc, brc);
  
  Free(rc);
  Free(brc);

  return ret;
}

int 
db_delete_chan_forbid(const char *chan)
{
  int ret;

  db_exec(ret, DELETE_CHAN_FORBID, chan);

  if(ret == -1)
    return FALSE;

  return TRUE;
}

int 
db_forbid_nick(const char *n)
{
  int ret;
  struct Nick *nick;

  TransBegin();

  if((nick = db_find_nick(n)) != NULL)
  {
    db_delete_nick(nick->id, nick->nickid, nick->pri_nickid);
    free_nick(nick);
  }

  db_exec(ret, INSERT_FORBID, n);

  if(ret == -1)
  {
    TransRollback();
    return FALSE;
  }

  if(TransCommit() != 0)
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

static int
db_fix_link(unsigned int id, unsigned int nickid)
{
  int new_nickid = -1;
  yada_rc_t *rc, *brc;

  brc = Bind("?d", &new_nickid);
  db_query(rc, GET_NEW_LINK, id, nickid);

  if(rc == NULL)
    return -1;

  if(Fetch(rc, brc) == 0)
  {
    Free(rc);
    Free(brc);
    return -1;
  }

  Free(rc);
  Free(brc);

  return new_nickid;
}

int
db_set_nick_master(unsigned int accid, const char *newnick)
{
  int newnickid;
  int ret;
  yada_rc_t *rc, *brc;

  brc = Bind("?d", &newnickid);
  db_query(rc, GET_NICKID_FROM_NICK, newnick);

  if(rc == NULL)
    return FALSE;

  if(Fetch(rc, brc) == 0)
  {
    Free(rc);
    Free(brc);
    return FALSE;
  }

  Free(rc);
  Free(brc);

  db_exec(ret, SET_NICK_MASTER, newnickid, accid);
  if(ret == -1)
    return FALSE;

  return TRUE;
}

int
db_delete_nick(unsigned int accid, unsigned int nickid, unsigned int priid)
{
  int ret;
  unsigned int newid;

  TransBegin();

  if(priid == nickid)
  {
    newid = db_fix_link(accid, nickid);

    if(newid == -1)
    {
      db_exec(ret, DELETE_NICK, nickid);
      if(ret != -1)
      {
        db_exec(ret, DELETE_ACCOUNT_CHACCESS, accid);
        if(ret != -1)
          db_exec(ret, DELETE_ACCOUNT, accid);
      }
    }
    else
    {
      db_exec(ret, SET_NICK_MASTER, newid, accid);
      if(ret != -1)
        db_exec(ret, DELETE_NICK, nickid);
    }
  }
  else
  {
    db_exec(ret, DELETE_NICK, nickid);
  }

  if(ret == -1)
  {
    TransRollback();
    return FALSE;
  }

  if(TransCommit() != 0)
    return FALSE;

  execute_callback(on_nick_drop_cb, accid, nickid, priid);

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
  //struct AccessEntry *aeval = (struct AccessEntry *)value;
  //struct ChanAccess *caval  = (struct ChanAccess *)value;
  struct ServiceBan *banval = (struct ServiceBan *)value;
//  struct JupeEntry *jval    = (struct JupeEntry *)value;
  int ret = 0;

  switch(type)
  {
    case ACCESS_LIST:
      db_exec(ret, INSERT_NICKACCESS, aeval->id, aeval->value);
      break;
    case CERT_LIST:
      db_exec(ret, INSERT_NICKCERT, aeval->id, aeval->value);
      break;
    case AKILL_LIST:
      db_exec(ret, INSERT_AKILL, banval->mask, banval->reason, 
          banval->setter, banval->time_set, banval->duration);
      break;
    case AKILL_SERVICES_LIST:
      db_exec(ret, INSERT_SERVICES_AKILL, banval->mask, banval->reason,
          banval->time_set, banval->duration);
      break;
    case AKICK_LIST:
      if(banval->target != 0)
      {
        db_exec(ret, INSERT_AKICK_ACCOUNT, banval->channel, banval->target,
            banval->setter, banval->reason, banval->time_set, 
            banval->duration);
      }
      else if(banval->mask != NULL)
      {
        db_exec(ret, INSERT_AKICK_MASK, banval->channel, banval->setter, 
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
    case JUPE_LIST:
      db_exec(ret, INSERT_JUPE, jval->setter, jval->name, jval->reason);
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
  struct InfoChanList *info;
  struct JupeEntry *jval;
  unsigned int query;
  
  switch(type)
  {
    case ACCESS_LIST:
      query = GET_NICKACCESS;
      aeval = MyMalloc(sizeof(struct AccessEntry));
      *entry = aeval;
      brc = Bind("?d?ps", &aeval->id, &aeval->value);
      break;
    case CERT_LIST:
      query = GET_NICKCERTS;
      aeval = MyMalloc(sizeof(struct AccessEntry));
      *entry = aeval;
      brc = Bind("?d?ps", &aeval->id, &aeval->value);
      break;
    case JUPE_LIST:
      query = GET_JUPE;
      jval = MyMalloc(sizeof(struct JupeEntry));
      *entry = jval;
      brc = Bind("?d?ps?ps?d", &jval->id, &jval->name, &jval->reason, &jval->setter);
      break;
    case ADMIN_LIST:
      query = GET_ADMINS;
      *entry = strval;
      brc = Bind("?ps", entry);
      break;
    case NICKLINK_LIST:
      query = GET_NICK_LINKS;
      *entry = strval;
      brc = Bind("?ps", entry);
      break;
    case AKILL_LIST:
      query = GET_AKILLS;

      banval = MyMalloc(sizeof(struct ServiceBan));
      *entry = banval;
      brc = Bind("?d?d?ps?ps?d?d", &banval->id, &banval->setter, &banval->mask, 
          &banval->reason, &banval->time_set, &banval->duration);
      break;
    case AKICK_LIST:
      query = GET_AKICKS;

      banval = MyMalloc(sizeof(struct ServiceBan));
      *entry = banval;
      brc = Bind("?d?d?d?d?ps?ps?d?d", &banval->id, &banval->channel,
          &banval->target, &banval->setter, &banval->mask, 
          &banval->reason, &banval->time_set, &banval->duration);
      break;
    case CHACCESS_LIST:
      query = GET_CHAN_ACCESSES;

      caval = MyMalloc(sizeof(struct ChanAccess));
      *entry = caval;
      brc = Bind("?d?d?d?d", &caval->id, &caval->channel, &caval->account,
          &caval->level);
      break;
    case NICKCHAN_LIST:
      query = GET_NICK_CHAN_INFO;

      info = MyMalloc(sizeof(struct InfoChanList));
      *entry = info;
      brc = Bind("?d?ps?d", &info->channel_id, &info->channel, &info->ilevel);
      break;
    case CHMASTER_LIST:
      query = GET_CHAN_MASTERS;

      *entry = strval;
      brc = Bind("?ps", entry);
      break;
    case NICK_LIST:
      query = GET_NICKS;

      *entry = strval;
      brc = Bind("?ps", entry);
      break;
    case NICK_LIST_OPER:
      query = GET_NICKS_OPER;

      *entry = strval;
      brc = Bind("?ps", entry);
      break;
    case NICK_FORBID_LIST:
      query = GET_FORBIDS;

      *entry = strval;
      brc = Bind("?ps", entry);
      break;
    case CHAN_LIST:
      query = GET_CHANNELS;

      *entry = strval;
      brc = Bind("?ps", entry);
      break;
    case CHAN_LIST_OPER:
      query = GET_CHANNELS_OPER;

      *entry = strval;
      brc = Bind("?ps", entry);
      break;

    case CHAN_FORBID_LIST:
      query = GET_CHANNEL_FORBID_LIST;

      *entry = strval;
      brc = Bind("?ps", entry);
      break;
  }

  db_query(rc, query, param);

  if(rc == NULL || brc == NULL)
  {
    if(brc != NULL)
      Free(brc);
    return NULL;
  }

  if(Fetch(rc, brc) == 0)
  {
    Free(brc);
    Free(rc);
    return NULL;
  }

  result = MyMalloc(sizeof(struct DBResult));

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
    banval->type = AKICK_BAN;
  }
  else if(type == JUPE_LIST)
  {
    DupString(jval->name, jval->name);
    DupString(jval->reason, jval->reason);
  }
  else if(type == NICKCHAN_LIST)
  {
    switch(info->ilevel)
    {
      case MASTER_FLAG:
        info->level = "MASTER";
        break;
      case CHANOP_FLAG:
        info->level = "CHANOP";
        break;
      case MEMBER_FLAG:
        info->level = "MEMBER";
        break;
    }
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
  struct InfoChanList *info;
  struct JupeEntry *jval;
  char *strval = (char*)*entry; 

  switch(type)
  {
    case ACCESS_LIST:
    case CERT_LIST:
      aeval = MyMalloc(sizeof(struct AccessEntry));
      *entry = aeval;
      Free(res->brc);
      res->brc = Bind("?d?ps", &aeval->id, &aeval->value);
      break;
    case JUPE_LIST:
      jval = MyMalloc(sizeof(struct JupeEntry));
      *entry = jval;
      Free(res->brc);
      res->brc = Bind("?d?ps?ps?d", &jval->id, &jval->name, &jval->reason, &jval->setter);
      break;
    case ADMIN_LIST:
    case NICKLINK_LIST:
    case CHMASTER_LIST:
    case NICK_LIST:
    case NICK_LIST_OPER:
    case NICK_FORBID_LIST:
    case CHAN_LIST:
    case CHAN_LIST_OPER:
    case CHAN_FORBID_LIST:
      *entry = strval;
      Free(res->brc);
      res->brc = Bind("?ps", entry);
      break;
    case AKILL_LIST:
      banval = MyMalloc(sizeof(struct ServiceBan));
      *entry = banval;
      Free(res->brc);
      res->brc = Bind("?d?d?ps?ps?d?d", &banval->id, &banval->setter, &banval->mask, 
          &banval->reason, &banval->time_set, &banval->duration);
     break;
    case AKICK_LIST:
      banval = MyMalloc(sizeof(struct ServiceBan));
      *entry = banval;
      Free(res->brc);
      res->brc = Bind("?d?ps?d?d?ps?ps?d?d", &banval->id, &banval->channel,
          &banval->target, &banval->setter, &banval->mask, 
          &banval->reason, &banval->time_set, &banval->duration);
      break;
    case CHACCESS_LIST:
      caval = MyMalloc(sizeof(struct ChanAccess));
      *entry = caval;
      Free(res->brc);
      res->brc = Bind("?d?d?d?d", &caval->id, &caval->channel, &caval->account,
          &caval->level);
      break;
    case NICKCHAN_LIST:
      info = MyMalloc(sizeof(struct InfoChanList));
      *entry = info;
      Free(res->brc);
      res->brc = Bind("?d?ps?d", &info->channel_id, &info->channel, &info->ilevel);
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
  }
  else if(type == JUPE_LIST)
  {
    DupString(jval->name, jval->name);
    DupString(jval->reason, jval->reason);
  }
  else if(type == NICKCHAN_LIST)
  {
    switch(info->ilevel)
    {
      case MASTER_FLAG:
        info->level = "MASTER";
        break;
      case CHANOP_FLAG:
        info->level = "CHANOP";
        break;
      case MEMBER_FLAG:
        info->level = "MEMBER";
        break;
    }
  }

  return result;
}

void
db_list_done(void *result)
{
  struct DBResult *res = (struct DBResult *)result;

  Free(res->brc);
  Free(res->rc);

  MyFree(res);
}

int
db_list_del(unsigned int type, unsigned int id, const char *param)
{
  int ret;

  if(id > 0)
    db_exec(ret, type, id, param);
  else
    db_exec(ret, type, param);

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
  {
    db_exec(ret, DELETE_DUPLICATE_CHACCESS, child, master, master, child);
    if(ret != -1)
    {
      db_exec(ret, MERGE_CHACCESS, master, child);
      if(ret != -1)
        db_exec(ret, DELETE_ACCOUNT, child);
    }
  }

  if(ret == -1)
  {
    TransRollback();
    return FALSE;
  }

  if(TransCommit() != 0)
    return FALSE;

  return TRUE;
}

unsigned int 
db_unlink_nick(unsigned int accid, unsigned int priid, unsigned int nickid)
{
  int ret;
  unsigned int new_accid, new_nickid;

  TransBegin();

  db_exec(ret, INSERT_NICK_CLONE, accid);
  if(ret != -1)
  {
    new_accid = InsertID("account", "id");
    if(priid != nickid)
      db_exec(ret, SET_NICK_LINK_EXCLUDE, new_accid, accid, nickid);
  }

  if(ret != -1)
  {
    new_nickid = db_fix_link(accid, priid);
    if(nickid == priid)
    {
      db_exec(ret, SET_NICK_MASTER, priid, accid);
      db_exec(ret, SET_NICK_MASTER, new_nickid, new_accid);
      db_exec(ret, SET_NICK_LINK_EXCLUDE, new_accid, accid, new_nickid);
    }
    else
      db_exec(ret, SET_NICK_MASTER, nickid, new_accid);
  }

  if(ret == -1)
  {
    TransRollback();
    return 0;
  }
  
  if(TransCommit() != 0)
    return FALSE;

  return new_accid;
}

struct RegChannel *
db_find_chan(const char *channel)
{
  yada_rc_t *rc, *brc;
  struct RegChannel *channel_p = NULL;
  char *retchan;

  assert(channel != NULL);

  db_query(rc, GET_FULL_CHAN, channel);

  if(rc == NULL)
    return NULL;
 
  channel_p = MyMalloc(sizeof(struct RegChannel));
 
  brc = Bind("?d?ps?ps?ps?d?B?B?B?B?B?B?B?B?B?B?ps?ps?ps?ps?d",
      &channel_p->id, &retchan, &channel_p->description, &channel_p->entrymsg, 
      &channel_p->regtime, &channel_p->priv, &channel_p->restricted,
      &channel_p->topic_lock, &channel_p->verbose, &channel_p->autolimit,
      &channel_p->expirebans, &channel_p->floodserv, &channel_p->autoop,
      &channel_p->autovoice, &channel_p->leaveops, &channel_p->url, 
      &channel_p->email, &channel_p->topic, &channel_p->mlock, 
      &channel_p->expirebans_lifetime);

  if(Fetch(rc, brc) == 0)
  {
    db_log("db_find_chan: '%s' not found.", channel);
    Free(brc);
    Free(rc);
    free_regchan(channel_p);
    return NULL;
  }

  strlcpy(channel_p->channel, retchan, sizeof(channel_p->channel));

  DupString(channel_p->description, channel_p->description);
  DupString(channel_p->entrymsg, channel_p->entrymsg);
  DupString(channel_p->url, channel_p->url);
  DupString(channel_p->email, channel_p->email);
  DupString(channel_p->topic, channel_p->topic);
  DupString(channel_p->mlock, channel_p->mlock);

  db_log("db_find_chan: Found chan %s", channel_p->channel);

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

  db_exec(ret, INSERT_CHAN, chan->channel, chan->description, CurrentTime,
      CurrentTime);

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

  access = MyMalloc(sizeof(struct ChanAccess));
  access->channel = chan->id;
  access->account = founder;
  access->level   = MASTER_FLAG;
 
  if(!db_list_add(CHACCESS_LIST, access))
  {
    TransRollback();
    return FALSE;
  }

  if(TransCommit() != 0)
    return FALSE;

  return TRUE;
}

int
db_delete_chan(const char *chan)
{
  int ret;

  db_exec(ret, DELETE_CHAN, chan);

  if(ret == -1)
    return FALSE;

  execute_callback(on_chan_drop_cb, chan);

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

  access = MyMalloc(sizeof(struct ChanAccess));
  brc = Bind("?d?d?d?d", &access->id, &access->channel, &access->account, 
      &access->level);

  if(Fetch(rc, brc) == 0)
  {
    MyFree(access);
    access = NULL;
  }

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

  akill = MyMalloc(sizeof(struct ServiceBan));

  brc = Bind("?d?ps?ps?d?d?d", &akill->id, &akill->mask, &akill->reason,
      &akill->setter, &akill->time_set, &akill->duration);

  if(Fetch(rc, brc) == 0)
  {
    db_log("db_find_akill: '%s' not found.", mask);
    Free(brc);
    Free(rc);
    free_serviceban(akill);
    return NULL;
  }

  DupString(akill->mask, akill->mask);
  DupString(akill->reason, akill->reason);

  db_log("db_find_akill: Found akill %s(asked for %s)", akill->mask, mask);

  Free(brc);
  Free(rc);

  return akill;
}

struct JupeEntry *
db_find_jupe(const char *name)
{
  yada_rc_t *rc, *brc;
  struct JupeEntry *jupe;

  assert(name != NULL);

  db_query(rc, FIND_JUPE, name);

  if(rc == NULL)
    return NULL;

  jupe = MyMalloc(sizeof(struct JupeEntry));

  brc = Bind("?d?ps?ps?d", &jupe->id, &jupe->name, &jupe->reason, &jupe->setter);

  if(Fetch(rc, brc) == 0)
  {
    db_log("db_find_jupe: '%s' not found.", name);
    Free(brc);
    Free(rc);
    free_jupeentry(jupe);
    return NULL;
  }

  DupString(jupe->name, jupe->name);
  DupString(jupe->reason, jupe->reason);

  db_log("db_find_jupe: Found jupe %s [%s]", jupe->name, jupe->reason);

  Free(brc);
  Free(rc);

  return jupe;
}

int
db_get_num_masters(unsigned int chanid)
{
  yada_rc_t *rc, *brc;
  int count;

  db_query(rc, GET_CHAN_MASTER_COUNT, chanid);

  if(rc == NULL)
    return 0;

  brc = Bind("?d", &count);

  if(Fetch(rc, brc) == 0)
    count = 0;

  Free(brc);
  Free(rc);

  return count;
}

int
db_get_num_channel_accesslist_entries(unsigned int chanid)
{
  yada_rc_t *rc, *brc;
  int count;

  db_query(rc, COUNT_CHANNEL_ACCESS_LIST, chanid);

  if(rc == NULL)
    return 0;

  brc = Bind("?d", &count);

  if(Fetch(rc, brc) == 0)
    count = 0;

  Free(brc);
  Free(rc);

  return count;
}

int
db_add_sentmail(unsigned int accid, const char *email)
{
  int ret;

  db_exec(ret, INSERT_SENT_MAIL, accid, email, CurrentTime);
  if(ret == -1)
    return 0;
  else
    return 1;
}

int
db_is_mailsent(unsigned int accid, const char *email)
{
  yada_rc_t *rc, *brc;
  int temp, ret = 0;

  db_query(rc, GET_SENT_MAIL, accid, email);

  if(rc == NULL)
    return 0;

  brc = Bind("?d", &temp);

  if(Fetch(rc, brc) == 0)
    ret = 0;
  else
    ret = 1;
  
  Free(brc);
  Free(rc);
  return ret;
}

char *
db_find_certfp(unsigned int accid, const char *certfp)
{
  yada_rc_t *rc, *brc;
  char *temp, *ret;
  
  db_query(rc, GET_NICKCERT, certfp, accid);

  if(rc == NULL)
    return NULL;

  brc = Bind("?ps", &temp);

  if(Fetch(rc, brc) == 0)
    ret = NULL;
  else
    DupString(ret, temp);

  Free(brc);
  Free(rc);

  return ret;
}

int
db_save_nick(struct Nick *nick)
{
  int ret;

  TransBegin();

  ret = db_set_number(SET_NICK_LAST_SEEN, nick->nickid, nick->last_seen);
  if(!ret)
  {
    TransRollback();
    return 0;
  }

  db_exec(ret, SAVE_NICK, nick->url, nick->email, nick->cloak,
      nick->enforce, nick->secure, nick->verified, nick->cloak_on,
      nick->admin, nick->email_verified, nick->priv, nick->language,
      nick->last_host, nick->last_realname, nick->last_quit, 
      nick->last_quit_time, nick->id);
  if(ret == -1)
  {
    TransRollback();
    return 0;
  }
  else
  {
    TransCommit();
    return 1;
  }
}

static void
expire_sentmail(void *param)
{
  //int ret;

  db_exec(ret, DELETE_EXPIRED_SENT_MAIL, Mail.expire_time, CurrentTime);
}

static void
expire_akills(void *param)
{
  yada_rc_t *rc, *brc;
  struct ServiceBan akill;
  char *setter;
  //int ret;

  db_query(rc, GET_EXPIRED_AKILL, CurrentTime);
  if(rc == NULL)
    return;

  brc = Bind("?d?ps?ps?ps?d?d", &akill.id, &setter, &akill.mask,
      &akill.reason, &akill.time_set, &akill.duration);

  while(Fetch(rc, brc) != 0)
  {
    ilog(L_NOTICE, "AKill on %s set by %s on %s(%s) has expired",
        akill.mask, setter, smalldate(akill.time_set), akill.reason);
    db_exec(ret, DELETE_AKILL, akill.mask);
  }

  Free(brc);
  Free(rc);
}
