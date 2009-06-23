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
 *  $Id: irc.c 1251 2007-12-05 21:48:18Z swalsh $
 */

#include <postgres.h>
#include <libpq-fe.h>
#include <catalog/pg_type.h>

#undef PACKAGE_VERSION
#undef PACKAGE_NAME
#undef PACKAGE_BUGREPORT
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME

#include "stdinc.h"
#include "dbm.h"
#include "language.h"
#include "parse.h"
#include "chanserv.h"
#include "nickname.h"
#include "interface.h"
#include "conf/modules.h"

#define TEMP_BUFSIZE 32

static database_t *pgsql;

static int pg_connect(const char *);
static char *pg_execute_scalar(int, int *, const char *, dlink_list*);
static result_set_t *pg_execute(int, int *, const char *, dlink_list*);
static int pg_execute_nonquery(int, const char *, dlink_list*);
static int pg_prepare(int, const char *);
static int64_t pg_insertid(const char *, const char *);
static int64_t pg_nextid(const char *, const char *);
static int pg_begin_transaction();
static int pg_commit_transaction();
static int pg_rollback_transaction();
static void pg_free_result(result_set_t *);
static int void_to_char(char, char **, void *);
static int pg_is_connected();

static query_t queries[QUERY_COUNT] = { 
  { GET_FULL_NICK, "SELECT account.id, primary_nick, nickname.id, "
    "(SELECT nick FROM nickname WHERE nickname.id=account.primary_nick), "
    "password, salt, url, email, cloak, flag_enforce, flag_secure, "
    "flag_verified, flag_cloak_enabled, flag_admin, flag_email_verified, "
    "flag_private, language, last_host, last_realname, "
    "last_quit_msg, last_quit_time, account.reg_time, nickname.reg_time, "
    "last_seen FROM account, nickname WHERE account.id = nickname.account_id AND "
    "lower(nick) = lower($1)", QUERY },
  { GET_NICK_FROM_ACCID, "SELECT nick from account, nickname WHERE account.id=$1 AND "
    "account.primary_nick=nickname.id", QUERY },
  { GET_NICK_FROM_NICKID, "SELECT nick from nickname WHERE id=$1", QUERY },
  { GET_ACCID_FROM_NICK, "SELECT account_id from nickname WHERE lower(nick)=lower($1)", QUERY },
  { GET_NICKID_FROM_NICK, "SELECT id from nickname WHERE lower(nick)=lower($1)", QUERY },
  { INSERT_ACCOUNT, "INSERT INTO account (primary_nick, password, salt, email, reg_time) VALUES "
    "($1, $2, $3, $4, $5)", EXECUTE },
  { INSERT_NICK, "INSERT INTO nickname (id, nick, account_id, reg_time, last_seen) VALUES "
    "($1, $2, $3, $4, $5)", EXECUTE },
  { DELETE_NICK, "DELETE FROM nickname WHERE id=$1", EXECUTE },
  { DELETE_ACCOUNT, "DELETE FROM account WHERE id=$1", EXECUTE },
  { INSERT_NICKACCESS, "INSERT INTO account_access (account_id, entry) VALUES($1, $2)", 
    EXECUTE },
  { GET_NICKACCESS, "SELECT id, entry FROM account_access WHERE account_id=$1 ORDER BY id", QUERY },
  { GET_ADMINS, "SELECT nick FROM account,nickname WHERE flag_admin=true AND "
    "account.primary_nick = nickname.id ORDER BY lower(nick)", QUERY },
  /* XXX: ORDER BY missing here */
  { GET_AKILLS, "SELECT akill.id, setter, mask, reason, time, duration FROM akill ORDER BY akill.time",
    QUERY },
  { GET_CHAN_ACCESSES, "SELECT channel_access.id, channel_access.channel_id, "
      "channel_access.account_id, channel_access.group_id, channel_access.level "
      "FROM channel_access JOIN account ON "
      "channel_access.account_id=account.id JOIN nickname ON "
      "account.primary_nick=nickname.id WHERE channel_id=$1 "
      "ORDER BY level, lower(nickname.nick) DESC", QUERY },
  { GET_CHANID_FROM_CHAN, "SELECT id from channel WHERE "
      "lower(channel)=lower($1)", QUERY },
  { GET_FULL_CHAN, "SELECT id, channel, description, entrymsg, reg_time, "
      "flag_private, flag_restricted, flag_topic_lock, flag_verbose, "
      "flag_autolimit, flag_expirebans, flag_floodserv, flag_autoop, "
      "flag_autovoice, flag_leaveops, url, email, topic, mlock, expirebans_lifetime, "
      "flag_autosave, last_used FROM channel WHERE lower(channel)=lower($1)", QUERY },
  { INSERT_CHAN, "INSERT INTO channel (channel, description, reg_time, last_used) "
    "VALUES($1, $2, $3, $4)", EXECUTE },
  { INSERT_CHANACCESS, "INSERT INTO channel_access (account_id, channel_id, level) VALUES "
    "($1, $2, $3)", EXECUTE } ,
  { SET_CHAN_LEVEL, "UPDATE channel_access SET level=$1 WHERE account_id=$2", EXECUTE },
  { DELETE_CHAN_ACCESS, "DELETE FROM channel_access WHERE id=$1", EXECUTE },
  { GET_CHAN_ACCESS, "SELECT id, channel_id, account_id, group_id, level "
    "FROM channel_access WHERE channel_id=$1 "
      "AND (account_id=$2 OR group_id IN (SELECT group_id FROM group_access "
      "WHERE account_id=$2)) ORDER BY level DESC LIMIT 1", QUERY },
  { GET_CHAN_ACCESS_EXACT, "SELECT id, channel_id, account_id, group_id, level "
    "FROM channel_access WHERE channel_id=$1 "
      "AND account_id=$2 LIMIT 1", QUERY },
  { DELETE_CHAN, "DELETE FROM channel WHERE id=$1", EXECUTE },
  { GET_AKILL, "SELECT id, setter, mask, reason, time, duration FROM akill WHERE mask=$1",
    QUERY },
  { INSERT_AKILL, "INSERT INTO akill (mask, reason, setter, time, duration) "
      "VALUES($1, $2, $3, $4, $5)", EXECUTE },
  { INSERT_SERVICES_AKILL, "INSERT INTO akill (mask, reason, time, duration) "
      "VALUES($1, $2, $3, $4)", EXECUTE },
  { SET_NICK_PASSWORD, "UPDATE account SET password=$1 WHERE id=$2", EXECUTE },
  { SET_NICK_SALT, "UPDATE account SET salt=$1 WHERE id=$2", EXECUTE },
  { SET_NICK_URL, "UPDATE account SET url=$1 WHERE id=$2", EXECUTE },
  { SET_NICK_EMAIL, "UPDATE account SET email=$1 WHERE id=$2", EXECUTE },
  { SET_NICK_CLOAK, "UPDATE account SET cloak=lower($1) WHERE id=$2", EXECUTE },
  { SET_NICK_LAST_QUIT, "UPDATE account SET last_quit_msg=$1 WHERE id=$2", EXECUTE },
  { SET_NICK_LAST_HOST, "UPDATE account SET last_host=$1 WHERE id=$2", EXECUTE },
  { SET_NICK_LAST_REALNAME, "UPDATE account SET last_realname=$1 where id=$2", EXECUTE },
  { SET_NICK_LANGUAGE, "UPDATE account SET language=$1 WHERE id=$2", EXECUTE },
  { SET_NICK_LAST_QUITTIME, "UPDATE account SET last_quit_time=$1 WHERE id=$2", EXECUTE },
  { SET_NICK_LAST_SEEN, "UPDATE nickname SET last_seen=$1 WHERE id=$2", EXECUTE },
  { SET_NICK_CLOAKON, "UPDATE account SET flag_cloak_enabled=$1 WHERE id=$2", EXECUTE },
  { SET_NICK_SECURE, "UPDATE account SET flag_secure=$1 WHERE id=$2", EXECUTE },
  { SET_NICK_ENFORCE, "UPDATE account SET flag_enforce=$1 WHERE id=$2", EXECUTE },
  { SET_NICK_ADMIN, "UPDATE account SET flag_admin=$1 WHERE id=$2", EXECUTE },
  { SET_NICK_PRIVATE, "UPDATE account SET flag_private=$1 WHERE id=$2", EXECUTE },
  { DELETE_NICKACCESS, "DELETE FROM account_access WHERE account_id=$1 AND entry=$2",
    EXECUTE },
  { DELETE_ALL_NICKACCESS, "DELETE FROM account_access WHERE account_id=$1", EXECUTE },
  /*{ DELETE_NICKACCESS_IDX, "DELETE FROM account_access WHERE id = "
          "(SELECT a.id FROM account_access AS a WHERE $1 = "
          "(SELECT COUNT(b.id)+1 FROM account_access AS b WHERE b.id < a.id AND "
          "b.account_id = $2) AND a.account_id = $2)", EXECUTE },*/
  { SET_NICK_LINK, "UPDATE nickname SET account_id=$1 WHERE account_id=$2", EXECUTE },
  { SET_NICK_LINK_EXCLUDE, "UPDATE nickname SET account_id=$1 WHERE account_id=$2 AND id=$3", EXECUTE },
  { INSERT_NICK_CLONE, "INSERT INTO account (primary_nick, password, salt, url, email, cloak, " 
    "flag_enforce, flag_secure, flag_verified, flag_cloak_enabled, "
    "flag_admin, flag_email_verified, flag_private, language, last_host, "
    "last_realname, last_quit_msg, last_quit_time, reg_time) "
    "SELECT primary_nick, password, salt, url, email, cloak, flag_enforce, "
    "flag_secure, flag_verified, flag_cloak_enabled, flag_admin, "
    "flag_email_verified, flag_private, language, last_host, last_realname, "
    "last_quit_msg, last_quit_time, reg_time FROM account WHERE id=$1", 
    EXECUTE },
  { GET_NEW_LINK, "SELECT id FROM nickname WHERE account_id=$1 AND NOT id=$2", QUERY },
  { SET_CHAN_LAST_USED, "UPDATE channel SET last_used=$1 WHERE id=$2", EXECUTE },
  { SET_CHAN_DESC, "UPDATE channel SET description=$1 WHERE id=$2", EXECUTE },
  { SET_CHAN_URL, "UPDATE channel SET url=$1 WHERE id=$2", EXECUTE },
  { SET_CHAN_EMAIL, "UPDATE channel SET email=$1 WHERE id=$2", EXECUTE },
  { SET_CHAN_ENTRYMSG, "UPDATE channel SET entrymsg=$1 WHERE id=$2", EXECUTE },
  { SET_CHAN_TOPIC, "UPDATE channel SET topic=$1 WHERE id=$2", EXECUTE },
  { SET_CHAN_MLOCK, "UPDATE channel SET mlock=$1 WHERE id=$2", EXECUTE },
  { SET_CHAN_PRIVATE, "UPDATE channel SET flag_private=$1 WHERE id=$2", EXECUTE },
  { SET_CHAN_RESTRICTED, "UPDATE channel SET flag_restricted=$1 WHERE id=$2", EXECUTE },
  { SET_CHAN_TOPICLOCK, "UPDATE channel SET flag_topic_lock=$1 WHERE id=$2", EXECUTE },
  { SET_CHAN_VERBOSE, "UPDATE channel SET flag_verbose=$1 WHERE id=$2", EXECUTE },
  { SET_CHAN_AUTOLIMIT, "UPDATE channel SET flag_autolimit=$1 WHERE id=$2", EXECUTE },
  { SET_CHAN_EXPIREBANS, "UPDATE channel SET flag_expirebans=$1 WHERE id=$2", EXECUTE },
  { SET_CHAN_FLOODSERV, "UPDATE channel SET flag_floodserv=$1 WHERE id=$2", EXECUTE },
  { SET_CHAN_AUTOOP, "UPDATE channel SET flag_autoop=$1 WHERE id=$2", EXECUTE },
  { SET_CHAN_AUTOVOICE, "UPDATE channel SET flag_autovoice=$1 WHERE id=$2", EXECUTE },
  { SET_CHAN_AUTOSAVE, "UPDATE channel SET flag_autosave=$1 WHERE id=$2", EXECUTE },
  { SET_CHAN_LEAVEOPS, "UPDATE channel SET flag_leaveops=$1 WHERE id=$2", EXECUTE },
  { INSERT_FORBID, "INSERT INTO forbidden_nickname (nick) VALUES ($1)", EXECUTE },
  { GET_FORBID, "SELECT nick FROM forbidden_nickname WHERE lower(nick)=lower($1)",
    QUERY },
  { DELETE_FORBID, "DELETE FROM forbidden_nickname WHERE lower(nick)=lower($1)", 
    EXECUTE },
  { INSERT_CHAN_FORBID, "INSERT INTO forbidden_channel (channel) VALUES ($1)", EXECUTE },
  { GET_CHAN_FORBID, "SELECT channel FROM forbidden_channel WHERE lower(channel)=lower($1)",
      QUERY },
  { DELETE_CHAN_FORBID, "DELETE FROM forbidden_channel WHERE lower(channel)=lower($1)",
    EXECUTE },
  { INSERT_AKICK_ACCOUNT, "INSERT INTO channel_akick (channel_id, target, setter, reason, "
    "time, duration, chmode) VALUES ($1, $2, $3, $4, $5, $6, $7)", EXECUTE },
  { INSERT_AKICK_MASK, "INSERT INTO channel_akick (channel_id, setter, reason, mask, "
    "time, duration, chmode) VALUES ($1, $2, $3, $4, $5, $6, $7)", EXECUTE },
  { GET_AKICKS, "SELECT channel_akick.id, channel_id, target, setter, mask, "
    "reason, time, duration, chmode FROM "
    "channel_akick WHERE channel_id=$1 AND chmode = $2 ORDER BY channel_akick.id", QUERY },
  /*{ DELETE_AKICK_IDX, "DELETE FROM channel_akick WHERE id = "
          "(SELECT id FROM channel_akick AS a WHERE $1 = "
          "(SELECT COUNT(id)+1 FROM channel_akick AS b WHERE b.id < a.id AND "
          "b.channel_id = $2) AND channel_id = $2)", EXECUTE },*/
  { DELETE_AKICK_MASK, "DELETE FROM channel_akick WHERE channel_id=$1 AND mask=$2 "
    " AND chmode = $3", EXECUTE },
  { DELETE_AKICK_ACCOUNT, "DELETE FROM channel_akick WHERE channel_id=$1 AND target IN (SELECT account_id "
    "FROM nickname WHERE lower(nick)=lower($2)) AND chmode = $3", EXECUTE },
  { SET_NICK_MASTER, "UPDATE account SET primary_nick=$1 WHERE id=$2", EXECUTE },
  { DELETE_AKILL, "DELETE FROM akill WHERE mask=$1", EXECUTE },
  { GET_CHAN_MASTER_COUNT, "SELECT COUNT(id) FROM channel_access WHERE channel_id=$1 AND level=4",
    QUERY },
  { GET_NICK_LINKS, "SELECT nick FROM nickname WHERE account_id=$1 ORDER BY lower(nick)", QUERY },
  { GET_NICK_LINKS_COUNT, "SELECT count(nick) FROM nickname WHERE account_id=$1", QUERY },
  { GET_NICK_CHAN_INFO, "SELECT channel.id, channel, level FROM "
    "channel, channel_access WHERE "
      "channel.id=channel_access.channel_id AND channel_access.account_id=$1 "
      "ORDER BY lower(channel.channel)", QUERY },
  { GET_CHAN_MASTERS, "SELECT nick FROM account, nickname, channel_access WHERE channel_id=$1 "
    "AND level=4 AND channel_access.account_id=account.id AND "
      "account.primary_nick=nickname.id ORDER BY lower(nick)", QUERY },
  { DELETE_ACCOUNT_CHACCESS, "DELETE FROM channel_access WHERE account_id=$1", EXECUTE },
  { DELETE_DUPLICATE_CHACCESS, "DELETE FROM channel_access WHERE "
      "(account_id=$1 AND level <= (SELECT level FROM channel_access AS x WHERE"
      " x.account_id=$2 AND x.channel_id = channel_access.channel_id)) OR "
      "(account_id=$3 AND level  < (SELECT level FROM channel_access AS x WHERE"
      " x.account_id=$4 AND x.channel_id = channel_access.channel_id))", 
      EXECUTE },
  { MERGE_CHACCESS, "UPDATE channel_access SET account_id=$1 WHERE account_id=$2", 
    EXECUTE },
  /*{ GET_EXPIRED_AKILL, "SELECT akill.id, nickname.nick, mask, reason, time, duration FROM "
    "account JOIN nickname ON "
    "account.primary_nick=nickname.id RIGHT OUTER JOIN akill ON "
    "akill.setter=account.id WHERE "*/
  { GET_EXPIRED_AKILL, "SELECT id, setter, mask, reason, time, duration FROM akill WHERE "
    "NOT duration = 0 AND time + duration < $1", QUERY },
  { INSERT_SENT_MAIL, "INSERT INTO sent_mail (account_id, email, sent) VALUES "
      "($1, $2, $3)", EXECUTE },
  { GET_SENT_MAIL, "SELECT id FROM sent_mail WHERE account_id=$1 OR email=$2",
    QUERY },
  { DELETE_EXPIRED_SENT_MAIL, "DELETE FROM sent_mail WHERE sent + $1 < $2", EXECUTE },
  { GET_NICKS, "SELECT nick FROM account, nickname WHERE account.id=nickname.account_id AND "
       "account.flag_private='f' ORDER BY lower(nick)", QUERY },
  { GET_NICKS_OPER, "SELECT nick FROM nickname ORDER BY lower(nick)", QUERY },
  { GET_FORBIDS, "SELECT nick FROM forbidden_nickname ORDER BY lower(nick)", QUERY },
  { GET_CHANNELS, "SELECT channel FROM channel WHERE flag_private='f' ORDER BY lower(channel)", QUERY },
  { GET_CHANNELS_OPER, "SELECT channel FROM channel ORDER BY lower(channel)", QUERY },
  { GET_CHANNEL_FORBID_LIST, "SELECT channel FROM forbidden_channel ORDER BY lower(channel)", QUERY },
  { SAVE_NICK, "UPDATE account SET url=$1, email=$2, cloak=$3, flag_enforce=$4, "
    "flag_secure=$5, flag_verified=$6, flag_cloak_enabled=$7, "
      "flag_admin=$8, flag_email_verified=$9, flag_private=$10, language=$11, "
      "last_host=$12, last_realname=$13, last_quit_msg=$14, last_quit_time=$15 "
      "WHERE id=$16", EXECUTE },
  { INSERT_NICKCERT, "INSERT INTO account_fingerprint (account_id, fingerprint, "
    "nickname_id) VALUES($1, upper($2), $3)", EXECUTE },
  { GET_NICKCERTS, "SELECT id, fingerprint, nickname_id FROM account_fingerprint "
    "WHERE account_id=$1 ORDER BY id", QUERY },
  { DELETE_NICKCERT, "DELETE FROM account_fingerprint WHERE "
    "account_id=$1 AND fingerprint=upper($2)", EXECUTE },
  /*{ DELETE_NICKCERT_IDX, "DELETE FROM account_fingerprint WHERE id = "
          "(SELECT id FROM account_fingerprint AS a WHERE $1 = "
          "(SELECT COUNT(id)+1 FROM account_fingerprint AS b WHERE b.id < a.id AND "
          "b.account_id = $2) AND account_id = $2)", EXECUTE },*/
  { DELETE_ALL_NICKACCESS, "DELETE FROM account_fingerprint WHERE "
    "account_id=$1", EXECUTE },
  { INSERT_JUPE, "INSERT INTO jupes (setter, name, reason) VALUES($1, $2, $3)",
    EXECUTE },
  { GET_JUPES, "SELECT id, name, reason, setter FROM jupes ORDER BY id", QUERY },
  { DELETE_JUPE_NAME, "DELETE FROM jupes WHERE lower(name) = lower($1)",
    EXECUTE },
  { FIND_JUPE, "SELECT id, name, reason, setter FROM jupes WHERE "
    "lower(name) = lower($1)", QUERY },
 { COUNT_CHANNEL_ACCESS_LIST, "SELECT COUNT(*) FROM channel_access "
    "JOIN account ON channel_access.account_id=account.id "
    "JOIN nickname ON account.primary_nick=nickname.id WHERE channel_id=$1",
    QUERY },
  { GET_NICKCERT, "SELECT fingerprint FROM account_fingerprint WHERE "
    "fingerprint=upper($1) AND account_id=$2", QUERY },
  { SET_EXPIREBANS_LIFETIME, "UPDATE channel SET expirebans_lifetime=$1 WHERE "
    "id=$2", EXECUTE },
  { GET_SERVICEMASK_MASKS, "SELECT mask FROM channel_akick WHERE channel_id = $1 "
    " AND chmode = $2", QUERY },
  { INSERT_GROUP, "INSERT INTO \"group\" (name, description, reg_time) "
    "VALUES ($1, $2, $3)", EXECUTE },
  { GET_FULL_GROUP, "SELECT id, name, description, email, url, flag_private, "
    "reg_time FROM \"group\" WHERE name=$1", QUERY },
  { DELETE_GROUP, "DELETE FROM \"group\" WHERE id=$1", EXECUTE },
  { GET_GROUP_FROM_GROUPID, "SELECT name FROM \"group\" WHERE id=$1", QUERY },
  { GET_GROUPID_FROM_GROUP, "SELECT id FROM \"group\" WHERE name=$1", QUERY },
  { SET_GROUP_URL, "UPDATE \"group\" SET url=$1 WHERE id=$2", EXECUTE },
  { SET_GROUP_DESC, "UPDATE \"group\" SET description=$1 WHERE id=$2", EXECUTE },
  { SET_GROUP_EMAIL, "UPDATE \"group\" SET email=$1 WHERE id=$2", EXECUTE },
  { SET_GROUP_PRIVATE, "UPDATE \"group\" SET flag_private=$1 WHERE id=$2",
    EXECUTE },
  { GET_GROUP_ACCESSES, "SELECT group_access.id, group_access.group_id, "
      "group_access.account_id, group_access.level FROM "
      "group_access JOIN account ON "
      "group_access.account_id=account.id JOIN nickname ON "
      "account.primary_nick=nickname.id WHERE group_id=$1 "
      "ORDER BY level, lower(nickname.nick) DESC", QUERY },
  { GET_GROUP_ACCESS, "SELECT id, group_id, account_id, level "
    "FROM group_access WHERE group_id=$1 AND account_id=$2", QUERY },
  { INSERT_GROUPACCESS, "INSERT INTO group_access "
    "(account_id, group_id, level) VALUES ($1, $2, $3)", EXECUTE } ,
  { DELETE_GROUPACCESS, "DELETE FROM group_access "
    "WHERE group_id=$1 AND account_id=$2", EXECUTE },
 { COUNT_GROUP_ACCESS_LIST, "SELECT COUNT(*) FROM group_access "
    "JOIN account ON group_access.account_id=account.id "
    "JOIN nickname ON account.primary_nick=nickname.id WHERE group_id=$1",
    QUERY },
  { GET_GROUP_MASTERS, "SELECT nick FROM account, nickname, group_access "
      "WHERE group_id=$1 AND level=3 AND group_access.account_id=account.id "
      "AND account.primary_nick=nickname.id ORDER BY lower(nick)", QUERY },
  { GET_GROUP_MASTER_COUNT, "SELECT COUNT(id) FROM group_access "
    "WHERE group_id=$1 AND level=4", QUERY },
  { INSERT_CHANACCESS_GROUP, "INSERT INTO channel_access "
    "(group_id, channel_id, level) VALUES ($1, $2, $3)", EXECUTE } ,
  { GET_CHAN_ACCESS_GROUP, "SELECT id, channel_id, account_id, group_id, level "
    "FROM channel_access WHERE channel_id=$1 AND group_id=$2", QUERY },
  { GET_CHAN_ACCESSES_GROUP, "SELECT ca.id, ca.channel_id, ca.account_id, "
      "ca.group_id, ca.level FROM channel_access AS ca "
      "JOIN \"group\" ON ca.group_id=\"group\".id WHERE ca.channel_id=$1 " 
      "ORDER BY level, lower(\"group\".name) DESC", QUERY },
  { GET_GROUPS_OPER, "SELECT name FROM \"group\" ORDER BY lower(name) DESC", QUERY },
  { GET_GROUPS, "SELECT name FROM \"group\" WHERE flag_private='f' ORDER BY lower(name) DESC",
    QUERY },
};


