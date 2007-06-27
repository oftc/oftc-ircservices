/*
 *  oftc-ircservices: an exstensible and flexible IRC Services package
 *  irc.c: The default IRC protocol handler
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

static void m_privmsg(struct Client *, struct Client *, int, char *[]);
static void m_notice(struct Client *, struct Client *, int, char *[]);
static void m_ping(struct Client *, struct Client *, int, char *[]);
static void m_nick(struct Client *, struct Client *, int, char*[]);
static void m_server(struct Client *, struct Client *, int, char*[]);
static void m_sjoin(struct Client *, struct Client *, int, char*[]);
static void m_part(struct Client *, struct Client *, int, char*[]);
static void m_quit(struct Client *, struct Client *, int, char*[]);
static void m_squit(struct Client *, struct Client *, int, char*[]);
static void m_mode(struct Client *, struct Client *, int, char*[]);
static void m_topic(struct Client *, struct Client *, int, char*[]);
static void m_kill(struct Client *, struct Client *, int, char*[]);
static void m_kick(struct Client *, struct Client *, int, char*[]);
static void m_version(struct Client *, struct Client *, int, char*[]);
static void m_stats(struct Client *, struct Client *, int, char*[]);
static void m_tburst(struct Client *, struct Client *, int, char*[]);

//static void do_user_modes(struct Client *client, const char *modes);

static void *irc_sendmsg_nick(va_list);
static void *irc_sendmsg_privmsg(va_list);
static void *irc_sendmsg_notice(va_list);
static void *irc_sendmsg_kick(va_list);
static void *irc_sendmsg_cmode(va_list);
static void *irc_sendmsg_invite(va_list);
static void *irc_sendmsg_akill(va_list);
static void *irc_sendmsg_unakill(va_list);
static void *irc_sendmsg_topic(va_list);
static void *irc_sendmsg_kill(va_list);
static void *irc_sendmsg_resv(va_list);
static void *irc_sendmsg_unresv(va_list);
static void *irc_sendmsg_server(va_list);
static void *irc_sendmsg_join(va_list);
static void *irc_server_connected(va_list);
static void *irc_sendmsg_nosuchsrv(va_list);
static void *part_one_client(va_list);
static char modebuf[MODEBUFLEN];
static char parabuf[MODEBUFLEN];
static char sendbuf[MODEBUFLEN];
static const char *para[MAXMODEPARAMS];
static char *mbuf;
static int pargs;

static struct Message away_msgtab = {
  "AWAY", 0, 0, 0, 0, 0, 0,
  { m_ignore, m_ignore }
};

static struct Message admin_msgtab = {
  "ADMIN", 0, 0, 0, 0, 0, 0,
  { m_ignore, m_ignore }
};

static struct Message version_msgtab = {
  "VERSION", 0, 0, 0, 0, 0, 0,
  { m_version, m_version }
};

static struct Message trace_msgtab = {
  "TRACE", 0, 0, 0, 0, 0, 0,
  { m_ignore, m_ignore }
};

static struct Message stats_msgtab = {
  "STATS", 0, 0, 0, 0, 0, 0,
  { m_stats, m_stats }
};

static struct Message whois_msgtab = {
  "WHOIS", 0, 0, 0, 0, 0, 0,
  { m_ignore, m_ignore }
};

static struct Message whowas_msgtab = {
  "WHOWAS", 0, 0, 0 ,0, 0, 0,
  { m_ignore, m_ignore }
};

static struct Message invite_msgtab = {
  "INVITE", 0, 0, 0, 0, 0, 0,
  { m_ignore, m_ignore }
};

static struct Message lusers_msgtab = {
  "LUSERS", 0, 0, 0, 0, 0, 0,
  { m_ignore, m_ignore }
};

static struct Message motd_msgtab = {
  "MOTD", 0, 0, 0, 0, 0, 0,
  { m_ignore, m_ignore }
};

static struct Message part_msgtab = {
  "PART", 0, 0, 2, 0, 0, 0,
  { m_part, m_ignore }
};

static struct Message ping_msgtab = {
  "PING", 0, 0, 1, 0, 0, 0,
  { m_ping, m_ignore }
};

static struct Message pong_msgtab = {
  "PONG", 0, 0, 1, 0, 0, 0,
  { m_ignore, m_ignore }
};

static struct Message eob_msgtab = {
  "EOB", 0, 0, 0, 0, 0, 0,
  { m_ignore, m_ignore }
};

static struct Message server_msgtab = {
  "SERVER", 0, 0, 3, 0, 0, 0,
  { m_server, m_ignore }
};

static struct Message nick_msgtab = {
  "NICK", 0, 0, 1, 0, 0, 0,
  { m_nick, m_ignore }
};

static struct Message sjoin_msgtab = {
  "SJOIN", 0, 0, 4, 0, 0, 0,
  { m_sjoin, m_ignore }
};

static struct Message quit_msgtab = {
  "QUIT", 0, 0, 0, 0, 0, 0,
  { m_quit, m_ignore }
};

static struct Message kill_msgtab = {
  "KILL", 0, 0, 0, 0, 0, 0,
  { m_kill, m_ignore }
};

static struct Message kick_msgtab = {
  "KICK", 0, 0, 0, 0, 0, 0,
  { m_kick, m_ignore }
};

static struct Message squit_msgtab = {
  "SQUIT", 0, 0, 1, 0, 0, 0,
  { m_squit, m_ignore }
};

static struct Message mode_msgtab = {
  "MODE", 0, 0, 2, 0, 0, 0,
  { m_mode, m_ignore }
};

static struct Message topic_msgtab = {
  "TOPIC", 0, 0, 2, 0, 0, 0, 
  { m_topic, m_ignore }
};

static struct Message privmsg_msgtab = {
  "PRIVMSG", 0, 0, 2, 0, 0, 0,
  { m_privmsg, m_ignore }
};

static struct Message notice_msgtab = {
  "NOTICE", 0, 0, 2, 0, 0, 0,
  { m_notice, m_ignore }
};

static struct Message tburst_msgtab = {
  "TBURST", 0, 0, 4, 0, 0, 0,
  { m_tburst, m_tburst }
};

static dlink_node *connected_hook;
static dlink_node *newuser_hook;
static dlink_node *privmsg_hook;
static dlink_node *notice_hook;
static dlink_node *kick_hook;
static dlink_node *cmode_hook;
static dlink_node *invite_hook;
static dlink_node *akill_hook;
static dlink_node *unakill_hook;
static dlink_node *topic_hook;
static dlink_node *kill_hook;
static dlink_node *resv_hook;
static dlink_node *unresv_hook;
static dlink_node *newserver_hook;
static dlink_node *join_hook;
static dlink_node *part_hook;
static dlink_node *nosuchsrv_hook;

struct ModeList ModeList[] = {
  { MODE_NOPRIVMSGS,  'n' },
  { MODE_TOPICLIMIT,  't' },
  { MODE_SECRET,      's' },
  { MODE_MODERATED,   'm' },
  { MODE_INVITEONLY,  'i' },
  { MODE_PARANOID,    'p' },
  { 0, '\0' }
};

INIT_MODULE(irc, "$Revision$")
{
  connected_hook  = install_hook(connected_cb, irc_server_connected);
  newuser_hook    = install_hook(send_newuser_cb, irc_sendmsg_nick);
  privmsg_hook    = install_hook(send_privmsg_cb, irc_sendmsg_privmsg);
  notice_hook     = install_hook(send_notice_cb, irc_sendmsg_notice);
  kick_hook       = install_hook(send_kick_cb, irc_sendmsg_kick);
  cmode_hook      = install_hook(send_cmode_cb, irc_sendmsg_cmode);
  invite_hook     = install_hook(send_invite_cb, irc_sendmsg_invite);
  akill_hook      = install_hook(send_akill_cb, irc_sendmsg_akill);
  unakill_hook    = install_hook(send_unakill_cb, irc_sendmsg_unakill);
  topic_hook      = install_hook(send_topic_cb, irc_sendmsg_topic);
  kill_hook       = install_hook(send_kill_cb, irc_sendmsg_kill);
  resv_hook       = install_hook(send_resv_cb, irc_sendmsg_resv);
  unresv_hook     = install_hook(send_unresv_cb, irc_sendmsg_unresv);
  newserver_hook  = install_hook(send_newserver_cb, irc_sendmsg_server);
  join_hook       = install_hook(send_join_cb, irc_sendmsg_join);
  part_hook       = install_hook(send_part_cb, part_one_client);
  nosuchsrv_hook  = install_hook(send_nosuchsrv_cb, irc_sendmsg_nosuchsrv);
  mod_add_cmd(&ping_msgtab);
  mod_add_cmd(&server_msgtab);
  mod_add_cmd(&nick_msgtab);
  mod_add_cmd(&sjoin_msgtab);
  mod_add_cmd(&eob_msgtab);
  mod_add_cmd(&part_msgtab);
  mod_add_cmd(&quit_msgtab);
  mod_add_cmd(&squit_msgtab);
  mod_add_cmd(&mode_msgtab);
  mod_add_cmd(&pong_msgtab);
  mod_add_cmd(&privmsg_msgtab);
  mod_add_cmd(&away_msgtab);
  mod_add_cmd(&admin_msgtab);
  mod_add_cmd(&whois_msgtab);
  mod_add_cmd(&whowas_msgtab);
  mod_add_cmd(&invite_msgtab);
  mod_add_cmd(&lusers_msgtab);
  mod_add_cmd(&motd_msgtab);
  mod_add_cmd(&version_msgtab);
  mod_add_cmd(&trace_msgtab);
  mod_add_cmd(&stats_msgtab);
  mod_add_cmd(&topic_msgtab);
  mod_add_cmd(&kill_msgtab);
  mod_add_cmd(&kick_msgtab);
  mod_add_cmd(&notice_msgtab);
  mod_add_cmd(&tburst_msgtab);
}

CLEANUP_MODULE
{
  uninstall_hook(connected_cb, irc_server_connected);
  mod_del_cmd(&ping_msgtab);
  mod_del_cmd(&server_msgtab);
  mod_del_cmd(&nick_msgtab);
  mod_del_cmd(&sjoin_msgtab);
  mod_del_cmd(&eob_msgtab);
  mod_del_cmd(&part_msgtab);
  mod_del_cmd(&quit_msgtab);
  mod_del_cmd(&squit_msgtab);
  mod_del_cmd(&mode_msgtab);
  mod_del_cmd(&pong_msgtab);
  mod_del_cmd(&privmsg_msgtab);
  mod_del_cmd(&away_msgtab);
  mod_del_cmd(&admin_msgtab);
  mod_del_cmd(&whois_msgtab);
  mod_del_cmd(&whowas_msgtab);
  mod_del_cmd(&invite_msgtab);
  mod_del_cmd(&lusers_msgtab);
  mod_del_cmd(&motd_msgtab);
  mod_del_cmd(&version_msgtab);
  mod_del_cmd(&trace_msgtab);
  mod_del_cmd(&stats_msgtab);
  mod_del_cmd(&topic_msgtab);
  mod_del_cmd(&tburst_msgtab);
  mod_del_cmd(&notice_msgtab);
  mod_del_cmd(&kick_msgtab);
  mod_del_cmd(&kill_msgtab);
}

/** Introduce a new server; currently only useful for connect and jupes
 * @param
 * prefix prefix, usually me.name
 * name server to introduce
 * info Server Information string
 */
