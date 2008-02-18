/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  dbchannel.h channel related header file(database side)
 *
 *  Copyright (C) 2002 by the past and present ircd coders, and others.
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
 *  $Id: msg.h 957 2007-05-07 16:52:26Z stu $
 */

#ifndef INCLUDED_dbchannel_h
#define INCLUDED_dbchannel_h

typedef struct RegChannel *DBChannel;

DBChannel dbchannel_find(const char *);
int dbchannel_delete(DBChannel);
int dbchannel_forbid(const char *);
int dbchannel_delete_forbid(const char *);
int dbchannel_register(DBChannel, Nickname);
int dbchannel_is_forbid(const char *);
int dbchannel_masters_list(unsigned int, dlink_list *);
void dbchannel_masters_list_free(dlink_list *);
int dbchannel_masters_count(unsigned int, int *);
int dbchannel_list_all(dlink_list *);
void dbchannel_list_all_free(dlink_list *);
int dbchannel_list_regular(dlink_list *);
void dbchannel_list_regular_free(dlink_list *);
int dbchannel_list_forbid(dlink_list *);
void dbchannel_list_forbid_free(dlink_list *);

DBChannel dbchannel_new();

/* Member getters */
dlink_node dbchannel_get_node(DBChannel);
unsigned int dbchannel_get_id(DBChannel);
time_t dbchannel_get_regtime(DBChannel);
const char *dbchannel_get_channel(DBChannel);
const char *dbchannel_get_description(DBChannel);
const char *dbchannel_get_entrymsg(DBChannel);
const char *dbchannel_get_url(DBChannel);
const char *dbchannel_get_email(DBChannel);
const char *dbchannel_get_topic(DBChannel);
const char *dbchannel_get_mlock(DBChannel);
char dbchannel_get_priv(DBChannel);
char dbchannel_get_restricted(DBChannel);
char dbchannel_get_topic_lock(DBChannel);
char dbchannel_get_verbose(DBChannel);
char dbchannel_get_autolimit(DBChannel);
char dbchannel_get_expirebans(DBChannel);
char dbchannel_get_floodserv(DBChannel);
char dbchannel_get_autoop(DBChannel);
char dbchannel_get_autovoice(DBChannel);
char dbchannel_get_leaveops(DBChannel);
unsigned int dbchannel_get_expirebans_lifetime(DBChannel);

/* FloodServ */
struct MessageQueue **dbchannel_get_flood_hash(DBChannel);
dlink_list *dbchannel_get_flood_list(DBChannel);
struct MessageQueue *dbchannel_get_gqueue(DBChannel);

/* Memer setters */
void dbchannel_set_node(DBChannel, dlink_node);
void dbchannel_set_id(DBChannel, unsigned int);
void dbchannel_set_regtime(DBChannel, time_t);
void dbchannel_set_channel(DBChannel, const char *);
void dbchannel_set_description(DBChannel, const char *);
void dbchannel_set_entrymsg(DBChannel, const char *);
void dbchannel_set_url(DBChannel, const char *);
void dbchannel_set_email(DBChannel, const char *);
void dbchannel_set_topic(DBChannel, const char *);
void dbchannel_set_mlock(DBChannel, const char *);
void dbchannel_set_priv(DBChannel, char);
void dbchannel_set_restricted(DBChannel, char);
void dbchannel_set_topic_lock(DBChannel, char);
void dbchannel_set_verbose(DBChannel, char);
void dbchannel_set_autolimit(DBChannel, char);
void dbchannel_set_expirebans(DBChannel, char);
void dbchannel_set_floodserv(DBChannel, char);
void dbchannel_set_autoop(DBChannel, char);
void dbchannel_set_autovoice(DBChannel, char);
void dbchannel_set_leaveops(DBChannel, char);
void dbchannel_set_expirebans_lifetime(DBChannel, unsigned int);

/* FloodServ */
void dbchannel_set_flood_hash(DBChannel, struct MessageQueue **);
void dbchannel_set_flood_list(DBChannel, dlink_list *);
void dbchannel_set_gqueue(DBChannel, struct MessageQueue *);

void dbchannel_free(DBChannel);

#endif