INIT_MODULE(pgsql, "$Revision: 1251 $")
{
  pgsql = MyMalloc(sizeof(database_t));

  pgsql->connect = pg_connect;
  pgsql->execute_scalar = pg_execute_scalar;
  pgsql->execute = pg_execute;
  pgsql->execute_nonquery = pg_execute_nonquery;
  pgsql->free_result = pg_free_result;
  pgsql->prepare = pg_prepare;
  pgsql->begin_transaction = pg_begin_transaction;
  pgsql->commit_transaction = pg_commit_transaction;
  pgsql->rollback_transaction = pg_rollback_transaction;
  pgsql->insert_id = pg_insertid;
  pgsql->next_id = pg_nextid;
  pgsql->is_connected = pg_is_connected;

  return pgsql;
}

CLEANUP_MODULE
{
  PQfinish(pgsql->connection);
  MyFree(pgsql);
}

static int 
pg_prepare(int id, const char *query)
{
  PGresult *result;
  char name[TEMP_BUFSIZE];
  int ret;

  snprintf(name, sizeof(name), "Query: %d", id);

  result = PQprepare(pgsql->connection, name, query, 0, NULL);
  if(result == NULL)
  {
    db_log("PG prepare Error: %s", PQerrorMessage(pgsql->connection));
    return 0;
  }

  ret = PQresultStatus(result);
  PQclear(result);

  if(ret != PGRES_COMMAND_OK)
  {
    db_log("PG prepare Error: %s", PQerrorMessage(pgsql->connection));
    return 0;
  }

  db_log("PG prepared: %s (%s)", name, query);

  return 1;
}

