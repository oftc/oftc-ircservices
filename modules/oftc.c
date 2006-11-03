/*
 *  oftc-ircservices: an exstensible and flexible IRC Services package
 *  oftc.c: A protocol handler for the OFTC IRC Network
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

static void *oftc_sendmsg_gnotice(va_list);
static void *oftc_sendmsg_svsmode(va_list);
static void *oftc_sendmsg_svscloak(va_list);
static void *oftc_identify(va_list);

static dlink_node *oftc_gnotice_hook;
static dlink_node *oftc_umode_hook;
static dlink_node *oftc_svscloak_hook;
static dlink_node *oftc_identify_hook;

struct Message gnotice_msgtab = {
  "GNOTICE", 0, 0, 3, 0, MFLG_SLOW, 0,
  { m_ignore, m_ignore }
};

INIT_MODULE(oftc, "$Revision$")
{
  oftc_gnotice_hook   = install_hook(send_gnotice_cb, oftc_sendmsg_gnotice);
  oftc_umode_hook     = install_hook(send_umode_cb, oftc_sendmsg_svsmode);
  oftc_svscloak_hook  = install_hook(send_cloak_cb, oftc_sendmsg_svscloak);
  oftc_identify_hook  = install_hook(on_identify_cb, oftc_identify); 
  mod_add_cmd(&gnotice_msgtab);
}

CLEANUP_MODULE
{
  mod_del_cmd(&gnotice_msgtab);
  uninstall_hook(send_gnotice_cb, oftc_sendmsg_gnotice);
  uninstall_hook(send_umode_cb, oftc_sendmsg_svsmode);
  uninstall_hook(send_cloak_cb, oftc_sendmsg_svscloak);
  uninstall_hook(on_identify_cb, oftc_identify);
}

static void *
oftc_identify(va_list args)
{
  struct Client *uplink = va_arg(args, struct Client *);
  struct Client *client = va_arg(args, struct Client *);

  send_umode(NULL, client, "+R");

  return pass_callback(oftc_identify_hook);
}

static void *
oftc_sendmsg_gnotice(va_list args)
{
  struct Client *client = va_arg(args, struct Client*);
  char          *source = va_arg(args, char *);
  char          *text   = va_arg(args, char *);
  
  // 1 is UMODE_ALL, aka UMODE_SERVERNOTICE
  sendto_server(client, ":%s GNOTICE %s 1 :%s", source, source, text);
  return pass_callback(oftc_gnotice_hook);
}

static void *
oftc_sendmsg_svscloak(va_list args)
{
  struct Client *client = va_arg(args, struct Client *);
  char *cloakstring = va_arg(args, char *);

  sendto_server(client, ":%s SVSCLOAK %s :%s", 
    me.name, client->name, cloakstring);
  
  return pass_callback(oftc_svscloak_hook);
}

static void *
oftc_sendmsg_svsmode(va_list args)
{
  struct Client *client = va_arg(args, struct Client*);
  char          *target = va_arg(args, char *);
  char          *mode   = va_arg(args, char *);

  sendto_server(client, ":%s SVSMODE %s :%s", me.name, target, mode);

  return pass_callback(oftc_umode_hook);
}

#if 0
XXX unused atm
static void
oftc_sendmsg_svsnick(struct Client *client, struct Client *target, char *newnick)
{
  sendto_server(client, ":%s SVSNICK %s :%s", me.name, target->name, newnick);
}

#endif