static void *
irc_sendmsg_server(va_list args) 
{
  struct Client *client = va_arg(args, struct Client *);

  sendto_server(me.uplink, "SERVER %s 1 :%s", client->name, client->info);

  return NULL;
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

static void *
irc_sendmsg_kick(va_list args)
{
  struct Client *client   = va_arg(args, struct Client *);
  char          *source   = va_arg(args, char *);
  char          *channel  = va_arg(args, char *); 
  char          *target   = va_arg(args, char *);
  char          *reason   = va_arg(args, char *);
  
  sendto_server(client, ":%s KICK %s %s :%s", 
      (source != NULL) ? source : me.name, channel, target, reason);
  return NULL;
}

static void *
irc_sendmsg_cmode(va_list args)
{
  struct Client *client   = va_arg(args, struct Client *);
  char          *source   = va_arg(args, char *);
  char          *channel  = va_arg(args, char *); 
  char          *mode     = va_arg(args, char *);
  char          *param    = va_arg(args, char *);
  
  sendto_server(client, ":%s MODE %s %s %s", 
      (source != NULL) ? source : me.name, channel, mode, param);
  return NULL;
}

static void *
irc_sendmsg_invite(va_list args)
{
  struct Client   *uplink   = va_arg(args, struct Client *);
  struct Service  *source   = va_arg(args, struct Service *);
  struct Channel  *channel  = va_arg(args, struct Channel *); 
  struct Client   *target   = va_arg(args, struct Client *);
  
  sendto_server(uplink, ":%s INVITE %s %s 1", 
      (source != NULL) ? source->name : me.name, target->name, channel->chname);
  return NULL;
}

static void *
irc_sendmsg_akill(va_list args)
{
  struct Client   *uplink   = va_arg(args, struct Client *);
  struct Service  *source   = va_arg(args, struct Service *);
  char            *setter   = va_arg(args, char *);
  char            *mask     = va_arg(args, char *);
  char            *reason   = va_arg(args, char *);
  time_t          duration  = va_arg(args, time_t);

  setter = setter;

  char name[NICKLEN];
  char user[USERLEN + 1];
  char host[HOSTLEN + 1];
  struct split_nuh_item nuh;

  nuh.nuhmask  = mask;
  nuh.nickptr  = name;
  nuh.userptr  = user;
  nuh.hostptr  = host;

  nuh.nicksize = sizeof(name);
  nuh.usersize = sizeof(user);
  nuh.hostsize = sizeof(host);

  split_nuh(&nuh);

  sendto_server(uplink, ":%s KLINE * %ld %s %s :autokilled: %s", 
      (source != NULL) ? source->name : me.name, duration, user, host, reason);

  return NULL;
}

static void *
irc_sendmsg_unakill(va_list args)
{
  struct Client   *uplink   = va_arg(args, struct Client *);
  struct Service  *source   = va_arg(args, struct Service *);
  char            *mask     = va_arg(args, char *);
  char name[NICKLEN];
  char user[USERLEN + 1];
  char host[HOSTLEN + 1];
  struct split_nuh_item nuh;

  nuh.nuhmask  = mask;
  nuh.nickptr  = name;
  nuh.userptr  = user;
  nuh.hostptr  = host;

  nuh.nicksize = sizeof(name);
  nuh.usersize = sizeof(user);
  nuh.hostsize = sizeof(host);

  split_nuh(&nuh);

  sendto_server(uplink, ":%s UNKLINE * %s %s", 
      (source != NULL) ? source->name : me.name, user, host);

  return NULL;
}

static void *
irc_sendmsg_topic(va_list args)
{
  struct Client   *uplink   = va_arg(args, struct Client *);
  struct Service  *source   = va_arg(args, struct Service *);
  struct Channel  *chptr    = va_arg(args, struct Channel *);
  struct Client   *setter   = va_arg(args, struct Client *);
  char            *topic    = va_arg(args, char *);

  source = source;

  if(topic == NULL)
  {
    sendto_server(uplink, ":%s TBURST 1 %s %lu %s", me.name, chptr->chname,
        CurrentTime, setter->name);
  }
  else
  {
    sendto_server(uplink, ":%s TBURST 1 %s %lu %s :%s", me.name, chptr->chname,
        CurrentTime, setter->name, topic);
  }

  return NULL;
}

static void *
irc_sendmsg_kill(va_list args)
{
  struct Client   *uplink   = va_arg(args, struct Client *);
  struct Service  *source   = va_arg(args, struct Service *);
  struct Client   *target   = va_arg(args, struct Client *);
  char            *reason   = va_arg(args, char *);

  sendto_server(uplink, ":%s KILL %s :%s (%s)", 
      (source != NULL) ? source->name : me.name, target->name, me.name, reason);

  return NULL;
}

static void *
irc_sendmsg_resv(va_list args)
{
  struct Client   *uplink   = va_arg(args, struct Client *);
  struct Service  *source   = va_arg(args, struct Service *);
  char            *resv     = va_arg(args, char *);
  char            *reason   = va_arg(args, char *);
  time_t          duration  = va_arg(args, time_t);

  if(duration == 0)
  {
    sendto_server(uplink, ":%s RESV * %s :%s", 
        (source != NULL) ? source->name : me.name, resv, reason);
  }
  else
  {
    sendto_server(uplink, ":%s ENCAP * RESV %ld %s :%s", 
        (source != NULL) ? source->name : me.name, duration, resv, reason);
  }
 
  return NULL;
}

static void *
irc_sendmsg_unresv(va_list args)
{
  struct Client   *uplink   = va_arg(args, struct Client *);
  struct Service  *source   = va_arg(args, struct Service *);
  char            *resv     = va_arg(args, char *);

  sendto_server(uplink, ":%s UNRESV * %s", 
      (source != NULL) ? source->name : me.name, resv);
 
  return NULL;
}



#if 0


XXX Unused as yet
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
#endif

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
 * chptr->mode.mode to change with SJOIN, NULL if none
 * para parameter to modes (i.e. (+l) 42), NULL if none
 */
static void *
irc_sendmsg_join(va_list args)
{
  struct Client *client = va_arg(args, struct Client *);
  char *source          = va_arg(args, char *);
  char *target          = va_arg(args, char *);
  time_t ts             = va_arg(args, time_t);
  char *mode            = va_arg(args, char *);
  char *para            = va_arg(args, char *);

  if (mode == NULL) 
  {
    mode = "0";
  }

  sendto_server(client, ":%s SJOIN %1u %s %s :%s", source,
    (unsigned long)ts, target, mode, para);

  return NULL;
}

#if 0
XXX Not used right now
/** Set User or Channelmode
 * not sanity checked!
 * source source of Modechange (server or client)
 * target target of Modechange (channel or client)
 * chptr->mode.mode to set (+ai)
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
  sendto_server(client, "SERVER %s 1 :%s", me.name, me.info);
  send_queued_write(client);

  me.uplink = client;

  /* Send out our list of services loaded */
  DLINK_FOREACH(ptr, services_list.head)
  {
    struct Service *service = ptr->data;

    introduce_client(service->name, service->name, TRUE);
  }

  return NULL;
}