static int 
pg_connect(const char *connection_string)
{
  int i;

  if(pgsql->connection == NULL)
    pgsql->connection = PQconnectdb(connection_string);
  else
    PQreset(pgsql->connection);

  if(pgsql->connection == NULL)
    return 0;

  if(PQstatus(pgsql->connection) != CONNECTION_OK)
    return 0;

  for(i = 0; i < QUERY_COUNT; i++)
  {
    query_t *query = &queries[i];
    db_log("Prepare %d: %s", i, query->name);
    if(query->name == NULL)
      continue;
    if(!pg_prepare(i, query->name))
    {
      ilog(L_CRIT, "Prepare: %d Failed (%s)", i, PQerrorMessage(pgsql->connection));
      return 0;
    }
  }

  return 1;
}

static int
pg_is_connected()
{
  if(pgsql->connection != NULL && PQstatus(pgsql->connection) == CONNECTION_OK)
    return TRUE;
  else
    return FALSE;
}

static inline int
void_to_char(char fmt, char **dst, void *src)
{
  char tmp[TEMP_BUFSIZE];

  if(src == NULL)
  {
    *dst = NULL;
    return TRUE;
  }

  switch(fmt)
  {
    case 'i':
      snprintf(tmp, sizeof(tmp), "%d", *(int *)src);
      DupString(*dst, tmp);
      return TRUE;
      break;
    case 'b':
      if(((char *)src)[0])
        DupString(*dst, "1");
      else
        DupString(*dst, "0");
      return TRUE;
      break;
    case 's':
      DupString(*dst, (char *)src);
      return TRUE;
      break;
    default:
      db_log("PG Unknown param type: %c", fmt);
      break;
  }

  return FALSE;
}

