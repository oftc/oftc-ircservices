/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
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
struct Callback *send_nick_cb;
struct Callback *send_akill_cb;
struct Callback *send_unakill_cb;
struct Callback *send_kick_cb;
struct Callback *send_cmode_cb;
struct Callback *send_invite_cb;
struct Callback *send_topic_cb;
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
struct Callback *on_channel_destroy_cb;

struct LanguageFile ServicesLanguages[LANG_LAST];

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
  send_nick_cb        = register_callback("Send a new nickname", NULL);
  send_akill_cb       = register_callback("Send AKILL to network", NULL);
  send_unakill_cb     = register_callback("Send UNAKILL", NULL);
  send_kick_cb        = register_callback("Send KICK to network", NULL);
  send_cmode_cb       = register_callback("Send Channel MODE", NULL);
  send_invite_cb      = register_callback("Send INVITE", NULL);
  send_topic_cb       = register_callback("Send TOPIC", NULL);
  on_nick_change_cb   = register_callback("Propagate NICK", NULL);
  on_join_cb          = register_callback("Propagate JOIN", NULL);
  on_part_cb          = register_callback("Propagate PART", NULL);
  on_quit_cb          = register_callback("Propagate QUIT", NULL);
  on_umode_change_cb  = register_callback("Propagate UMODE", NULL);
  on_cmode_change_cb  = register_callback("Propagate CMODE", NULL);
  on_quit_cb          = register_callback("Propagate SQUIT", NULL);
  on_identify_cb      = register_callback("Identify Callback", NULL);
  on_newuser_cb       = register_callback("New user coming to us", NULL);
  on_channel_destroy_cb = register_callback("Channel is being destroyed", NULL);

  load_language(ServicesLanguages, "services.en");
}