#if 0
XXX Not used ATM
static void
do_user_modes(struct Client *client, const char *modes)
{
  char *ch = (char*)modes;
  int dir = MODE_ADD;

  while(*ch)
  {
    switch(*ch)
    {
      case '+':
        dir = MODE_ADD;
        break;
      case '-':
        dir = MODE_DEL;
        break;
      case 'o':
        if(dir == MODE_ADD)
        {
          SetOper(client);
          ilog(L_DEBUG, "Setting %s as operator(o)\n", client->name);
        }
        else
          ClearOper(client);
        break;
    }
    ch++;
  }
}

#endif

/* part_one_client()
 *
 * inputs - pointer to server
 *    - pointer to source client to remove
 *    - char pointer of name of channel to remove from
 * output - none
 * side effects - remove ONE client given the channel name
 */
static void *
part_one_client(va_list args)
{
  struct Client *client = va_arg(args, struct Client *);
  struct Client *source = va_arg(args, struct Client *);
  char *name = va_arg(args, char *);
  char *reason = va_arg(args, char *);
  struct Channel *chptr = NULL;
  struct Membership *ms = NULL;

  if ((chptr = hash_find_channel(name)) == NULL)
  {
    ilog(L_CRIT, "Trying to part %s from %s which doesnt exist", source->name,
        name);
    return NULL;
  }

  if ((ms = find_channel_link(source, chptr)) == NULL)
  {
    ilog(L_CRIT, "Trying to part %s from %s which they aren't on", source->name,
        chptr->chname);
    return NULL;
  }

  remove_user_from_channel(ms);

  if(MyConnect(source))
  {
    if (reason[0] != '\0')
      sendto_server(client, ":%s PART %s :%s", source->name, chptr->chname, reason);
    else
      sendto_server(client, ":%s PART %s", source->name, chptr->chname);

    execute_callback(on_part_cb, client, source, chptr, reason);
  }

  return NULL;
}

