/*
 *  oftc-ircservices: an exstensible and flexible IRC Services package
 *  hybridts6.c: A protocol handler for hybrid TS6
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

//static void m_mode(struct Client *, struct Client *, int, char*[]);

static void *irc_sendmsg_nick(va_list);
static void *irc_sendmsg_privmsg(va_list);
static void *irc_sendmsg_notice(va_list);
static void *irc_server_connected(va_list);

struct Message privmsg_msgtab = {
  "PRIVMSG", 0, 0, 2, 0, 0, 0,
  { process_privmsg, m_ignore }
};

static dlink_node *connected_hook;
static dlink_node *newuser_hook;
static dlink_node *privmsg_hook;
static dlink_node *notice_hook;

INIT_MODULE(hybrid_ts6, "$Revision$")
{
  connected_hook = install_hook(connected_cb, irc_server_connected);
  newuser_hook = install_hook(send_newuser_cb, irc_sendmsg_nick);
  privmsg_hook = install_hook(send_privmsg_cb, irc_sendmsg_privmsg);
  notice_hook  = install_hook(send_notice_cb, irc_sendmsg_notice);
  mod_add_cmd(&privmsg_msgtab);
}

CLEANUP_MODULE
{
  uninstall_hook(connected_cb, irc_server_connected);
  mod_del_cmd(&privmsg_msgtab);
}

/** Introduce a new server; currently only useful for connect and jupes
 * @param
 * prefix prefix, usually me.name
 * name server to introduce
 * info Server Information string
 */
static void 
irc_sendmsg_server(struct Client *client, char *prefix, char *name, char *info) 
{
  if (prefix == NULL) 
  {
    sendto_server(client, "SERVER %s 1 :%s", name, info);
  } 
  else 
  {
    sendto_server(client, ":%s SERVER %s 2 :%s", prefix, name, info);
  }
}

/** Introduce a new user
 * @param
 * nick Nickname of user
 * user username ("identd") of user
 * host hostname of that user
 * info Realname Information
 * umode usermode to add (i.e. "ao")
 */
static void *
irc_sendmsg_nick(va_list args)
{
  struct Client *client = va_arg(args, struct Client*);
  char          *nick   = va_arg(args, char *);
  char          *user   = va_arg(args, char *);
  char          *host   = va_arg(args, char *);
  char          *info   = va_arg(args, char *);
  char          *umode  = va_arg(args, char *);
  
  // NICK who hop ts umode user host server info
  sendto_server(client, "NICK %s 1 666 +%s %s %s %s :%s", 
    nick, umode, user, host, me.name, info);

  return NULL;
}

/** Send a Message to an user
 */
static void *
irc_sendmsg_privmsg(va_list args)
{
  struct Client *client = va_arg(args, struct Client *);
  char          *source = va_arg(args, char *);
  char          *target = va_arg(args, char *);
  char          *text   = va_arg(args, char *);
  
  sendto_server(client, ":%s PRIVMSG %s :%s", source, target, text);
  return NULL;
}

/** Send a Message to an user
 */
static void *
irc_sendmsg_notice(va_list args)
{
  struct Client *client = va_arg(args, struct Client *);
  char          *source = va_arg(args, char *);
  char          *target = va_arg(args, char *);
  char          *text   = va_arg(args, char *);
  
  sendto_server(client, ":%s NOTICE %s :%s", source, target, text);
  return NULL;
}


#if 0
XXX Not used atm
/** Change nick of one of our introduced fake clients
 * DO NOT use for remote clients or it will cause havoc, use SVSNICK instead
 * @param
 * oldnick Old nick of Client
 * newnick New nick of Client
 */
static void
irc_sendmsg_chnick(struct Client *client, char *oldnick, char *newnick)
{
  sendto_server(client, "%s NICK :%s", oldnick, newnick);
}

/** Quit a Client
 * @param
 * nick Nickname of user
 * reason Quit Reason
 */
static void 
irc_sendmsg_quit(struct Client *client, char *nick, char *reason)
{
  sendto_server(client, ":%s QUIT :%s", nick, reason);
}

/** SQuit a Server
 * @param
 * source who originated the squit
 * target Server to be squit
 * reason Why the squit?
 */
static void
irc_sendmsg_squit(struct Client *client, char *source, char *target, char *reason)
{
  sendto_server(client, ":%s SQUIT %s :%s", source, target, reason);
}


/** Send a PING to a remote client
 * @param
 * source Source of PING
 * target Target of PING
 */
static void
irc_sendmsg_ping(struct Client *client, char *source, char *target)
{
  sendto_server(client, ":%s PING :%s", source, target);
}

/** Let a client join a channel
 * @param
 * source who's joining?
 * target where is it joining?
 * mode mode to change with SJOIN, NULL if none
 * para parameter to modes (i.e. (+l) 42), NULL if none
 */
static void
irc_sendmsg_join(struct Client *client, char *source, char *target, char *mode, char *para)
{
  if (mode == NULL) 
  {
    mode = "0";
    para = "";
  }
  else if (para == NULL) 
  {
    para = "";
  }
  sendto_server(client, ":%s SJOIN 0 %s %s %s", source, target, mode, para);
}

/** Set User or Channelmode
 * not sanity checked!
 * source source of Modechange (server or client)
 * target target of Modechange (channel or client)
 * mode Mode to set (+ai)
 */
static void
irc_sendmsg_mode(struct Client *client, char *source, char *target, char *mode)
{
  sendto_server(client, ":%s MODE %s %s", source, target, mode);
}
#endif

static void *
irc_server_connected(va_list args)
{
  struct Client *client = va_arg(args, struct Client *);
  dlink_node *ptr;
  
  sendto_server(client, "PASS %s TS 5", client->server->pass);
  sendto_server(client, "CAPAB :KLN PARA EOB QS UNKLN GLN ENCAP TBURST CHW IE EX");
  irc_sendmsg_server(client, NULL, me.name, me.info);
  send_queued_write(client);

  me.uplink = client;

  /* Send out our list of services loaded */
  DLINK_FOREACH(ptr, services_list.head)
  {
    struct Service *service = ptr->data;

    introduce_service(service);

  }

  return NULL;
}
