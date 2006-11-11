/*
 *  oftc-ircservices: an exstensible and flexible IRC Services package
 *  chanserv.c: A C implementation of Chanell Services
 *
 *  Copyright (C) 2006 The OFTC Coding department
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

static struct Service *chanserv = NULL;

static dlink_node *cs_cmode_hook;
static dlink_node *cs_join_hook;

static void *cs_on_cmode_change(va_list);
static void *cs_on_client_join(va_list);

static void m_register(struct Service *, struct Client *, int, char *[]);
static void m_help(struct Service *, struct Client *, int, char *[]);
static void m_drop(struct Service *, struct Client *, int, char *[]);

/* temp */
static void m_not_avail(struct Service *, struct Client *, int, char *[]);

static struct ServiceMessage register_msgtab = {
  NULL, "REGISTER", 0, 2, CS_HELP_REG_SHORT, CS_HELP_REG_LONG,
  { m_unreg, m_register, m_register, m_register }
};

static struct ServiceMessage help_msgtab = {
  NULL, "HELP", 0, 0, CS_HELP_SHORT, CS_HELP_LONG,
  { m_help, m_help, m_help, m_help }
};

/* 
 * contrary to old services:  /msg chanserv ACCESS #channel ADD foo bar baz
 * we do this:                /msg chanserv ACCESS ADD #channel foo bar baz
 * this is not nice, but fits our structure better
 */
static struct SubMessage set_sub[] = {
  { "FOUNDER",     0, 1, -1, -1, m_not_avail },
  { "SUCCESSOR",   0, 1, -1, -1, m_not_avail },
  { "PASSWORD",    0, 1, -1, -1, m_not_avail },
  { "DESC",        0, 1, -1, -1, m_not_avail },
  { "URL",         0, 1, -1, -1, m_not_avail },
  { "EMAIL",       0, 1, -1, -1, m_not_avail },
  { "ENTRYMSG",    0, 1, -1, -1, m_not_avail },
  { "TOPIC",       0, 1, -1, -1, m_not_avail },
  { "KEEPTOPIC",   0, 1, -1, -1, m_not_avail },
  { "TOPICLOCK",   0, 1, -1, -1, m_not_avail },
  { "MLOCK",       0, 1, -1, -1, m_not_avail },
  { "PRIVATE",     0, 1, -1, -1, m_not_avail },
  { "RESTRICTED",  0, 1, -1, -1, m_not_avail },
  { "SECURE",      0, 1, -1, -1, m_not_avail },
  { "SECUREOPS",   0, 1, -1, -1, m_not_avail },
  { "LEAVEOPS",    0, 1, -1, -1, m_not_avail },
  { "VERBOSE",     0, 1, -1, -1, m_not_avail },
  { "AUTOLIMIT",   0, 1, -1, -1, m_not_avail },
  { "CLEARBANS",   0, 1, -1, -1, m_not_avail },
  { "NULL",        0, 0,  0,  0, m_not_avail }
};

static struct ServiceMessage set_msgtab = {
  set_sub, "SET", 0, 0, CS_SET_SHORT, CS_SET_LONG,
  { m_unreg, m_not_avail, m_not_avail, m_not_avail }
};

static struct SubMessage access_sub[6] = {
  { "ADD",   0, 3, -1, -1, m_not_avail },
  { "DEL",   0, 2, -1, -1, m_not_avail },
  { "LIST",  0, 2, -1, -1, m_not_avail },
  { "VIEW",  0, 1, -1, -1, m_not_avail },
  { "COUNT", 0, 0, -1, -1, m_not_avail },
  { "NULL",  0, 0,  0,  0, NULL }
};

static struct ServiceMessage access_msgtab = {
  access_sub, "ACCESS", 0, 0, -1, -1, 
  { m_unreg, m_not_avail, m_not_avail, m_not_avail }
};

static struct SubMessage levels_sub[6] = {
  { "SET",      0, 3, -1, -1, m_not_avail },
  { "LIST",     0, 0, -1, -1, m_not_avail },
  { "RESET",    0, 0, -1, -1, m_not_avail },
  { "DIS",      0, 1, -1, -1, m_not_avail },
  { "DISABLED", 0, 1, -1, -1, m_not_avail },
  { "NULL",     0, 0,  0,  0, NULL }
};

static struct ServiceMessage levels_msgtab = {
  levels_sub, "LEVELS", 0, 0, -1, -1,
  { m_unreg, m_not_avail, m_not_avail, m_not_avail }
};

static struct SubMessage akick_sub[7] = {
  { "ADD",     0, 2, -1, -1, m_not_avail }, 
  { "DEL",     0, 1, -1, -1, m_not_avail },
  { "LIST",    0, 1, -1, -1, m_not_avail },
  { "VIEW",    0, 1, -1, -1, m_not_avail },
  { "ENFORCE", 0, 0, -1, -1, m_not_avail },
  { "COUNT",   0, 0, -1, -1, m_not_avail },
  { "NULL",    0, 0,  0,  0, NULL }
};

static struct ServiceMessage akick_msgtab = {
  akick_sub, "AKICK", 0, 0, -1, -1,
  { m_unreg, m_not_avail, m_not_avail, m_not_avail }
};

