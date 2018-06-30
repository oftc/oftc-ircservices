/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  channel.h - IRC channel information
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

#ifndef INCLUDED_channel_h
#define INCLUDED_channel_h

#include "dbchannel.h"

struct Channel *make_channel(const char *);
void init_channel();
void cleanup_channel();
void remove_ban(struct Ban *bptr, dlink_list *list);
struct Membership *find_channel_link(struct Client *, struct Channel *);
void add_user_to_channel(struct Channel *, struct Client *, unsigned int, int);
void destroy_channel(struct Channel *);
void remove_user_from_channel(struct Membership *);
struct Ban *find_bmask(const struct Client *, const dlink_list *const);
void set_channel_topic(struct Channel *, const char *,const char *, time_t);

struct Channel
{
  struct Channel *hnextch;

  dlink_node node;

  struct Mode mode;

  char *topic;
  char *topic_info;

  dlink_list members;
  dlink_list invites;
  dlink_list banlist;
  dlink_list exceptlist;
  dlink_list invexlist;
  dlink_list quietlist;
  
  time_t channelts;
  time_t limit_time;

  char chname[CHANNELLEN + 1];

  DBChannel *regchan;
};

struct Membership
{
  dlink_node channode;     /*!< link to chptr->members    */
  dlink_node usernode;     /*!< link to source_p->channel */
  struct Channel *chptr;   /*!< Channel pointer */
  struct Client *client_p; /*!< Client pointer */
  unsigned int flags;      /*!< user/channel flags, e.g. CHFL_CHANOP */
};

#define IsMember(who, chan) ((find_channel_link(who, chan)) ? 1 : 0)
#define AddMemberFlag(x, y) ((x)->flags |=  (y))
#define DelMemberFlag(x, y) ((x)->flags &= ~(y))

extern dlink_list global_channel_list;

#endif /* INCLUDED_channel_h */