size_t
join_log_params(char *target, int parc, char *parv[])
{
  size_t length = 0;
  int i;

  if(parv[0] != NULL)
    length += strlcpy(target, parv[0], IRC_BUFSIZE);
  for(i = 1; i < parc; i++)
  {
    length += strlcat(target, ", ", IRC_BUFSIZE);
    if(parv[i] != NULL)
      length += strlcat(target, parv[i], IRC_BUFSIZE);
    else
      length += strlcat(target, "NULL", IRC_BUFSIZE);
  }

  return length;
}

static PGresult *
internal_execute(int id, int *error, const char *format,
    dlink_list *args)
{
  PGresult *result;
  char **params;
  char name[TEMP_BUFSIZE];
  char log_params[IRC_BUFSIZE];
  int ret, len, i = 0;
  dlink_node *ptr = NULL;
  size_t count = 0;

  len = strlen(format);
  params = MyMalloc(sizeof(char*) * len);

  DLINK_FOREACH(ptr, args->head)
  {
    void_to_char(format[count], &params[count], ptr->data);
    count++;
  }

  snprintf(name, sizeof(name), "Query: %d", id);
  if(len > 0)
    join_log_params(log_params, len, (char**)params);

  result = PQexecPrepared(pgsql->connection, name, len, (const char**)params,
      NULL, NULL, 0);

  for(i = 0; i < len; i++)
    MyFree(params[i]);

  if(id < QUERY_COUNT)
  {
    db_log("Executing query %d (%s) Parameters: [%s]", id, queries[id].name, 
      len > 0 ? log_params : "None");
  }
  else
  {
    db_log("Execute dynamic query %d Parameters: [%s]", id,
      len > 0 ? log_params : "None");
  }

  if(result == NULL)
  {
    db_log("PG execute Error: %s", PQerrorMessage(pgsql->connection));
    *error = 1;
    return NULL;
  }

  ret = PQresultStatus(result);
  if(ret != PGRES_TUPLES_OK && ret != PGRES_COMMAND_OK)
  {
    db_log("PG execute Error(%d): %s", ret, PQerrorMessage(pgsql->connection));
    PQclear(result);
    *error = ret;
    return NULL;
  }

  if(PQntuples(result) == 0 || PQnfields(result) == 0)
    *error = 0;

  return result;
}