static struct ServiceMessage drop_msgtab = {
  NULL, "DROP", 0, 1, -1, -1,
  { m_unreg, m_drop, m_drop, m_drop }
};

static struct ServiceMessage identify_msgtab = {
  NULL, "IDENTIFY", 0, 1, -1, -1,
  { m_unreg, m_not_avail, m_not_avail, m_not_avail }
};


/*
INFO
LIST
INVITE
OP
DEOP
UNBAN
CLEAR
VOP
AOP
SOP
SYNC
SENDPASS
*/

INIT_MODULE(chanserv, "$Revision$")
{
  chanserv = make_service("ChanServ");
  clear_serv_tree_parse(&chanserv->msg_tree);
  dlinkAdd(chanserv, &chanserv->node, &services_list);
  hash_add_service(chanserv);
/*  introduce_service(chanserv);
  load_language(chanserv, "chanserv.en");
  load_language(chanserv, "chanserv.rude");
  load_language(chanserv, "chanserv.de");
*/
  mod_add_servcmd(&chanserv->msg_tree, &register_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &help_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &set_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &drop_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &akick_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &identify_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &levels_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &access_msgtab);
  cs_cmode_hook = install_hook(on_cmode_change_cb, cs_on_cmode_change);
  cs_join_hook  = install_hook(on_join_cb, cs_on_client_join);
}

CLEANUP_MODULE
{
  exit_client(find_client(chanserv->name), &me, "Service unloaded");
  hash_del_service(chanserv);
  dlinkDelete(&chanserv->node, &services_list);
}

static void 
m_register(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  struct Channel    *chptr;
  struct RegChannel *regchptr;

  // Bail out if channelname does not start with a hash
  if ( *parv[1] != '#' )
  {
    reply_user(service, client, _L(chanserv, client, CS_NAMESTART_HASH));
    return;
  }

  // Bail out if services dont know the channel (it does not exist)
  // or if client is no member of the channel
  chptr = hash_find_channel(parv[1]);
  if ((chptr == NULL) || (! IsMember(client, chptr))) 
  {
    reply_user(service, client, _L(chanserv, client, CS_NOT_ONCHAN));
    return;
  }
  
  // bail out if client is not opped on channel
  if (! IsChanop(client, chptr))
  {
    reply_user(service, client, _L(chanserv, client, CS_NOT_OPPED));
    return;
  }

  // finally, bail out if channel is already registered
  regchptr = db_find_chan(parv[1]);
  if (regchptr != NULL)
  {
    reply_user(service, client, _L(chanserv, client, CS_ALREADY_REG), parv[1]);
    MyFree(regchptr);
    return;
  }

  if (db_register_chan(client, parv[1]) == 0)
  {
    chptr->regchan = db_find_chan(parv[1]);
    reply_user(service, client, _L(chanserv, client, CS_REG_SUCCESS), parv[1]);
    global_notice(NULL, "%s (%s@%s) registered channel %s", 
      client->name, client->username, client->host, parv[1]);
  }
  else
  {
    reply_user(service, client, _L(chanserv, client, CS_REG_FAIL), parv[1]);
  }
}

static void 
m_drop(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  struct RegChannel *regchptr;

  regchptr = db_find_chan(parv[1]);
  if (regchptr == NULL)
  {
    reply_user(service, client, _L(chanserv, client, CS_NOT_REG), parv[1]);
    MyFree(regchptr);
    return;
  }
  
  if (strncmp(regchptr->founder, client->name, NICKLEN+1) != 0)
  {
    reply_user(service, client, _L(chanserv, client, CS_OWN_CHANNEL_ONLY), parv[1]);
    MyFree(regchptr);
    return;
  }

  if (db_delete_chan(parv[1]) == 0)
  {
    reply_user(service, client, _L(chanserv, client, CS_REGISTERED), parv[1]);
  } else
  {
    reply_user(service, client, _L(chanserv, client, CS_REG_FAILED), parv[1]);
  }

  MyFree(regchptr);
  return;
}

/* XXX temp XXX */
static void
m_not_avail(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  reply_user(service, client, "This function is currently not implemented.  Bug the Devs! ;-)");
}


static void
m_help(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  do_help(service, client, parv[1], parc, parv);
}

#if 0
XXX not used atm
static void
m_set(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  reply_user(service, client, "Unknown SET option");
}

#endif

static void *
cs_on_cmode_change(va_list args) 
{
/*  struct Client  *client_p = va_arg(args, struct Client*);
  struct Client  *source_p = va_arg(args, struct Client*);
  struct Channel *chptr    = va_arg(args, struct Channel*);
  int             parc     = va_arg(args, int);
  char           **parv    = va_arg(args, char **);
*/
  // ... actually do stuff    

  // last function to call in this func
  return pass_callback(cs_cmode_hook);
}

static void *
cs_on_client_join(va_list args)
{
  /* struct Client *source_p = va_arg(args, struct Client *);
   * char          *name     = va_arg(args, char *);
   */

  return pass_callback(cs_join_hook);
}
