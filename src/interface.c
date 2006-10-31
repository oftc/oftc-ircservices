/*
 *  oftc-ircservices: an exstensible and flexible IRC Services package
 *  interface.c: Functions for interfacing with service modules
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

dlink_list services_list = { 0 };
struct Callback *newuser_cb;
struct Callback *privmsg_cb;
struct Callback *notice_cb;
struct Callback *gnotice_cb;
struct Callback *umode_cb;
static BlockHeap *services_heap  = NULL;

struct Callback *nick_hook;
struct Callback *join_hook;
struct Callback *part_hook;
struct Callback *quit_hook;
struct Callback *umode_hook;
struct Callback *cmode_hook;
struct Callback *squit_hook;

void
init_interface()
{
  services_heap = BlockHeapCreate("services", sizeof(struct Service), SERVICES_HEAP_SIZE);
  newuser_cb    = register_callback("introduce user", NULL);
  privmsg_cb    = register_callback("message user", NULL);
  notice_cb     = register_callback("NOTICE user", NULL);
  gnotice_cb    = register_callback("Global Notice", NULL);
  umode_cb      = register_callback("Set UMODE", NULL);
  nick_hook     = register_callback("Propagate NICK", NULL);
  join_hook     = register_callback("Propagate JOIN", NULL);
  part_hook     = register_callback("Propagate PART", NULL);
  quit_hook     = register_callback("Propagate QUIT", NULL);
  umode_hook    = register_callback("Propagate UMODE", NULL);
  cmode_hook    = register_callback("Propagate CMODE", NULL);
  squit_hook    = register_callback("Propagate SQUIT", NULL);
}

struct Service *
make_service(char *name)
{
  struct Service *service = BlockHeapAlloc(services_heap);  

  strlcpy(service->name, name, sizeof(service->name));

  return service;
}

void
introduce_service(struct Service *service)
{
  struct Client *client = make_client(&me);

  client->tsinfo = CurrentTime;
  dlinkAdd(client, &client->node, &global_client_list);

  /* copy the nick in place */
  strlcpy(client->name, service->name, sizeof(client->name));
  hash_add_client(client);

  register_remote_user(&me, client, "services", me.name, me.name, service->name);

  /* If we are not connected yet, the service will be sent as part of burst */
  if(me.uplink != NULL)
  {
    execute_callback(newuser_cb, me.uplink, service->name, "services", me.name,
      service->name, "o");
  }
}

void
tell_user(struct Service *service, struct Client *client, char *text)
{
  execute_callback(privmsg_cb, me.uplink, service->name, client->name, text);
}

void
reply_user(struct Service *service, struct Client *client, const char *fmt, ...)
{
  char buf[IRC_BUFSIZE+1];
  va_list ap;
  
  va_start(ap, fmt);
  vsnprintf(buf, IRC_BUFSIZE, fmt, ap);
  va_end(ap);
  execute_callback(notice_cb, me.uplink, service->name, client->name, buf);
}

void
send_umode(struct Service *service, struct Client *client, const char *mode)
{
  execute_callback(umode_cb, me.uplink, client->name, mode);
}

void
global_notice(struct Service *service, char *text, ...)
{
  va_list arg;
  char buf[IRC_BUFSIZE];
  char buf2[IRC_BUFSIZE];
  
  va_start(arg, text);
  vsnprintf(buf, IRC_BUFSIZE, text, arg);
  va_end(arg);

  if (service != NULL)
  {
    ircsprintf(buf2, "[%s] %s", service->name, buf);
    execute_callback(gnotice_cb, me.uplink, me.name, buf2);
  }
  else
    execute_callback(gnotice_cb, me.uplink, me.name, buf);
}

void
do_help(struct Service *service, struct Client *client, 
    const char *command, int parc, char *parv[])
{
  struct ServiceMessage *msg;
 
  /* Command specific help, show the long entry. */
  if(command != NULL)
  {
    msg = find_services_command(command, &service->msg_tree);
    /* Not possible */
    assert(msg != NULL);

    reply_user(service, client, _L(service, client, msg->help_long));
    return;
  }
  do_serv_help_messages(service, client);
}

void
chain_umode(struct Client *client_p, struct Client *source_p, int parc, char *parv[])
{
    execute_callback(umode_hook, client_p, source_p, parc, parv);
}

void
chain_cmode(struct Client *client_p, struct Client *source_p, struct Channel *chptr, int parc, char *parv[])
{
    execute_callback(cmode_hook, client_p, source_p, chptr, parc, parv);
}

void 
chain_squit(struct Client *client, struct Client *source, char *comment)
{
    execute_callback(squit_hook, client, source, comment);
}

void
chain_quit(struct Client *source, char *comment)
{
    execute_callback(quit_hook, source, comment);
}

void
chain_part(struct Client *client, struct Client *source, char *name)
{
    execute_callback(part_hook, client, source, name);
}

void
chain_nick(struct Client *client_p, struct Client *source_p, 
           int parc, char **parv, int newts, char *nick, char *gecos)
{
    execute_callback(nick_hook, client_p, source_p, parc, parv, newts, nick, gecos);
}