static char *
pg_execute_scalar(int id, int *error, const char *format, dlink_list *args)
{
  PGresult *result;
  char *value;

  result = internal_execute(id, error, format, args);

  if(result == NULL)
    return NULL;

  if(PQntuples(result) == 0 || PQgetisnull(result, 0, 0))
  {
    *error = 0;
    PQclear(result);
    return NULL;
  }

  DupString(value, PQgetvalue(result, 0, 0));

  PQclear(result);

  *error = 0;
  return value;
}

static int
pg_execute_nonquery(int id, const char *format, dlink_list *args)
{
  PGresult *result;
  int num_rows, error;

  result = internal_execute(id, &error, format, args);

  if(result == NULL)
    return -1;

  num_rows = atoi(PQcmdTuples(result));
  PQclear(result);

  return num_rows;
}

static result_set_t *
pg_execute(int id, int *error, const char *format, dlink_list *args)
{
  PGresult *result;
  result_set_t *results;
  int num_cols;
  int i, j;

  result = internal_execute(id, error, format, args);

  if(result == NULL)
    return NULL;

  results = MyMalloc(sizeof(result_set_t));

  results->row_count = PQntuples(result);
  if(results->row_count > 0)
    results->rows = MyMalloc(sizeof(row_t) * results->row_count);

  num_cols = PQnfields(result);

  for(i = 0; i < results->row_count; i++)
  {
    row_t *row = &results->rows[i];

    row->col_count = num_cols;
    if(num_cols > 0)
      row->cols = MyMalloc(sizeof(char**) * num_cols);
    for(j = 0; j < row->col_count; j++)
    {
      if(PQgetisnull(result, i, j))
        row->cols[j] = NULL;
      else
      {
        char *value = PQgetvalue(result, i, j);
        switch(PQftype(result, j))
        {
          case BOOLOID:
            if(*value == 't')
              DupString(row->cols[j], "1");
            else if(*value == 'f')
              DupString(row->cols[j], "0");
            break;
          case TIMESTAMPOID:
            DupString(row->cols[j], "lalaldate");
            break;
          default:
            DupString(row->cols[j], value);
            break;
        }
      }
    }
  }
  PQclear(result);
  *error = 0;

  return results;
}

