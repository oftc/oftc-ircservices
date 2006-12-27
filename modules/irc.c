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

static void m_ping(struct Client *, struct Client *, int, char *[]);
static void m_nick(struct Client *, struct Client *, int, char*[]);
static void m_server(struct Client *, struct Client *, int, char*[]);
static void m_sjoin(struct Client *, struct Client *, int, char*[]);
static void m_part(struct Client *, struct Client *, int, char*[]);
static void m_quit(struct Client *, struct Client *, int, char*[]);
static void m_squit(struct Client *, struct Client *, int, char*[]);
static void m_mode(struct Client *, struct Client *, int, char*[]);
static void m_topic(struct Client *, struct Client *, int, char*[]);

//static void do_user_modes(struct Client *client, const char *modes);
static void set_final_mode(struct Mode *, struct Mode *);
static void remove_our_modes(struct Channel *, struct Client *);
static void remove_a_mode(struct Channel *, struct Client *, int, char);

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
static void *irc_server_connected(va_list);
static char modebuf[MODEBUFLEN];
static char parabuf[MODEBUFLEN];
static char sendbuf[MODEBUFLEN];
static const char *para[MAXMODEPARAMS];
static char *mbuf;
static int pargs;

struct Message away_msgtab = {
  "AWAY", 0, 0, 0, 0, 0, 0,
  { m_ignore, m_ignore }
};

struct Message admin_msgtab = {
  "ADMIN", 0, 0, 0, 0, 0, 0,
  { m_ignore, m_ignore }
};

struct Message version_msgtab = {
  "VERSION", 0, 0, 0, 0, 0, 0,
  { m_ignore, m_ignore }
};

struct Message trace_msgtab = {
  "TRACE", 0, 0, 0, 0, 0, 0,
  { m_ignore, m_ignore }
};

struct Message stats_msgtab = {
  "STATS", 0, 0, 0, 0, 0, 0,
  { m_ignore, m_ignore }
};

struct Message whois_msgtab = {
  "WHOIS", 0, 0, 0, 0, 0, 0,
  { m_ignore, m_ignore }
};

struct Message whowas_msgtab = {
  "WHOWAS", 0, 0, 0 ,0, 0, 0,
  { m_ignore, m_ignore }
};

struct Message invite_msgtab = {
  "INVITE", 0, 0, 0, 0, 0, 0,
  { m_ignore, m_ignore }
};

struct Message lusers_msgtab = {
  "LUSERS", 0, 0, 0, 0, 0, 0,
  { m_ignore, m_ignore }
};

struct Message motd_msgtab = {
  "MOTD", 0, 0, 0, 0, 0, 0,
  { m_ignore, m_ignore }
};

struct Message part_msgtab = {
  "PART", 0, 0, 2, 0, 0, 0,
  { m_part, m_ignore }
};

struct Message ping_msgtab = {
  "PING", 0, 0, 1, 0, 0, 0,
  { m_ping, m_ignore }
};

struct Message pong_msgtab = {
  "PONG", 0, 0, 1, 0, 0, 0,
  { m_ignore, m_ignore }
};

struct Message eob_msgtab = {
  "EOB", 0, 0, 0, 0, 0, 0,
  { m_ignore, m_ignore }
};

struct Message server_msgtab = {
  "SERVER", 0, 0, 3, 0, 0, 0,
  { m_server, m_ignore }
};

struct Message nick_msgtab = {
  "NICK", 0, 0, 1, 0, 0, 0,
  { m_nick, m_ignore }
};

struct Message sjoin_msgtab = {
  "SJOIN", 0, 0, 4, 0, 0, 0,
  { m_sjoin, m_ignore }
};

struct Message quit_msgtab = {
  "QUIT", 0, 0, 0, 0, 0, 0,
  { m_quit, m_ignore }
};

struct Message squit_msgtab = {
  "SQUIT", 0, 0, 1, 0, 0, 0,
  { m_squit, m_ignore }
};

struct Message mode_msgtab = {
  "MODE", 0, 0, 2, 0, 0, 0,
  { m_mode, m_ignore }
};

struct Message topic_msgtab = {
  "TOPIC", 0, 0, 2, 0, 0, 0, 
  { m_topic, m_ignore }
};

