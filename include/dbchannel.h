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

#include "nickname.h"

typedef struct 
{
  dlink_node node;

  unsigned int id;
  time_t regtime;
  char channel[CHANNELLEN+1];
  char *description;
  char *entrymsg;
  char *url;
  char *email;
  char *topic;
  char *mlock;
  char priv;
  char restricted;
  char topic_lock;
  char verbose;
  char autolimit;
  char expirebans;
  char floodserv;
  char autoop;
  char autovoice;
  char leaveops;
  char autosave;
  unsigned int expirebans_lifetime;
  /* Per Host */
  struct MessageQueue **flood_hash;
  dlink_list flood_list;
  /* Per Channel */
  struct MessageQueue *gqueue;
} DBChannel;

DBChannel *dbchannel_find(const char *);
int dbchannel_delete(DBChannel *);
int dbchannel_forbid(const char *);
int dbchannel_delete_forbid(const char *);
int dbchannel_register(DBChannel *, Nickname *);
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

DBChannel *dbchannel_new();

/* Member getters */
dlink_node dbchannel_get_node(DBChannel *);
unsigned int dbchannel_get_id(DBChannel *);
time_t dbchannel_get_regtime(DBChannel *);
const char *dbchannel_get_channel(DBChannel *);
const char *dbchannel_get_description(DBChannel *);
const char *dbchannel_get_entrymsg(DBChannel *);
const char *dbchannel_get_url(DBChannel *);
const char *dbchannel_get_email(DBChannel *);
const char *dbchannel_get_topic(DBChannel *);
const char *dbchannel_get_mlock(DBChannel *);
char dbchannel_get_priv(DBChannel *);
char dbchannel_get_restricted(DBChannel *);
char dbchannel_get_topic_lock(DBChannel *);
char dbchannel_get_verbose(DBChannel *);
char dbchannel_get_autolimit(DBChannel *);
char dbchannel_get_expirebans(DBChannel *);
char dbchannel_get_floodserv(DBChannel *);
char dbchannel_get_autoop(DBChannel *);
char dbchannel_get_autovoice(DBChannel *);
char dbchannel_get_autosave(DBChannel *);
char dbchannel_get_leaveops(DBChannel *);
unsigned int dbchannel_get_expirebans_lifetime(DBChannel *);

/* FloodServ */
struct MessageQueue **dbchannel_get_flood_hash(DBChannel *);
dlink_list *dbchannel_get_flood_list(DBChannel *);
struct MessageQueue *dbchannel_get_gqueue(DBChannel *);

/* Memer setters */
inline int dbchannel_set_node(DBChannel *, dlink_node);
inline int dbchannel_set_id(DBChannel *, unsigned int);
inline int dbchannel_set_regtime(DBChannel *, time_t);
inline int dbchannel_set_channel(DBChannel *, const char *);
inline int dbchannel_set_description(DBChannel *, const char *);
inline int dbchannel_set_entrymsg(DBChannel *, const char *);
inline int dbchannel_set_url(DBChannel *, const char *);
inline int dbchannel_set_email(DBChannel *, const char *);
inline int dbchannel_set_topic(DBChannel *, const char *);
inline int dbchannel_set_mlock(DBChannel *, const char *);
inline int dbchannel_set_priv(DBChannel *, char);
inline int dbchannel_set_restricted(DBChannel *, char);
inline int dbchannel_set_topic_lock(DBChannel *, char);
inline int dbchannel_set_verbose(DBChannel *, char);
inline int dbchannel_set_autolimit(DBChannel *, char);
inline int dbchannel_set_expirebans(DBChannel *, char);
inline int dbchannel_set_floodserv(DBChannel *, char);
inline int dbchannel_set_autoop(DBChannel *, char);
inline int dbchannel_set_autovoice(DBChannel *, char);
inline int dbchannel_set_autosave(DBChannel *, char);
inline int dbchannel_set_leaveops(DBChannel *, char);
inline int dbchannel_set_expirebans_lifetime(DBChannel *, unsigned int);

/* FloodServ */
inline int dbchannel_set_flood_hash(DBChannel *, struct MessageQueue **);
inline int dbchannel_set_flood_list(DBChannel *, dlink_list *);
inline int dbchannel_set_gqueue(DBChannel *, struct MessageQueue *);

inline void dbchannel_free(DBChannel *);

#endif
