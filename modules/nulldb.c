/*
 *  oftc-ircservices: an exstensible and flexible IRC Services package
 *  pgsql.c : A database module to interfacing with postgresql
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
 */

#include "stdinc.h"
#include "dbm.h"
#include "language.h"
#include "parse.h"
#include "chanserv.h"
#include "nickname.h"
#include "interface.h"
#include "conf/modules.h"

#define TEMP_BUFSIZE 32

static database_t *nulldb;

static int dbconnect(const char *);
static char *execute_scalar(int, int *, const char *, dlink_list*);
static result_set_t *execute(int, int *, const char *, dlink_list*);
static int execute_nonquery(int, const char *, dlink_list*);
static int prepare(int, const char *);
static int64_t insertid(const char *, const char *);
static int64_t nextid(const char *, const char *);
static int begin_transaction();
static int commit_transaction();
static int rollback_transaction();
static void free_result(result_set_t *);
static int is_connected();
static int process_notifies();

INIT_MODULE(nulldb, "$Revision: $")
{
  nulldb = MyMalloc(sizeof(database_t));

  nulldb->connect = dbconnect;
  nulldb->execute_scalar = execute_scalar;
  nulldb->execute = execute;
  nulldb->execute_nonquery = execute_nonquery;
  nulldb->free_result = free_result;
  nulldb->prepare = prepare;
  nulldb->begin_transaction = begin_transaction;
  nulldb->commit_transaction = commit_transaction;
  nulldb->rollback_transaction = rollback_transaction;
  nulldb->insert_id = insertid;
  nulldb->next_id = nextid;
  nulldb->is_connected = is_connected;
  nulldb->process_notifies = process_notifies;

  return nulldb;
}

CLEANUP_MODULE
{
}

static int
prepare(int id, const char *query)
{
  return 1;
}

static int
dbconnect(const char *connection_string)
{
  return 1;
}

static int
is_connected()
{
    return TRUE;
}

static char *
execute_scalar(int id, int *error, const char *format, dlink_list *args)
{
  return NULL;
}

static int
execute_nonquery(int id, const char *format, dlink_list *args)
{
  return 0;
}

static result_set_t *
execute(int id, int *error, const char *format, dlink_list *args)
{
  return NULL;
}

static void
free_result(result_set_t *result)
{
  return;
}

static int
begin_transaction()
{
  return TRUE;
}

static int
commit_transaction()
{
  return TRUE;
}

static int
rollback_transaction()
{
  return TRUE;
}

static int64_t
insertid(const char *table, const char *column)
{
  return 1;
}

static int64_t
nextid(const char *table, const char *column)
{
  return 1;
}

static int
process_notifies()
{
  return TRUE;
}