static void
m_ping(struct Client *client, struct Client *source, int parc, char *parv[])
{
  sendto_server(source, ":%s PONG %s :%s", me.name, me.name, source->name);
}

static void
m_server(struct Client *client, struct Client *source, int parc, char *parv[])
{
  if(IsConnecting(client))
  {
    sendto_server(client, "SVINFO 5 5 0: %lu", CurrentTime);
    irc_sendmsg_ping(client, me.name, me.name);
    SetServer(client);
    hash_add_client(client);
    global_notice(NULL, "Completed server connection to %s", source->name);
    ClearConnecting(client);
    client->servptr = &me;
    dlinkAdd(client, &client->lnode, &me.server_list);
  }
  else
  {
    struct Client *newclient = make_client(client);

    strlcpy(newclient->name, parv[1], sizeof(newclient->name));
    strlcpy(newclient->info, parv[3], sizeof(newclient->info));
    newclient->hopcount = atoi(parv[2]);
    SetServer(newclient);
    dlinkAdd(newclient, &newclient->node, &global_client_list);
    hash_add_client(newclient);
    newclient->servptr = source;
    dlinkAdd(newclient, &newclient->lnode, &newclient->servptr->server_list);
    ilog(L_DEBUG, "Got server %s from hub %s", parv[1], source->name);
  }
}

