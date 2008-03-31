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
#include "dbm.h"
#include "language.h"
#include "parse.h"
#include "nickserv.h"
#include "chanserv.h"
#include "nickname.h"
#include "interface.h"
#include "msg.h"
#include "send.h"
#include "dbmail.h"

#define LOG_BUFSIZE 2048

static FBFILE *db_log_fb;
static database_t *database;

void
init_db()
{
  //char *dbstr;
  char logpath[LOG_BUFSIZE];
  char port[128] = {'\0'};
  //int len;
  char foo[128];
  char cstring[IRC_BUFSIZE];

  strcpy(foo, "pgsql.la");

  if(Database.port != 0)
    snprintf(port, 127, "%d", Database.port);

  database = load_module(foo);

  snprintf(cstring, sizeof(cstring), "host='%s' user='%s' password='%s' dbname='%s'",
    Database.hostname, Database.username, Database.password,
    Database.dbname);

  if(!database->connect(cstring))
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
  eventAdd("Expire sent mail", dbmail_expire_sentmail, NULL, 60); 

  execute_callback(on_db_init_cb);
}

void
db_try_reconnect()
{
  /*
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
  */
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
  size_t i;
  dlink_list list = { 0 };
  size_t len = strlen(format);

  va_start(args, format);

  for(i = 0; i < len; ++i)
    dlinkAdd(va_arg(args, void *), make_dlink_node(), &list);

  va_end(args);

  result = database->execute_scalar(query_id, error, format, &list);

  return result;
}

result_set_t *
db_execute(int query_id, int *error, const char *format, ...)
{
  va_list args;
  size_t i;
  result_set_t *results;
  dlink_list list = { 0 };
  size_t len = strlen(format);

  va_start(args, format);

  for(i = 0; i < len; ++i)
    dlinkAdd(va_arg(args, void *), make_dlink_node(), &list);

  va_end(args);

  results = database->execute(query_id, error, format, &list);

  return results;
}

int
db_execute_nonquery(int query_id, const char *format, ...)
{
  va_list args;
  int num_rows;
  size_t i;
  dlink_list list = { 0 };
  size_t len = strlen(format);

  va_start(args, format);

  for(i = 0; i < len; ++i)
    dlinkAdd(va_arg(args, void *), make_dlink_node(), &list);

  va_end(args);

  num_rows = database->execute_nonquery(query_id, format, &list);

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

int
db_string_list(unsigned int query, dlink_list *list)
{
  int error, i;
  result_set_t *results;

  results = db_execute(query, &error, "", 0);

  if(results == NULL && error != 0)
  {
    ilog(L_CRIT, "db_string_list: query %d database error %d", query, error);
    return FALSE;
  }
  else if(results == NULL)
    return FALSE;

  if(results->row_count == 0)
    return FALSE;

  for(i = 0; i < results->row_count; ++i)
  {
    char *tmp;
    row_t *row = &results->rows[i];
    DupString(tmp, row->cols[0]);
    dlinkAdd(tmp, make_dlink_node(), list);
  }

  db_free_result(results);

  return dlink_list_length(list);
}

int
db_string_list_by_id(unsigned int query, dlink_list *list, unsigned int id)
{
  int error, i;
  result_set_t *results;

  results = db_execute(query, &error, "i", &id);

  if(results == NULL && error != 0)
  {
    ilog(L_CRIT, "db_string_list_by_id: query %d database error %d", query, error);
    return FALSE;
  }
  else if(results == NULL)
    return FALSE;

  if(results->row_count == 0)
    return FALSE;

  for(i = 0; i < results->row_count; ++i)
  {
    char *tmp;
    row_t *row = &results->rows[i];
    DupString(tmp, row->cols[0]);
    dlinkAdd(tmp, make_dlink_node(), list);
  }

  db_free_result(results);

  return dlink_list_length(list);
}

void
db_string_list_free(dlink_list *list)
{
  dlink_node *ptr, *next;
  char *tmp;

  ilog(L_DEBUG, "Freeing string list %p of length %lu", list,
    dlink_list_length(list));

  DLINK_FOREACH_SAFE(ptr, next, list->head)
  {
    tmp = (char *)ptr->data;
    MyFree(tmp);
    dlinkDelete(ptr, list);
    free_dlink_node(ptr);
  }
}