static void
pg_free_result(result_set_t *result)
{
  int i, j;

  if(result == NULL)
    return;

  for(i = 0; i < result->row_count; i++)
  {
    row_t *row = &result->rows[i];

    for(j = 0; j < row->col_count; j++)
    {
      MyFree(row->cols[j]);
    }
    MyFree(row->cols);
  }
  MyFree(result->rows);
  MyFree(result);
}

static int
pg_begin_transaction()
{
  PGresult *result;
  int ret;

  result = PQexec(pgsql->connection, "BEGIN");
  if(result == NULL)
    return FALSE;
  
  ret = PQresultStatus(result);
  if(ret != PGRES_COMMAND_OK)
  {
    db_log("PG Begin Error(%d): %s", ret, PQerrorMessage(pgsql->connection));
    PQclear(result);
    return FALSE;
  }

  PQclear(result);
  return TRUE;
}

static int
pg_commit_transaction()
{
  PGresult *result;
  int ret;

  result = PQexec(pgsql->connection, "COMMIT");
  if(result == NULL)
    return FALSE;
  
  ret = PQresultStatus(result);
  if(ret != PGRES_COMMAND_OK)
  {
    db_log("PG Commit Error(%d): %s", ret, PQerrorMessage(pgsql->connection));
    PQclear(result);
    return FALSE;
  }

  PQclear(result);
  return TRUE;
}