static void
m_privmsg(struct Client *client, struct Client *source, int parc, char *parv[])
{
  process_privmsg(1, client, source, parc, parv);
}

static void
m_notice(struct Client *client, struct Client *source, int parc, char *parv[])
{
  process_privmsg(0, client, source, parc, parv);
}

static void
m_sjoin(struct Client *client, struct Client *source, int parc, char *parv[])
{
  struct Channel *chptr;
  struct Client  *target;
  struct Mode    mode;
  time_t         newts;
  int            args = 0;
  char           have_many_nicks = NO;
  int            isnew = 0;
  unsigned       int fl;
  char           *s;
  char           *p; /* pointer used making sjbuf */

  if (IsClient(source) || parc < 5)
    return;

  /* SJOIN's for local channels can't happen. */
  if (*parv[2] != '#')
    return;

  modebuf[0] = '\0';
  mbuf = modebuf;
  pargs = 0;
  newts = atol(parv[1]);
  s = parv[3];

  memset(&mode, 0, sizeof(mode));

  while (*s)
  {
    switch (*(s++))
    {
      case 't':
        mode.mode |= MODE_TOPICLIMIT;
        break;
      case 'n':
        mode.mode |= MODE_NOPRIVMSGS;
        break;
      case 's':
        mode.mode |= MODE_SECRET;
        break;
      case 'm':
        mode.mode |= MODE_MODERATED;
        break;
      case 'i':
        mode.mode |= MODE_INVITEONLY;
        break;
      case 'p':
        mode.mode |= MODE_PARANOID;
        break;
      case 'c':
        mode.mode |= MODE_NOCOLOR;
        break;
      case 'k':
        strlcpy(mode.key, parv[4 + args], sizeof(mode.key));
        args++;
        if (parc < 5+args)
          return;
        break;
      case 'l':
        mode.limit = atoi(parv[4 + args]);
        args++;
        if (parc < 5+args)
          return;
        break;
    }
  }

  if ((chptr = hash_find_channel(parv[2])) == NULL)
  {
    isnew = 1;
    chptr = make_channel(parv[2]);
    ilog(L_DEBUG, "Created channel %s", parv[2]);
  } 

  chptr->mode = mode;

  parabuf[0] = '\0';

  chptr->channelts = newts;

  mbuf = modebuf;
  sendbuf[0] = '\0';
  pargs = 0;

  *mbuf++ = '+';

  s = parv[args + 4];
  while (*s == ' ')
    s++;
  if ((p = strchr(s, ' ')) != NULL)
  {
    *p++ = '\0';
    while (*p == ' ')
      p++;
    have_many_nicks = *p;
  }

  while (*s)
  {
    int valid_mode = YES;
    fl = 0;

    do
    {
      switch (*s)
      {
        case '@':
          fl |= CHFL_CHANOP;
          s++;
          break;
#ifdef HALFOPS
        case '%':
          fl |= CHFL_HALFOP;
          s++;
          break;
#endif
        case '+':
          fl |= CHFL_VOICE;
          s++;
          break;
        default:
          valid_mode = NO;
          break;
      }
    } while (valid_mode);

    target = find_chasing(source, s, NULL);

    /*
     * if the client doesnt exist, or if its fake direction/server, skip.
     * we cannot send ERR_NOSUCHNICK here because if its a UID, we cannot
     * lookup the nick, and its better to never send the numeric than only
     * sometimes.
     */
    if (target == NULL ||
        target->from != client ||
        !IsClient(target))
    {
      goto nextnick;
    }

    if (!IsMember(target, chptr))
    {
      add_user_to_channel(chptr, target, fl, !have_many_nicks);
      execute_callback(on_join_cb, target, chptr->chname);
      ilog(L_DEBUG, "Added %s!%s@%s to %s", target->name, target->username,
          target->host, chptr->chname);
    }

    if (fl & CHFL_CHANOP)
    {
      para[pargs++] = target->name;

      if (pargs >= MAXMODEPARAMS)
        pargs = 0;
    }
#ifdef HALFOPS
    if (fl & CHFL_HALFOP)
    {
      para[pargs++] = target->name;

      if (pargs >= MAXMODEPARAMS)
        pargs = 0;
    }
#endif
    if (fl & CHFL_VOICE)
    {
      para[pargs++] = target->name;

      if (pargs >= MAXMODEPARAMS)
        pargs = 0;
    }

nextnick:
    if ((s = p) == NULL)
      break;
    while (*s == ' ')
      s++;
    if ((p = strchr(s, ' ')) != NULL)
    {
      *p++ = 0;
      while (*p == ' ')
        p++;
    }
  }

  *mbuf = '\0';

  /* If this happens, its the result of a malformed SJOIN
   * a remnant from the old persistent channel code. *sigh*
   * Or it could be the result of a client just leaving
   * and leaving us with a channel formed just as the client parts.
   * - Dianora
   */

  if(isnew)
    execute_callback(on_channel_created_cb, chptr);

  if ((dlink_list_length(&chptr->members) == 0) && isnew)
  {
    destroy_channel(chptr);
    return;
  }

  if (parv[4 + args][0] == '\0')
    return;
}

