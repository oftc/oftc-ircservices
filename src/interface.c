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
struct Callback *send_newuser_cb;
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
struct Callback *on_newuser_cb;
struct Callback *on_identify_cb;

void
init_interface()
{
  services_heap       = BlockHeapCreate("services", sizeof(struct Service), SERVICES_HEAP_SIZE);
  /* XXX some of these should probably have default callbacks */
  send_newuser_cb     = register_callback("us introducing user", NULL);
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
  on_newuser_cb       = register_callback("New user coming to us", NULL);
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
    execute_callback(send_newuser_cb, me.uplink, service->name, "services", me.name,
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
  char *s, *t;
  
  va_start(ap, fmt);
  vsnprintf(buf, IRC_BUFSIZE, fmt, ap);
  va_end(ap);
  s = buf;
  while (*s) 
  {
    t = s;
    s += strcspn(s, "\n");
    if (*s)
      *s++ = 0;
    execute_callback(send_notice_cb, me.uplink, service->name, client->name, 
        *t != '\0' ? t : " ");
  }
}

void
send_umode(struct Service *service, struct Client *client, const char *mode)
{
  execute_callback(send_umode_cb, me.uplink, client->name, mode);
}
  
void
identify_user(struct Client *client)
{
  if(IsOper(client) && IsServAdmin(client))
    client->service_handler = ADMIN_HANDLER;
  else if(IsOper(client))
    client->service_handler = OPER_HANDLER;
  else
    client->service_handler = REG_HANDLER;

  SetIdentified(client);

  if(client->nickname->cloak[0] != '\0')
    cloak_user(client, client->nickname->cloak);

  execute_callback(on_identify_cb, me.uplink, client);
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
    if(msg == NULL)
    {
      reply_user(service, client, "HELP for %s is not available.",
          command);
      return;
    }

    reply_user(service, client, _L(service, client, msg->help_long));
    return;
  }
  do_serv_help_messages(service, client);
}

char *
replace_string(char *str, const char *value)
{
  char *ptr;
  size_t size = strlen(value);

  MyFree(str);
  ptr = MyMalloc(size+1);
  strlcpy(ptr, value, size+1);

  return ptr;
}

int
check_list_entry(const char *table, unsigned int id, const char *value)
{
  struct AccessEntry entry;
  void *ptr;

  ptr = db_list_first(table, id, &entry);

  while(ptr != NULL)
  {
    if(match(entry.value, value))
    {
      printf("check_list_entry: Found match: %s %s\n", entry.value, value);
      MyFree(entry.value);
      db_list_done(ptr);
      return TRUE;
    }
    
    printf("check_list_entry: Not Found match: %s %s\n", entry.value, value);
    MyFree(entry.value);
    ptr = db_list_next(ptr, &entry);
  }
  db_list_done(ptr);
  return FALSE;
}

void
free_nick(struct Nick *nick)
{
  printf("Freeing nick %p for %s\n", nick, nick->nick);
  MyFree(nick->email);
  nick->email = NULL;
  MyFree(nick->url);
  nick->url = NULL;
  MyFree(nick->last_quit);
  nick->last_quit = NULL;
}

int 
check_nick_pass(struct Nick *nick, const char *password)
{
  char fullpass[PASSLEN*2+1];
  char *pass;
  int ret;

  assert(nick);
  assert(nick->salt);
  
  snprintf(fullpass, PASSLEN*2, "%s%s", password, nick->salt);
  
  pass = crypt_pass(fullpass);
  if(strncmp(nick->pass, pass, sizeof(nick->pass)) == 0)
    ret = 1;
  else 
    ret = 0;

  MyFree(pass);
  return ret;
}

/*
 * make_random_string: fill buffer with (length - 1) random characters
 * a-z A-Z and then add a terminating \0
 */

static char randchartab[] = 
{
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
  'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
};

void 
make_random_string(char *buffer, size_t length)
{
  size_t i;
  int maxidx, j;

  maxidx = sizeof(randchartab) - 1;

  for (i = 0; i < (length - 1); i++) {
    j = rand() % (maxidx + 1);
    buffer[i] = randchartab[j];
  }
  buffer[length - 1] = 0;
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
chain_join(struct Client *source, char *channel)
{
  execute_callback(on_join_cb, source, channel);
}
