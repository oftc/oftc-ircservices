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
#include "parse.h"
#include "nickserv.h"
#include "conf/database.h"
#include <libdbipp/dbipp.hh>

char *queries[QUERY_COUNT] = { 
  "SELECT account.id, (SELECT nick FROM nickname WHERE id=account.primary_nick), "
    "password, salt, url, email, cloak, flag_enforce, "
    "flag_secure, flag_verified, flag_cloak_enabled, "
    "flag_admin, flag_email_verified, language, last_host, last_realname, "
    "last_quit_msg, last_quit_time, account.reg_time, nickname.reg_time, "
    "last_seen FROM account, nickname WHERE account.id = nickname.user_id AND "
    "lower(nick) = lower(?v)",
  "SELECT nick from account, nickname WHERE account.id=?d AND "
    "account.primary_nick=nickname.id",
  "SELECT user_id from nickname WHERE lower(nick)=lower(?v)",
  "INSERT INTO account (password, salt, email, reg_time) VALUES "
    "(?v, ?v, ?v, ?d)",
  "INSERT INTO nickname (nick, user_id, reg_time, last_seen) VALUES "
    "(?v, ?d, ?d, ?d)",
  "DELETE FROM nickname WHERE lower(nick)=lower(?v)",
  "DELETE FROM account WHERE id=?d",
  "INSERT INTO account_access (parent_id, entry) VALUES(?d, ?v)",
  "SELECT id, entry FROM account_access WHERE parent_id=?d",
  "SELECT nick FROM account,nickname WHERE flag_admin=true AND "
    "account.primary_nick = nickname.id",
  "SELECT akill.id, setter, mask, reason, time, duration FROM akill",
  "SELECT id, channel_id, account_id, level FROM channel_access WHERE "
    "channel_id=?d",
  "SELECT id from channel WHERE lower(channel)=lower(?v)",
  "SELECT id, channel, description, entrymsg, flag_forbidden, flag_private, "
    "flag_restricted, flag_topic_lock, flag_verbose, flag_autolimit, "
    "flag_expirebans, url, email, topic, mlock FROM channel WHERE "
    "lower(channel)=lower(?v)",
  "INSERT INTO channel (channel, description) VALUES(?v, ?v)",
  "INSERT INTO channel_access (account_id, channel_id, level) VALUES "
    "(?d, ?d, ?d)" ,
  "UPDATE channel_access SET level=?d WHERE account_id=?d",
  "DELETE FROM channel_access WHERE channel_id=?d AND account_id=?d",
  "SELECT id, channel_id, account_id, level FROM channel_access WHERE "
    "channel_id=?d AND account_id=?d",
  "DELETE FROM channel WHERE lower(channel)=lower(?v)",
  "SELECT id, mask, reason, setter, time, duration FROM akill WHERE mask=?v",
  "INSERT INTO akill (mask, reason, setter, time, duration) "
    "VALUES(?v, ?v, ?d, ?d, ?d)",
  "UPDATE account SET password=?v WHERE id=?d",
  "UPDATE account SET url=?v WHERE id=?d",
  "UPDATE account SET email=?v WHERE id=?d",
  "UPDATE account SET cloak=?v WHERE id=?d",
  "UPDATE account SET last_quit_msg=?v WHERE id=?d",
  "UPDATE account SET last_host=?v WHERE id=?d",
  "UPDATE account SET last_realname=?v where id=?d",
  "UPDATE account SET language=?d WHERE id=?d",
  "UPDATE account SET last_quit_time=?d WHERE id=?d",
  "UPDATE nickname SET last_seen=?d WHERE user_id=?d",
  "UPDATE account SET flag_cloak_enabled=?B WHERE id=?d",
  "UPDATE account SET flag_secure=?B WHERE id=?d",
  "UPDATE account SET flag_enforce=?B WHERE id=?d",
  "UPDATE account SET flag_admin=?B WHERE id=?d",
  "DELETE FROM account_access WHERE parent_id=?d AND entry=?v",
  "DELETE FROM account_access WHERE parent_id=?d",
  "DELETE FROM account_access WHERE id = "
    "(SELECT id FROM account_access AS a WHERE ?d = "
    "(SELECT COUNT(id)+1 FROM account_access AS b WHERE b.id < a.id AND "
    "b.parent_id = ?d) AND parent_id = ?d)",
  "UPDATE nickname SET user_id=?d WHERE user_id=?d",
  "INSERT INTO account (password, salt, url, email, cloak, flag_enforce, "
    "flag_secure, flag_verified, flag_cloak_enabled, "
    "flag_admin, flag_email_verified, language, last_host, last_realname, "
    "last_quit_msg, last_quit_time, reg_time) SELECT password, salt, url, "
    "email, cloak, flag_enforce, flag_secure, flag_verified, "
    "flag_cloak_enabled, flag_admin, flag_email_verified, language, last_host, "
    "last_realname, last_quit_msg, last_quit_time, reg_time FROM account "
    "WHERE id=?d",
  "SELECT nick FROM nickname WHERE user_id=?d",
  "UPDATE channel SET description=?v WHERE id=?d",
  "UPDATE channel SET url=?v WHERE id=?d",
  "UPDATE channel SET email=?v WHERE id=?d",
  "UPDATE channel SET entrymsg=?v WHERE id=?d",
  "UPDATE channel SET topic=?v WHERE id=?d",
  "UPDATE channel SET mlock=?v WHERE id=?d",
  "UPDATE channel SET flag_forbidden=?B WHERE id=?d",
  "UPDATE channel SET flag_private=?B WHERE id=?d",
  "UPDATE channel SET flag_restricted=?B WHERE id=?d",
  "UPDATE channel SET flag_topic_lock=?B WHERE id=?d",
  "UPDATE channel SET flag_verbose=?B WHERE id=?d",
  "UPDATE channel SET flag_autolimit=?B WHERE id=?d",
  "UPDATE channel SET flag_expirebans=?B WHERE id=?d",
  "INSERT INTO forbidden_nickname (nick) VALUES (?v)",
  "SELECT nick FROM forbidden_nickname WHERE lower(nick)=lower(?v)",
  "DELETE FROM forbidden_nickname WHERE lower(nick)=lower(?v)",
  "INSERT INTO channel_akick (channel_id, target, setter, reason, "
    "time, duration) VALUES (?d, ?d, ?d, ?v, ?d, ?d)",
  "INSERT INTO channel_akick (channel_id, setter, reason, mask, "
    "time, duration) VALUES (?d, ?d, ?v, ?v, ?d, ?d)",
  "SELECT channel_akick.id, channel.channel, target, setter, mask, reason, time, duration FROM "
    "channel_akick, channel WHERE channel_id=?d AND channel.id=channel_id",
  "DELETE FROM channel_akick WHERE id = "
    "(SELECT id FROM channel_akick AS a WHERE ?d = "
    "(SELECT COUNT(id)+1 FROM channel_akick AS b WHERE b.id < a.id AND "
    "b.channel_id = ?d) AND channel_id = ?d)",
  "DELETE FROM channel_akick WHERE channel_id=?d AND mask=?v",
  "DELETE FROM channel_akick WHERE channel_id=?d AND target IN (SELECT user_id "
    "FROM nickname WHERE lower(nick)=lower(?v))",
  "UPDATE account SET primary_nick=(SELECT id FROM nickname WHERE "
    "lower(nick)=lower(?v)) WHERE id=?d",
  "DELETE FROM akill WHERE mask=?v",
  "SELECT COUNT(id) FROM channel_access WHERE channel_id=?d AND level=?d"
};


DBM::DBM()
{
}

DBM::~DBM()
{
}
void
DBM::connect()
{
}