/*
 * ms_nick()
 *
 * server -> server nick change
 *    parv[0] = sender prefix
 *    parv[1] = nickname
 *    parv[2] = TS when nick change
 *
 * server introducing new nick
 *    parv[0] = sender prefix
 *    parv[1] = nickname
 *    parv[2] = hop count
 *    parv[3] = TS
 *    parv[4] = umode
 *    parv[5] = username
 *    parv[6] = hostname
 *    parv[7] = server
 *    parv[8] = ircname
 */
static void
m_nick(struct Client *client_p, struct Client *source_p,
        int parc, char *parv[])
{
  struct Client* target_p;
  char nick[NICKLEN];
  char ngecos[REALLEN + 1];
  time_t newts = 0;
  char *nnick = parv[1];
  char *nhop = parv[2];
  char *nts = parv[3];
  char *nusername = parv[5];
  char *nhost = parv[6];
  char *nserver = parv[7];

  if (parc < 2 || EmptyString(nnick))
    return;

  /* fix the lengths */
  strlcpy(nick, nnick, sizeof(nick));

  if (parc == 9)
  {
    struct Client *server_p = find_server(nserver);

    strlcpy(ngecos, parv[8], sizeof(ngecos));

    if (server_p == NULL)
    {
      global_notice(NULL, "Invalid server %s from %s for NICK %s",
          nserver, source_p->name, nick);
      return;
    }

    if (check_clean_nick(client_p, source_p, nick, nnick, server_p) ||
        check_clean_user(client_p, nick, nusername, server_p) ||
        check_clean_host(client_p, nick, nhost, server_p))
      return;

    /* check the length of the clients gecos */
    if (strlen(parv[8]) > REALLEN)
      global_notice(NULL, "Long realname from server %s for %s", nserver, nnick);

    if (IsServer(source_p))
      newts = atol(nts);
  }
  else if (parc == 3)
  {
    if (IsServer(source_p))
      /* Server's cant change nicks.. */
      return;

    if (check_clean_nick(client_p, source_p, nick, nnick, source_p->servptr))
      return;

    /*
     * Yes, this is right. HOP field is the TS field for parc = 3
     */
    newts = atol(nhop);
  }

  /* if the nick doesnt exist, allow it and process like normal */
  if (!(target_p = find_client(nick)))
  {
    nick_from_server(client_p, source_p, parc, parv, newts, nick, ngecos);
    return;
  }

  if (target_p == source_p)
  {
    if (strcmp(target_p->name, nick))
    {
      /* client changing case of nick */
      nick_from_server(client_p, source_p, parc, parv, newts, nick, ngecos);
      return;
    }
    else
      /* client not changing nicks at all */
      return;
  }
}

