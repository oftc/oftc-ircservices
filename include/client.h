/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  client.h - Client related header file (IRC side)
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

#ifndef INCLUDED_client_h
#define INCLUDED_client_h

#include "nickname.h"

extern dlink_list global_client_list;
extern dlink_list global_server_list;
EXTERN unsigned int user_modes[];

#define FLAGS_PINGSENT      0x00000001UL /* Unreplied ping sent*/
#define FLAGS_DEADSOCKET    0x00000002UL /* Local socket is dead--Exiting soon*/
#define FLAGS_KILLED        0x00000004UL /* Prevents "QUIT" from being sent to this */
#define FLAGS_CLOSING       0x00000008UL /* set when closing */
#define FLAGS_CONNECTING    0x00000010UL /* Currently connecting not bursted */
#define FLAGS_ONACCESS      0x00000020UL /* Client isnt authed with nickserv but does match the access list*/
#define FLAGS_ENFORCE       0x00000040UL /* User is to be enforced */
#define FLAGS_SENTCERT      0x00000080UL /* User identified via SSL */

#define STAT_SERVER         0x01
#define STAT_CLIENT         0x02
#define STAT_ME             0x04

/* umodes, settable flags */
#define UMODE_SERVNOTICE   0x00001 /* server notices such as kill */
#define UMODE_CCONN        0x00002 /* Client Connections */
#define UMODE_REJ          0x00004 /* Bot Rejections */
#define UMODE_SKILL        0x00008 /* Server Killed */
#define UMODE_FULL         0x00010 /* Full messages */
#define UMODE_SPY          0x00020 /* see STATS / LINKS */
#define UMODE_DEBUG        0x00040 /* 'debugging' info */
#define UMODE_NCHANGE      0x00080 /* Nick change notice */
#define UMODE_WALLOP       0x00100 /* send wallops to them */
#define UMODE_OPERWALL     0x00200 /* Operwalls */
#define UMODE_INVISIBLE    0x00400 /* makes user invisible */
#define UMODE_BOTS         0x00800 /* shows bots */
#define UMODE_EXTERNAL     0x01000 /* show servers introduced and splitting */
#define UMODE_CALLERID     0x02000 /* block unless caller id's */
#define UMODE_SOFTCALLERID 0x04000 /* block unless on common channel */
#define UMODE_UNAUTH       0x08000 /* show unauth connects here */
#define UMODE_LOCOPS       0x10000 /* show locops */
#define UMODE_DEAF         0x20000 /* don't receive channel messages */

/* user information flags, only settable by remote mode or local oper */
#define UMODE_OPER         0x40000 /* Operator */
#define UMODE_ADMIN        0x80000 /* Admin on server */
#define UMODE_IDENTIFIED  0x100000 /* Registered with nickserv */
#define UMODE_GOD          0x10000000 /* Operator is God */
#define UMODE_ALL    UMODE_SERVNOTICE

#define HasID(x)		((x)->id[0] != '\0')

#define IsConnecting(x)         ((x)->flags & FLAGS_CONNECTING)
#define IsDefunct(x)            ((x)->flags & (FLAGS_DEADSOCKET|FLAGS_CLOSING))
#define IsDead(x)               ((x)->flags & FLAGS_DEADSOCKET)
#define IsClosing(x)            ((x)->flags & FLAGS_CLOSING)
#define IsOnAccess(x)           ((x)->flags & FLAGS_ONACCESS)
#define IsEnforce(x)            ((x)->flags & FLAGS_ENFORCE)
#define IsSentCert(x)           ((x)->flags & FLAGS_SENTCERT)

#define SetConnecting(x)        ((x)->flags |= FLAGS_CONNECTING)
#define SetClosing(x)           ((x)->flags |= FLAGS_CLOSING)
#define SetOnAccess(x)          ((x)->flags |= FLAGS_ONACCESS)
#define SetEnforce(x)           ((x)->flags |= FLAGS_ENFORCE)
#define SetSentCert(x)          ((x)->flags |= FLAGS_SENTCERT)

#define ClearConnecting(x)      ((x)->flags &= ~FLAGS_CONNECTING)
#define ClearOnAccess(x)        ((x)->flags &= ~FLAGS_ONACCESS)
#define ClearEnforce(x)         ((x)->flags &= ~FLAGS_ENFORCE)
#define ClearSentCert(x)        ((x)->flags &= ~FLAGS_SENTCERT)