static int
pg_rollback_transaction()
{
  PGresult *result;
  int ret;

  result = PQexec(pgsql->connection, "ROLLBACK");
  if(result == NULL)
    return FALSE;
  
  ret = PQresultStatus(result);
  if(ret != PGRES_COMMAND_OK)
  {
    db_log("PG Rollback Error(%d): %s", ret, PQerrorMessage(pgsql->connection));
    PQclear(result);
    return FALSE;
  }

  PQclear(result);
  return TRUE;
}

static int64_t
pg_insertid(const char *table, const char *column)
{
  char *pgquery;
  PGresult *result;
  int len, ret;
  int64_t id;

  len = strlen("SELECT currval(pg_get_serial_sequence('',''))") +
    strlen(table) + strlen(column) + 1;
  pgquery = MyMalloc(len);
  snprintf(pgquery, len, "SELECT currval(pg_get_serial_sequence('%s','%s'))",
      table, column);

  result = PQexec(pgsql->connection, pgquery);
  if(result == NULL)
    return -1;

  ret = PQresultStatus(result);
  if(ret != PGRES_TUPLES_OK)
  {
    db_log("PG inset_id Error(%d): %s", ret, PQerrorMessage(pgsql->connection));
    PQclear(result);
    MyFree(pgquery);
    return FALSE;
  }
  id = atol(PQgetvalue(result, 0, 0));

  PQclear(result);
  MyFree(pgquery);
  return(id);
}

static int64_t
pg_nextid(const char *table, const char *column)
{
  char *pgquery;
  PGresult *result;
  int len, ret;
  int64_t id;

  len = strlen("SELECT nextval(pg_get_serial_sequence('',''))") +
    strlen(table) + strlen(column) + 1;
  pgquery = MyMalloc(len);
  snprintf(pgquery, len, "SELECT nextval(pg_get_serial_sequence('%s','%s'))",
      table, column);

  result = PQexec(pgsql->connection, pgquery);
  if(result == NULL)
    return -1;

  ret = PQresultStatus(result);
  if(ret != PGRES_TUPLES_OK)
  {
    db_log("PG next_id Error(%d): %s", ret, PQerrorMessage(pgsql->connection));
    PQclear(result);
    MyFree(pgquery);
    return FALSE;
  }
  id = atol(PQgetvalue(result, 0, 0));

  PQclear(result);
  MyFree(pgquery);
  return(id);

}