struct Message privmsg_msgtab = {
  "PRIVMSG", 0, 0, 2, 0, 0, 0,
  { process_privmsg, m_ignore }
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
  
  ilog(L_DEBUG, "MODE %s %s %s going out.", channel, mode, param);
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

  sendto_server(uplink, ":%s KLINE * 0 %s %s :autokilled: %s(Set by %s)", 
      (source != NULL) ? source->name : me.name, user, host, reason, setter);

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

  sendto_server(uplink, ":%s TBURST 1 %s %lu %s :%s", me.name, chptr->chname,
      CurrentTime, setter->name, topic);

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

#if 0
XXX Not used right now
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

    introduce_client(service->name);
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

/* set_final_mode
 *
 * inputs - channel mode
 *    - old channel mode
 * output - NONE
 * side effects - walk through all the channel modes turning off modes
 *      that were on in oldmode but aren't on in mode.
 *      Then walk through turning on modes that are on in mode
 *      but were not set in oldmode.
 */

static void
set_final_mode(struct Mode *mode, struct Mode *oldmode)
{
  char *pbuf = parabuf;
  int len;
  int i;

  *mbuf++ = '-';

  for (i = 0; ModeList[i].letter; i++)
  {
    if ((ModeList[i].mode & oldmode->mode) &&
        !(ModeList[i].mode & mode->mode))
      *mbuf++ = ModeList[i].letter;
  }

  if (oldmode->limit != 0 && mode->limit == 0)
    *mbuf++ = 'l';

  if (oldmode->key[0] && !mode->key[0])
  {
    *mbuf++ = 'k';
    len = ircsprintf(pbuf, "%s ", oldmode->key);
    pbuf += len;
    if ((ModeList[i].mode & mode->mode) &&
        !(ModeList[i].mode & oldmode->mode))
      *mbuf++ = ModeList[i].letter;
  }

  if (mode->limit != 0 && oldmode->limit != mode->limit)
  {
    *mbuf++ = 'l';
    len = ircsprintf(pbuf, "%d ", mode->limit);
    pbuf += len;
    pargs++;
  }

  if (mode->key[0] && strcmp(oldmode->key, mode->key))
  {
    *mbuf++ = 'k';
    len = ircsprintf(pbuf, "%s ", mode->key);
    pbuf += len;
    pargs++;
  }
  if (*(mbuf-1) == '+')
    *(mbuf-1) = '\0';
  else
    *mbuf = '\0';
}

/* remove_our_modes()
 *
 * inputs - pointer to channel to remove modes from
 *    - client pointer
 * output - NONE
 * side effects - Go through the local members, remove all their
 *      chanop modes etc., this side lost the TS.
 */
static void
remove_our_modes(struct Channel *chptr, struct Client *source)
{
  remove_a_mode(chptr, source, CHFL_CHANOP, 'o');
#ifdef HALFOPS
  remove_a_mode(chptr, source, CHFL_HALFOP, 'h');
#endif
  remove_a_mode(chptr, source, CHFL_VOICE, 'v');
}
/* remove_a_mode()
 *
 * inputs - pointer to channel
 *    - server or client removing the mode
 *    - mask o/h/v mask to be removed
 *    - flag o/h/v to be removed
 * output - NONE
 * side effects - remove ONE mode from all members of a channel
 */
static void
remove_a_mode(struct Channel *chptr, struct Client *source,
             int mask, char flag)
{
  dlink_node *ptr;
  struct Membership *ms;
  char lmodebuf[MODEBUFLEN];
  char *sp=sendbuf;
  const char *lpara[MAXMODEPARAMS];
  int count = 0;
  int i;
  int l;

  mbuf = lmodebuf;
  *mbuf++ = '-';
  *sp = '\0';

  DLINK_FOREACH(ptr, chptr->members.head)
  {
    ms = ptr->data;

    if ((ms->flags & mask) == 0)
      continue;

    ms->flags &= ~mask;

    lpara[count++] = ms->client_p->name;

    *mbuf++ = flag;

    if (count >= MAXMODEPARAMS)
    {
      for(i = 0; i < MAXMODEPARAMS; i++)
      {
        l = ircsprintf(sp, " %s", lpara[i]);
        sp += l;
      }
      *mbuf = '\0';
    }
  }
}

/* part_one_client()
 *
 * inputs - pointer to server
 *    - pointer to source client to remove
 *    - char pointer of name of channel to remove from
 * output - none
 * side effects - remove ONE client given the channel name
 */
static void
part_one_client(struct Client *client, struct Client *source, char *name)
{
  struct Channel *chptr = NULL;
  struct Membership *ms = NULL;

  if ((chptr = hash_find_channel(name)) == NULL)
  {
    global_notice(NULL, "Trying to part %s from %s which doesnt exist", source->name,
        name);
    return;
  }

  if ((ms = find_channel_link(source, chptr)) == NULL)
  {
    global_notice(NULL, "Trying to part %s from %s which they aren't on", source->name,
        chptr->chname);
    return;
  }

  remove_user_from_channel(ms);
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
m_sjoin(struct Client *client, struct Client *source, int parc, char *parv[])
{
  struct Channel *chptr;
  struct Client  *target;
  time_t         newts;
  time_t         oldts;
  time_t         tstosend;
  struct Mode mode, *oldmode;
  int            args = 0;
  char           keep_our_modes = YES;
  char           keep_new_modes = YES;
  char           have_many_nicks = NO;
  int            lcount;
  char           nick_prefix[4];
  char           *np;
  int            len_nick = 0;
  int            isnew = 0;
  int            buflen = 0;
  int          slen;
  unsigned       int fl;
  char           *s;
  char     *sptr;
  char nick_buf[IRC_BUFSIZE]; /* buffer for modes and prefix */
  char           *nick_ptr; 
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

  mode.mode = 0;
  mode.limit = 0;
  mode.key[0] = '\0';
  s = parv[3];

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

  parabuf[0] = '\0';

  if ((chptr = hash_find_channel(parv[2])) == NULL)
  {
    isnew = 1;
    chptr = make_channel(parv[2]);
   // XXX what were you thinking?! chptr->regchan = db_find_chan(parv[2]); // the result doesnt matter here -mc
    ilog(L_DEBUG, "Created channel %s", parv[2]);
  }

  oldts   = chptr->channelts;
  oldmode = &chptr->mode;

  if (newts < 800000000)
  {

    newts = (oldts == 0) ? 0 : 800000000;
  }

  if (isnew)
    chptr->channelts = tstosend = newts;
  else if (newts == 0 || oldts == 0)
    chptr->channelts = tstosend = 0;
  else if (newts == oldts)
    tstosend = oldts;
  else if (newts < oldts)
  {
    keep_our_modes = NO;
    chptr->channelts = tstosend = newts;
  }
  else
  {
    keep_new_modes = NO;
    tstosend = oldts;
  }

  if (!keep_new_modes)
    mode = *oldmode;
  else if (keep_our_modes)
  {
    mode.mode |= oldmode->mode;
    if (oldmode->limit > mode.limit)
      mode.limit = oldmode->limit;
    if (strcmp(mode.key, oldmode->key) < 0)
      strcpy(mode.key, oldmode->key);
  }

  set_final_mode(&mode, oldmode);
  chptr->mode = mode;

  /* Lost the TS, other side wins, so remove modes on this side */
  if (!keep_our_modes)
  {
    remove_our_modes(chptr, source);
  }

  if (parv[3][0] != '0' && keep_new_modes)
  {
    channel_modes(chptr, source, modebuf, parabuf);
  }
  else
  {
    modebuf[0] = '0';
    modebuf[1] = '\0';
  }

  buflen = ircsprintf(nick_buf, ":%s SJOIN %lu %s %s %s:",
      source->name, (unsigned long)tstosend,
      chptr->chname, modebuf, parabuf);
  nick_ptr = nick_buf + buflen;

  /* check we can fit a nick on the end, as well as \r\n and a prefix "
   * @%+", and a space.
   */
  if (buflen >= (IRC_BUFSIZE - LIBIO_MAX(NICKLEN, IDLEN) - 2 - 3 - 1))
  {
    return;
  }

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

    len_nick = strlen(target->name);

    np = nick_prefix;

    if (keep_new_modes)
    {
      if (fl & CHFL_CHANOP)
      {
        *np++ = '@';
        len_nick++;
      }
#ifdef HALFOPS
      if (fl & CHFL_HALFOP)
      {
        *np++ = '%';
        len_nick++;
      }
#endif
      if (fl & CHFL_VOICE)
      {
        *np++ = '+';
        len_nick++;
      }
    }
    else
    {
      if (fl & (CHFL_CHANOP|CHFL_HALFOP))
        fl = CHFL_DEOPPED;
      else
        fl = 0;
    }

    *np = '\0';

    if ((nick_ptr - nick_buf + len_nick) > (IRC_BUFSIZE  - 2))
    {
      sendto_server(client, "%s", nick_buf);

      buflen = ircsprintf(nick_buf, ":%s SJOIN %lu %s %s %s:",
          source->name, (unsigned long)tstosend,
          chptr->chname, modebuf, parabuf);
      nick_ptr = nick_buf + buflen;
    }

    nick_ptr += ircsprintf(nick_ptr, "%s%s ", nick_prefix, target->name);

    if (!IsMember(target, chptr))
    {
      add_user_to_channel(chptr, target, fl, !have_many_nicks);
      chain_join(target, chptr->chname);
      ilog(L_DEBUG, "Added %s!%s@%s to %s", target->name, target->username,
          target->host, chptr->chname);
    }

    if (fl & CHFL_CHANOP)
    {
      *mbuf++ = 'o';
      para[pargs++] = target->name;

      if (pargs >= MAXMODEPARAMS)
      {
        /*
         * Ok, the code is now going to "walk" through
         * sendbuf, filling in para strings. So, I will use sptr
         * to point into the sendbuf.
         * Notice, that ircsprintf() returns the number of chars
         * successfully inserted into string.
         * - Dianora
         */

        sptr = sendbuf;
        *mbuf = '\0';
        for(lcount = 0; lcount < MAXMODEPARAMS; lcount++)
        {
          slen = ircsprintf(sptr, " %s", para[lcount]); /* see? */
          sptr += slen;         /* ready for next */
        }
        mbuf = modebuf;
        *mbuf++ = '+';

        sendbuf[0] = '\0';
        pargs = 0;
      }
    }
#ifdef HALFOPS
    if (fl & CHFL_HALFOP)
    {
      *mbuf++ = 'h';
      para[pargs++] = target->name;

      if (pargs >= MAXMODEPARAMS)
      {
        sptr = sendbuf;
        *mbuf = '\0';
        for(lcount = 0; lcount < MAXMODEPARAMS; lcount++)
        {
          slen = ircsprintf(sptr, " %s", para[lcount]);
          sptr += slen;
        }

        mbuf = modebuf;
        *mbuf++ = '+';

        sendbuf[0] = '\0';
        pargs = 0;
      }
    }
#endif
    if (fl & CHFL_VOICE)
    {
      *mbuf++ = 'v';
      para[pargs++] = target->name;

      if (pargs >= MAXMODEPARAMS)
      {
        sptr = sendbuf;
        *mbuf = '\0';
        for (lcount = 0; lcount < MAXMODEPARAMS; lcount++)
        {
          slen = ircsprintf(sptr, " %s", para[lcount]);
          sptr += slen;
        }

        mbuf = modebuf;
        *mbuf++ = '+';

        sendbuf[0] = '\0';
        pargs = 0;
      }
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
  *(nick_ptr - 1) = '\0';

  /*
   * checking for lcount < MAXMODEPARAMS at this time is wrong
   * since the code has already verified above that pargs < MAXMODEPARAMS
   * checking for para[lcount] != '\0' is also wrong, since
   * there is no place where para[lcount] is set!
   * - Dianora
   */

  if (pargs != 0)
  {
    sptr = sendbuf;

    for (lcount = 0; lcount < pargs; lcount++)
    {
      slen = ircsprintf(sptr, " %s", para[lcount]);
      sptr += slen;
    }

  }

  /* If this happens, its the result of a malformed SJOIN
   * a remnant from the old persistent channel code. *sigh*
   * Or it could be the result of a client just leaving
   * and leaving us with a channel formed just as the client parts.
   * - Dianora
   */

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

  name = strtoken(&p, parv[1], ",");
    
  while (name)
  {
    part_one_client(client, source, name);
    chain_part(client, source, name);
    name = strtoken(&p, NULL, ",");
  }
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
  chain_squit(client, source, comment);
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