#define IsServer(x)             ((x)->status & STAT_SERVER)
#define IsClient(x)             ((x)->status & STAT_CLIENT)
#define IsMe(x)                 ((x)->status & STAT_ME)

#define SetServer(x)            ((x)->status |= STAT_SERVER)
#define SetClient(x)            ((x)->status |= STAT_CLIENT)
#define SetMe(x)                ((x)->status |= STAT_ME)

#define IsOper(x)               ((x)->umodes & UMODE_OPER)
#define IsIdentified(x)         ((x)->umodes & UMODE_IDENTIFIED)
#define IsAdmin(x)              ((x)->umodes & UMODE_ADMIN)
#define IsGod(x)                ((x)->umodes & UMODE_GOD)

#define SetOper(x)              ((x)->umodes |= UMODE_OPER)
#define SetIdentified(x)        ((x)->umodes |= UMODE_IDENTIFIED)
#define SetAdmin(x)             ((x)->umodes |= UMODE_ADMIN)

#define ClearOper(x)            ((x)->umodes &= ~UMODE_OPER)
#define ClearIdentified(x)      ((x)->umodes &= ~UMODE_IDENTIFIED)

#define MyConnect(x)            ((x)->from == &me)
#define MyClient(x)             (MyConnect(x) && IsClient(x))

#define IDLEN           12 /* this is the maximum length, not the actual
                              generated length; DO NOT CHANGE! */

#define MODE_QUERY  0
#define MODE_ADD    1
#define MODE_DEL   -1

#define SHA1_DIGEST_LENGTH 40

struct Server
{
  dlink_node node;
  fde_t fd;
  int flags;
  struct dbuf_queue buf_recvq;
  struct dbuf_queue buf_sendq;
  char pass[PASSLEN+1];
};

struct Client
{
  dlink_node node;
  dlink_node lnode;
  dlink_list channel;

  dlink_list server_list;   /**< Servers on this server      */
  dlink_list client_list;   /**< Clients on this server      */

  struct Client *hnext;         /* For client hash table lookups by name */
  struct Client *idhnext;       /* For SID hash table lookups by sid */
  struct Client *from;
  struct Client *servptr;
  struct Client *uplink;        /* services uplink server */
  char *release_to;    /* The name of theclient this one will give its nick to */

  Nickname   *nickname;

  char          name[HOSTLEN+1];
  char          release_name[NICKLEN+1];
  char          host[HOSTLEN+1];
  char          realhost[HOSTLEN+1];
  char          sockhost[HOSTLEN+1];
  char          id[IDLEN + 1];      /* client ID, unique ID per client */
  char          info[REALLEN + 1];  /* Free form additional client info */
  char          username[USERLEN + 1];
  char          certfp[SHA1_DIGEST_LENGTH+1];
  char          ctcp_version[IRC_BUFSIZE];

  struct Server      *server;

  time_t        tsinfo;
  time_t        firsttime;
  time_t        enforce_time;
  time_t        release_time;
  unsigned int  status;
  unsigned int  umodes;
  unsigned int  access;
  unsigned int  num_badpass;    /* Number of incorrect passwords */
  unsigned int  hopcount;
  unsigned char handler;        /* Handler index */
  int flags;

  struct irc_ssaddr ip;
  int           aftype; 
} Client;

void init_client();
void cleanup_client();
struct Client *make_client(struct Client*);
struct Server *make_server(struct Client*);
struct Client *find_person(const struct Client *, const char *);
struct Client *find_chasing(struct Client *, const char *, int *);
void dead_link_on_write(struct Client *, int);
void set_user_mode(struct Client *, struct Client *, int, char *[]);
void exit_client(struct Client *, struct Client *, const char *);
int check_clean_nick(struct Client *, struct Client *, char *, char *, 
    struct Client *);
int check_clean_user(struct Client *, char *, char *, struct Client *);
int check_clean_host(struct Client *, char *, char *, struct Client *);
void nick_from_server(struct Client *, struct Client *, int,
                     char *[], time_t, char *, char *);
void register_remote_user(struct Client *, struct Client *,
                         const char *, const char *, const char *, const char *);
int valid_hostname(const char *);

#endif /* INCLUDED_client_h */