static void 
m_part(struct Client *client, struct Client *source, int parc, char *parv[])
{
  char *p, *name;
  char reason[KICKLEN + 1];

  reason[0] = '\0';

  if(parc > 2)
    strlcpy(reason, parv[2], sizeof(reason));

  name = strtoken(&p, parv[1], ",");

  while (name)
  {
    execute_callback(on_part_cb, client, source, hash_find_channel(name), 
        reason);
    execute_callback(send_part_cb, client, source, name, reason);
    name = strtoken(&p, NULL, ",");
  }
}

static void
m_kick(struct Client *client, struct Client *source, int parc, char *parv[])
{
  struct Client *target;
  char *reason;

  reason = (EmptyString(parv[3])) ? parv[2] : parv[3];

  target = find_person(source, parv[2]);
  if(target == NULL)
    return;

  execute_callback(send_part_cb, client, target, parv[1], reason);
}

static void
m_quit(struct Client *client, struct Client *source, int parc, char *parv[])
{
  char *comment = (parc > 1 && parv[1]) ? parv[1] : client->name;

  if (strlen(comment) > (size_t)KICKLEN)
    comment[KICKLEN] = '\0';

  exit_client(source, source, comment);
}

static void
m_kill(struct Client *client, struct Client *source, int parc, char *parv[])
{
  struct Client *target = find_person(source, parv[1]);
  char *comment;
  
  if(target == NULL)
    return;
  
  comment = (parc > 1 && parv[2]) ? parv[2] : target->name;

  if (strlen(comment) > (size_t)KICKLEN)
    comment[KICKLEN] = '\0';

  exit_client(target, source, comment);
}

static void
m_squit(struct Client *client, struct Client *source, int parc, char *parv[])
{
  struct Client *target = NULL;
  char *comment;
  const char *server;
  char def_reason[] = "No reason";

  server = parv[1];

  if ((target = find_server(server)) == NULL)
    return;

  if (!IsServer(target) || IsMe(target))
    return;

  comment = (parc > 2 && parv[2]) ? parv[2] : def_reason;

  if (strlen(comment) > (size_t)REASONLEN)
    comment[REASONLEN] = '\0';

  exit_client(target, source, comment);
//  chain_squit(client, source, comment);

  if(target == me.uplink)
    me.uplink = NULL;
}

/*
 * m_mode - MODE command handler
 * parv[0] - sender
 * parv[1] - channel
 */
