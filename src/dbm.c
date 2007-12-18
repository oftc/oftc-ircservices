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
  struct Module *mod;

  mod = find_module("pgsql.la", 0);
  unload_module(mod);
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

      ilog(L_NOTICE, "Database connection restored after %d seconds",
          num_attempts * 5);
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

int
db_prepare(int id, const char *query)
{
  if(database->last_index == 0)
    database->last_index = QUERY_COUNT;

  if(id == -1)
    id = database->last_index++;

  database->prepare(id, query);

  return id;
}

char *
db_execute_scalar(int query_id, int *error, const char *format, ...)
{
  va_list args;
  char *result;

  va_start(args, format);
  result = database->execute_scalar(query_id, error, format, args);
  va_end(args);

  return result;
}

result_set_t *
db_execute(int query_id, int *error, const char *format, ...)
{
  va_list args;
  result_set_t *results;

  va_start(args, format);
  results = database->execute(query_id, error, format, args);
  va_end(args);

  return results;
}

int
db_execute_nonquery(int query_id, const char *format, ...)
{
  va_list args;
  int num_rows;

  va_start(args, format);
  num_rows = database->execute_nonquery(query_id, format, args);
  va_end(args);

  return num_rows;
}

int
db_begin_transaction()
{
  return database->begin_transaction();
}

int
db_commit_transaction()
{
  return database->commit_transaction();
}

int
db_rollback_transaction()
{
  return database->rollback_transaction();
}

void
db_free_result(result_set_t *result)
{
  database->free_result(result);
}

int64_t
db_nextid(const char *table, const char *column)
{
  return database->next_id(table, column);
}

int64_t
db_insertid(const char *table, const char *column)
{
  return database->insert_id(table, column);
}

#define db_query(ret, query_id, args...) do                           \
{                                                                     \
} while(0)

#define db_exec(ret, query_id, args...) do                            \
{                                                                     \
} while(0)

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
    case CERT_LIST:
      query = GET_NICKCERTS;
      aeval = MyMalloc(sizeof(struct AccessEntry));
      *entry = aeval;
      brc = Bind("?d?ps", &aeval->id, &aeval->value);
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
    default:
      assert(0 == 1);
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
