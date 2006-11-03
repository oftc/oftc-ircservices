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
struct Callback *send_privmsg_cb;
struct Callback *send_notice_cb;
struct Callback *send_gnotice_cb;
struct Callback *send_umode_cb;
struct Callback *send_cloak_cb;
static BlockHeap *services_heap  = NULL;

struct Callback *on_nick_change_cb;
struct Callback *on_join_cb;
struct Callback *on_part_cb;
struct Callback *on_quit_cb;
struct Callback *on_umode_change_cb;
struct Callback *on_cmode_change_cb;
struct Callback *on_squit_cb;

struct Callback *on_identify_cb;

void
init_interface()
{
  services_heap       = BlockHeapCreate("services", sizeof(struct Service), SERVICES_HEAP_SIZE);
  /* XXX some of these should probably have default callbacks */
  newuser_cb          = register_callback("introduce user", NULL);
  send_privmsg_cb     = register_callback("message user", NULL);
  send_notice_cb      = register_callback("NOTICE user", NULL);
  send_gnotice_cb     = register_callback("Global Notice", NULL);
  send_umode_cb       = register_callback("Set UMODE", NULL);
  send_cloak_cb       = register_callback("Cloak an user", NULL);
  on_nick_change_cb   = register_callback("Propagate NICK", NULL);
  on_join_cb          = register_callback("Propagate JOIN", NULL);
  on_part_cb          = register_callback("Propagate PART", NULL);
  on_quit_cb          = register_callback("Propagate QUIT", NULL);
  on_umode_change_cb  = register_callback("Propagate UMODE", NULL);
  on_cmode_change_cb  = register_callback("Propagate CMODE", NULL);
  on_quit_cb          = register_callback("Propagate SQUIT", NULL);
  on_identify_cb      = register_callback("Identify Callback", NULL);
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
  execute_callback(send_privmsg_cb, me.uplink, service->name, client->name, text);
}

void
reply_user(struct Service *service, struct Client *client, const char *fmt, ...)
{
  char buf[IRC_BUFSIZE+1];
  va_list ap;
  
  va_start(ap, fmt);
  vsnprintf(buf, IRC_BUFSIZE, fmt, ap);
  va_end(ap);
  execute_callback(send_notice_cb, me.uplink, service->name, client->name, buf);
}

void
send_umode(struct Service *service, struct Client *client, const char *mode)
{
  execute_callback(send_umode_cb, me.uplink, client->name, mode);
}
  
int
identify_user(struct Client *client, const char *password)
{
  struct Nick *nick;

  if((nick = db_find_nick(client->name)) == NULL)
    return ERR_ID_NONICK; 

  if(strncmp(nick->pass, servcrypt(password, nick->pass), 
    sizeof(nick->pass)) != 0)
  {
    MyFree(nick);
    return ERR_ID_WRONGPASS;
  }

  client->nickname = nick;

  if(IsOper(client) && IsServAdmin(client))
    client->service_handler = ADMIN_HANDLER;
  else if(IsOper(client))
    client->service_handler = OPER_HANDLER;
  else
    client->service_handler = REG_HANDLER;

  SetIdentified(client);

  execute_callback(on_identify_cb, me.uplink, client);

  return ERR_ID_NOERROR;
}

void
cloak_user(struct Client *client, char *cloak)
{
  execute_callback(send_cloak_cb, client, cloak);
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
    execute_callback(send_gnotice_cb, me.uplink, me.name, buf2);
  }
  else
    execute_callback(send_gnotice_cb, me.uplink, me.name, buf);
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
  execute_callback(on_umode_change_cb, client_p, source_p, parc, parv);
}

void
chain_cmode(struct Client *client_p, struct Client *source_p, struct Channel *chptr, int parc, char *parv[])
{
  execute_callback(on_cmode_change_cb, client_p, source_p, chptr, parc, parv);
}

void 
chain_squit(struct Client *client, struct Client *source, char *comment)
{
  execute_callback(on_quit_cb, client, source, comment);
}

void
chain_quit(struct Client *source, char *comment)
{
  execute_callback(on_quit_cb, source, comment);
}

void
chain_part(struct Client *client, struct Client *source, char *name)
{
  execute_callback(on_part_cb, client, source, name);
}

void
chain_nick(struct Client *client_p, struct Client *source_p, 
    int parc, char **parv, int newts, char *nick, char *gecos)
{
  execute_callback(on_nick_change_cb, client_p, source_p, parc, parv, newts, nick, gecos);
}