void
cleanup_interface()
{
  BlockHeapDestroy(services_heap);
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
reply_user(struct Service *source, struct Service *service, 
    struct Client *client, unsigned int langid,
    ...)
{
  char buf[IRC_BUFSIZE+1];
  va_list ap;
  char *s, *t;
  char *langstr = NULL;
  struct LanguageFile *languages;

  if(service == NULL)
    languages = ServicesLanguages;
  else
    languages = service->languages;
  
  if(langid != 0)
  {
    if(client->nickname == NULL)
      langstr = languages[0].entries[langid];
    else
      langstr = languages[client->nickname->language].entries[langid];
  }
   
  if(langstr == NULL)
    langstr = "%s";
  
  va_start(ap, langid);
  vsnprintf(buf, IRC_BUFSIZE, langstr, ap);
  va_end(ap);
  s = buf;
  while (*s) 
  {
    t = s;
    s += strcspn(s, "\n");
    if (*s)
      *s++ = 0;
    execute_callback(send_notice_cb, me.uplink, source->name, client->name, 
        *t != '\0' ? t : " ");
  }
}

void
send_umode(struct Service *service, struct Client *client, const char *mode)
{
  execute_callback(send_umode_cb, me.uplink, client->name, mode);
}

void
send_nick_change(struct Service *service, struct Client *client, 
    const char *newnick)
{
  execute_callback(send_nick_cb, me.uplink, client, newnick);
}

void
send_akill(struct Service *service, char *setter, struct ServiceBan *akill)
{
  execute_callback(send_akill_cb, me.uplink, service, setter, akill->mask,
      akill->reason);
}

void
remove_akill(struct Service *service, struct ServiceBan *akill)
{
  execute_callback(send_unakill_cb, me.uplink, service, akill->mask);
}

void
send_cmode(struct Service *service, struct Channel *chptr, const char *mode,
    const char *param)
{
  execute_callback(send_cmode_cb, me.uplink, service->name, chptr->chname, 
      mode, param);
}

void
send_topic(struct Service *service, struct Channel *chptr, 
    struct Client *client, const char *topic)
{
  execute_callback(send_topic_cb, me.uplink, service, chptr, client,
      topic);
}

void
kick_user(struct Service *service, struct Channel *chptr, const char *client, 
    const char *reason)
{
  execute_callback(send_kick_cb, me.uplink, service->name, chptr->chname, 
      client, reason);
}

void
op_user(struct Service *service, struct Channel *chptr, struct Client *client)
{
  send_cmode(service, chptr, "+o", client->name);
}

void
deop_user(struct Service *service, struct Channel *chptr, struct Client *client)
{
  send_cmode(service, chptr, "-o", client->name);
}

void
devoice_user(struct Service *service, struct Channel *chptr, 
    struct Client *client)
{
  send_cmode(service, chptr, "-v", client->name);
}

void
invite_user(struct Service *service, struct Channel *chptr, struct Client *client)
{
  execute_callback(send_invite_cb, me.uplink, service, chptr, client);
}

void
ban_mask(struct Service *service, struct Channel *chptr, const char *mask)
{
  struct Client *client = find_client(service->name);

  send_cmode(service, chptr, "+b", mask);
  add_id(client, chptr, (char*)mask, CHFL_BAN);
}

void
unban_mask(struct Service *service, struct Channel *chptr, const char *mask)
{
  send_cmode(service, chptr, "-b", mask);
  del_id(chptr, (char*)mask, CHFL_BAN);
}
  
void
identify_user(struct Client *client)
{
  struct Nick *nick = client->nickname;

  SetIdentified(client);

  if(nick->admin && IsOper(client))
    client->access = ADMIN_FLAG;
  else if(!nick->admin && IsOper(client))
    client->access = OPER_FLAG;
  else
    client->access = IDENTIFIED_FLAG;

  if(nick->cloak[0] != '\0' && nick->cloak_on)
    cloak_user(client, nick->cloak);

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
  struct ServiceMessage *msg, *sub;

  /* Command specific help, show the long entry. */
  if(command != NULL)
  {
    msg = find_services_command(command, &service->msg_tree);
    if(msg == NULL)
    {
      reply_user(service, NULL, client, SERV_HELP_NOT_AVAIL, command);
      return;
    }

    sub = msg->sub;
    
    if(parc > 1)
    { 
      while(sub != NULL && sub->cmd != NULL)
      {
        if(strncasecmp(sub->cmd, parv[2], sizeof(sub->cmd)) == 0)
        {
          reply_user(service, service, client, sub->help_long, "");
          return;   
        }
        sub++;
      }
      reply_user(service, service, client, SERV_SUB_HELP_NOT_AVIL, command, parv[2]);
      return;
    }

    reply_user(service, service, client, msg->help_long, "");
    sub = msg->sub;
    
    while(sub != NULL && sub->cmd != NULL)
    {
      if(sub->help_short > 0)
        reply_user(service, service, client, sub->help_short, sub->cmd);
      else
        reply_user(service, service, client, 0, sub->cmd);

      sub++;
      if(sub->cmd == NULL)
        sub = NULL;
    }
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
check_list_entry(unsigned int type, unsigned int id, const char *value)
{
  struct AccessEntry *entry;
  void *ptr, *first;

  first = ptr = db_list_first(type, id, (void**)&entry);

  if(ptr == NULL)
    return FALSE;

  while(ptr != NULL)
  {
    if(match(entry->value, value))
    {
      ilog(L_DEBUG, "check_list_entry: Found match: %s %s", entry->value, 
          value);
      MyFree(entry->value);
      MyFree(entry);
      db_list_done(first);
      return TRUE;
    }
    
    ilog(L_DEBUG, "check_list_entry: Not Found match: %s %s", entry->value, 
        value);
    MyFree(entry->value);
    MyFree(entry);
    ptr = db_list_next(ptr, type, (void**)&entry);
  }
  db_list_done(first);
  return FALSE;
}

int enforce_matching_serviceban(struct Service *service, struct Channel *chptr, 
    struct Client *client)
{
  struct ServiceBan *sban;
  void *ptr, *first;

  if(chptr != NULL && chptr->regchan == NULL)
    return FALSE;

  if(chptr != NULL)
    first = ptr = db_list_first(AKICK_LIST, chptr->regchan->id, (void**)&sban);
  else
    first = ptr = db_list_first(AKILL_LIST, 0, (void**)&sban);

  if(ptr == NULL)
  {
    free_serviceban(sban);
    return FALSE;
  }

  while(ptr != NULL)
  {
    if(enforce_client_serviceban(service, chptr, client, sban))
    {
      free_serviceban(sban);
      db_list_done(first);
      return TRUE;
    }
    free_serviceban(sban);
    if(chptr != NULL)
      ptr = db_list_next(ptr, AKICK_LIST, (void**)&sban);
    else
      ptr = db_list_next(ptr, AKILL_LIST, (void**)&sban);
  }

  db_list_done(first);

  return FALSE;
}

int
enforce_akick(struct Service *service, struct Channel *chptr, 
    struct ServiceBan *akick)
{
  dlink_node *ptr;
  int numkicks = 0;

  DLINK_FOREACH(ptr, chptr->members.head)
  {
    struct Membership *ms = ptr->data;
    struct Client *client = ms->client_p;

    numkicks += enforce_client_serviceban(service, chptr, client, akick);
  }
  return numkicks;
}

int
enforce_client_serviceban(struct Service *service, struct Channel *chptr, 
    struct Client *client, struct ServiceBan *sban)
{
  struct irc_ssaddr addr;
  struct split_nuh_item nuh;
  char name[NICKLEN];
  char user[USERLEN + 1];
  char host[HOSTLEN + 1];
  int type, bits, found = 0;

  if(sban->mask == NULL && sban->type == AKICK_BAN)
  {
    char *nick = db_get_nickname_from_id(sban->target);
    if(ircncmp(nick, client->name, NICKLEN) == 0)
    {
      snprintf(host, HOSTLEN, "%s!*@*", nick);
      ban_mask(service, chptr, host);
      kick_user(service, chptr, client->name, sban->reason);
      return TRUE;
    }
    return FALSE;
  }
  DupString(nuh.nuhmask, sban->mask);

  nuh.nickptr  = name;
  nuh.userptr  = user;
  nuh.hostptr  = host;

  nuh.nicksize = sizeof(name);
  nuh.usersize = sizeof(user);
  nuh.hostsize = sizeof(host);

  split_nuh(&nuh);

  type = parse_netmask(host, &addr, &bits);

  if(match(name, client->name) && match(user, client->username))
  {
    switch(type)
    {
      case HM_HOST:
        if(match(host, client->host))
          found = 1;
        break;
    }
    if(found)
    {
      if(sban->type == AKICK_BAN)
      {
        ban_mask(service, chptr, sban->mask);
        kick_user(service, chptr, client->name, sban->reason);
      }
      else if(sban->type == AKILL_BAN)
      {
        char *setter = db_get_nickname_from_id(sban->setter);

        send_akill(service, setter, sban);
        MyFree(setter);
      }
    }
  }
  MyFree(nuh.nuhmask);
  return found;
}

void
free_regchan(struct RegChannel *regchptr)
{
  MyFree(regchptr->description);
  MyFree(regchptr->entrymsg);
  MyFree(regchptr);
}

void
free_nick(struct Nick *nick)
{
  ilog(L_DEBUG, "Freeing nick %p for %s", nick, nick->nick);
  MyFree(nick->email);
  nick->email = NULL;
  MyFree(nick->url);
  nick->url = NULL;
  MyFree(nick->last_quit);
  nick->last_quit = NULL;
  MyFree(nick);
}

void
free_serviceban(struct ServiceBan *ban)
{
  ilog(L_DEBUG, "Freeing serviceban %p for %s", ban, ban->mask);
  MyFree(ban->mask);
  MyFree(ban->reason);
  MyFree(ban->channel);
  MyFree(ban);
}

void
free_chanaccess(struct ChanAccess *access)
{
	ilog(L_DEBUG, "Freeing chanaccess %p for channel %d", access, access->channel);
	MyFree(access);
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
chain_part(struct Client *client, struct Client *source, char *name)
{
  execute_callback(on_part_cb, client, source, name);
}

void
chain_join(struct Client *source, char *channel)
{
  execute_callback(on_join_cb, source, channel);
}
