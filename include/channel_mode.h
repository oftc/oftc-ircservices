/*
 *  channel_mode.h: The ircd channel mode header.
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
 *  $Id$
 */


#ifndef INCLUDED_channel_mode_h
#define INCLUDED_channel_mode_h

#define MODEBUFLEN    200

/* Maximum mode changes allowed per client, per server is different */
#define MAXMODEPARAMS 4

/* can_send results */
#define CAN_SEND_NO	0
#define CAN_SEND_NONOP  1
#define CAN_SEND_OPV	2


/* Channel related flags */
#define CHFL_CHANOP     0x0001 /* Channel operator   */
#define CHFL_HALFOP     0x0002 /* Channel half op    */
#define CHFL_VOICE      0x0004 /* the power to speak */
#define CHFL_DEOPPED    0x0008 /* deopped by us, modes need to be bounced */
#define CHFL_BAN        0x0010 /* ban channel flag */
#define CHFL_EXCEPTION  0x0020 /* exception to ban channel flag */
#define CHFL_INVEX      0x0040

/* channel modes ONLY */
#define MODE_PARANOID   0x0001
#define MODE_SECRET     0x0002
#define MODE_MODERATED  0x0004
#define MODE_TOPICLIMIT 0x0008
#define MODE_INVITEONLY 0x0010
#define MODE_NOPRIVMSGS 0x0020

/* cache flags for silence on ban */
#define CHFL_BAN_CHECKED  0x0080
#define CHFL_BAN_SILENCED 0x0100

#define CHACCESS_NOTONCHAN  -1
#define CHACCESS_PEON       0
#define CHACCESS_HALFOP     1
#define CHACCESS_CHANOP     2

/* name invisible */
#define SecretChannel(x)        (((x)->mode.mode & MODE_SECRET))
#define PubChannel(x)           (!SecretChannel(x))
/* knock is forbidden, halfops can't kick/deop other halfops.
 * +pi means paranoid and will generate notices on each invite */
#define ParanoidChannel(x)       (((x)->mode.mode & MODE_PARANOID))

struct ChModeChange
{
  char letter;
  const char *arg;
  const char *id;
  int dir;
  int caps;
  int nocaps;
  int mems;
  struct Client *client;
};

struct ChCapCombo
{
  int count;
  int cap_yes;
  int cap_no;
};

struct Ban
{
  dlink_node node;
  size_t len;
  char *name;
  char *username;
  char *host;
  char *who;
  struct irc_ssaddr addr;
  int bits;
  char type;
  time_t when;
};

struct Mode
{
  unsigned int mode;   /*!< simple modes */
  unsigned int limit;  /*!< +l userlimit */
  char key[KEYLEN];    /*!< +k key */
};

struct Membership;

EXTERN struct Callback *channel_access_cb;

void init_channel_modes(void);
void set_chcap_usage_counts(struct Client *);
void unset_chcap_usage_counts(struct Client *);

EXTERN void channel_modes(struct Channel *, struct Client *, char *, char *);

EXTERN int add_id(struct Client *, struct Channel *, char *, int);
EXTERN int del_id(struct Channel *, char *, int);
EXTERN void set_channel_mode(struct Client *, struct Client *, struct Channel *,
                             struct Membership *, int, char **, char *);
EXTERN void clear_ban_cache(struct Channel *);
EXTERN int has_member_flags(struct Membership *, unsigned int);

#define IsChanop(who, chan)      \
    has_member_flags(find_channel_link(who, chan), CHFL_CHANOP)

#endif /* INCLUDED_channel_mode_h */