static void
m_mode(struct Client *client_p, struct Client *source_p, int parc, char *parv[])
{
  struct Channel *chptr = NULL;
  struct Membership *member = NULL;

  if (*parv[1] == '\0')
  {
    return;
  }

  /* Now, try to find the channel in question */
  if (!IsChanPrefix(*parv[1]))
  {
    /* if here, it has to be a non-channel name */
    set_user_mode(client_p, source_p, parc, parv);
    ilog(L_DEBUG, "%s %s", client_p->name, source_p->name);
    return;
  }

  if ((chptr = hash_find_channel(parv[1])) == NULL)
  {
    global_notice(NULL, "Mode for unknown channel %s", parv[1]);
    return;
  }

  /* Now known the channel exists */
  if (parc < 3)
  {
    return;
  }

  /* bounce all modes from people we deop on sjoin
   * servers have always gotten away with murder,
   * including telnet servers *g* - Dianora
   *
   * XXX Is it worth the bother to make an ms_mode() ? - Dianora
   */
  else if (IsServer(source_p))
  {
    set_channel_mode(client_p, source_p, chptr, NULL, parc - 2, parv + 2,
                     chptr->chname);
  }
  else
  {
    member = find_channel_link(source_p, chptr);

    if (!has_member_flags(member, CHFL_DEOPPED))
    {
      /* Finish the flood grace period... */
      set_channel_mode(client_p, source_p, chptr, member, parc - 2, parv + 2,
                       chptr->chname);
    }
  }
}

static void
m_topic(struct Client *client_p, struct Client *source_p, 
    int parc, char *parv[])
{
  struct Channel *chptr;
  char topic_info[USERHOST_REPLYLEN];

  if((chptr = hash_find_channel(parv[1])) == NULL)
  {
    ilog(L_ERROR, "Got Topic for channel %s which we know nothing about.",
        parv[1]);
    return;
  }
  
  ircsprintf(topic_info, "%s!%s@%s", source_p->name, source_p->username,
      source_p->host);

  set_channel_topic(chptr, parv[2], topic_info, CurrentTime);
}

static void
m_version(struct Client *client_p, struct Client *source_p,
    int parc, char *parv[])
{
  struct Client *target = NULL;

  if((target = find_server(parv[1])) == NULL)
    return;

  if(IsMe(target))
  {
    sendto_server(me.uplink, "351 %s oftc-ircservices-%s %s", source_p->name,
        VERSION, me.name);
    return;
  }

  execute_callback(send_nosuchsrv_cb, source_p->name, parv[1]);
}

static void
m_stats(struct Client *client_p, struct Client *source_p,
  int parc, char *parv[])
{
  struct Client *target = NULL;

  if((target = find_server(parv[2])) == NULL)
    return;


  if(IsMe(target) && *parv[1] == 'z' && IsOper(source_p))
  {
    dlink_list *usage = block_heap_get_usage();
    dlink_node *ptr = NULL, *next_ptr = NULL;
    struct BlockHeapInfo *bi = NULL;

    DLINK_FOREACH_SAFE(ptr, next_ptr, usage->head)
    {
      bi = (struct BlockHeapInfo *)ptr->data;
      sendto_server(me.uplink,
        ":%s %d %s z :%s mempool: used %u/%u free %u/%u (size %u/%u)",
        me.name, 249, source_p->name, bi->name, bi->used_elm, bi->used_mem,
        bi->free_elm, bi->free_mem, bi->size_elm, bi->size_mem);
      dlinkDelete(ptr, usage);
      MyFree(bi->name);
      bi->name = NULL;
      MyFree(bi);
    }
    MyFree(usage);
    sendto_server(me.uplink, ":%s 219 %s %c :End of /STATS report",
      me.name, source_p->name, 'z');
  }
  else if(!IsMe(target))
  {
    execute_callback(send_nosuchsrv_cb, source_p->name, parv[2]);
  }
  else
  {
    /* ERR NO PERM */
  }
}

static void
m_tburst(struct Client *client_p, struct Client *source_p, int parc, 
    char *parv[])
{
  struct Channel *chptr = NULL;
  time_t remote_topic_ts = atol(parv[3]);
  const char *topic = "";
  const char *setby = "";

  /*
   * Do NOT test parv[5] for an empty string and return if true!
   * parv[5] CAN be an empty string, i.e. if the other side wants
   * to unset our topic.  Don't forget: an empty topic is also a
   * valid topic.
   */

  if ((chptr = hash_find_channel(parv[2])) == NULL)
    return;

  if (parc == 6)
  {
    topic = parv[5];
    setby = parv[4];
  }

  set_channel_topic(chptr, topic, setby, remote_topic_ts);
}

static void *
irc_sendmsg_nosuchsrv(va_list args)
{
  char *source = va_arg(args, char *);
  char *server = va_arg(args, char *);

  sendto_server(me.uplink, ":%s 402 %s %s :No such server", me.name, source,
    server);

  return NULL;
}
