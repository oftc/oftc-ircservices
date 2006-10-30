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
 *  $Id: $
 */

#include "stdinc.h"

static struct Service *chanserv = NULL;

static void m_register(struct Service *, struct Client *, int, char *[]);
static void m_help(struct Service *, struct Client *, int, char *[]);
static void m_set(struct Service *, struct Client *, int, char *[]);

static struct ServiceMessage register_msgtab = {
  NULL, "REGISTER", 0, 2, CS_HELP_REG_SHORT, CS_HELP_REG_LONG,
  { m_register, m_alreadyreg, m_alreadyreg, m_alreadyreg }
};

static struct ServiceMessage help_msgtab = {
  NULL, "HELP", 0, 0, CS_HELP_SHORT, CS_HELP_LONG,
  { m_help, m_help, m_help, m_help }
};

INIT_MODULE(chanserv, "$Revision: 470 $")
{
  chanserv = make_service("ChanServ");
  clear_serv_tree_parse(&chanserv->msg_tree);
  dlinkAdd(chanserv, &chanserv->node, &services_list);
  hash_add_service(chanserv);
  introduce_service(chanserv);
  load_language(chanserv, "chanserv.en");
  load_language(chanserv, "chanserv.rude");
  load_language(chanserv, "chanserv.de");

  mod_add_servcmd(&chanserv->msg_tree, &register_msgtab);
  mod_add_servcmd(&chanserv->msg_tree, &help_msgtab);
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
  struct RegChannel *chan;

  if (client->nickname == NULL)
  {
    reply_user(service, client, _L(chanserv, client, CS_REG_NS_FIRST));
  }

  if ( *parv[1] != '#' )
  {
    reply_user(service, client, _L(chanserv, client, CS_NAMESTART_HASH));
  }

  chan = db_find_chan(parv[1]);
  if (chan != NULL)
  {
    reply_user(service, client, _L(chanserv, client, CS_ALREADY_REG), parv[1]);
    // XXX free chan here please.
    return;
  }

  if (db_register_chan(client, parv[1]))
  {
    // XXX attach RegChannel to Channel
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
m_help(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  do_help(service, client, parv[1], parc, parv);
}

static void
m_set(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  reply_user(service, client, "Unknown SET option");
}

