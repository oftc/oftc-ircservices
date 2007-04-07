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
#include <openssl/hmac.h>

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
struct Callback *send_kill_cb;
struct Callback *send_newserver_cb;
struct Callback *send_join_cb;
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
struct Callback *on_topic_change_cb;
struct Callback *on_privmsg_cb;
struct Callback *on_notice_cb;

struct LanguageFile ServicesLanguages[LANG_LAST];
struct ModeList *ServerModeList;

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
  send_kill_cb        = register_callback("Send KILL", NULL);
  send_newserver_cb   = register_callback("Introduce new server", NULL);
  send_join_cb        = register_callback("Send JOIN", NULL);
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
  on_topic_change_cb  = register_callback("Topic changed", NULL);
  on_privmsg_cb       = register_callback("Privmsg for channel received", NULL);
  on_notice_cb        = register_callback("Notice for channel received", NULL);

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
  if(ServicesState.namesuffix != NULL)
    strlcat(service->name, ServicesState.namesuffix, sizeof(service->name));

  return service;
}

void
introduce_client(const char *name)
{
  struct Client *client = make_client(&me);

  client->tsinfo = CurrentTime;
  dlinkAdd(client, &client->node, &global_client_list);

  /* copy the nick in place */
  strlcpy(client->name, name, sizeof(client->name));
  hash_add_client(client);

  register_remote_user(&me, client, "services", me.name, me.name, name);

  /* If we are not connected yet, the service will be sent as part of burst */
  if(me.uplink != NULL)
  {
    execute_callback(send_newuser_cb, me.uplink, name, "services", me.name,
      name, "o");
  }
}

struct Client*
introduce_server(const char *name, const char *gecos)
{
  struct Client *client = make_client(&me);

  client->tsinfo = CurrentTime;
  dlinkAdd(client, &client->node, &global_client_list);

  strlcpy(client->name, name, sizeof(client->name));
  strlcpy(client->info, gecos, sizeof(client->info));
  hash_add_client(client);

  if(me.uplink != NULL)
    execute_callback(send_newserver_cb, client);

  return client;
}

struct Channel*
join_channel(struct Client *service, const char *chname)
{
  struct Channel *channel = hash_find_channel(chname);

  if(channel == NULL)
    channel = make_channel(chname);

  execute_callback(send_join_cb, me.uplink, me.name, chname,
    channel->channelts, 0, service->name);

  add_user_to_channel(channel, service, 0, 0);
  chain_join(service, channel->chname);

  return channel;
}

void
tell_user(struct Service *service, struct Client *client, char *text)
{
  //execute_callback(send_privmsg_cb, me.uplink, service->name, client->name, text);
}

/* Ensure the result buffer is TIME_BUFFER+1 in size */
size_t
strtime(struct Client *client, time_t tm, char *result)
{
  char *timestr;
  
  if(client->nickname == NULL)
    timestr = ServicesLanguages[0].entries[SERV_DATETIME_FORMAT];
  else
    timestr = ServicesLanguages[client->nickname->language].entries[SERV_DATETIME_FORMAT];

  if(tm <= 0)
    return strlcpy(result, "Unknown", TIME_BUFFER + 1);
  else
    return strftime(result, TIME_BUFFER, timestr, gmtime(&tm));
}

void reply_time(struct Service *service, struct Client *client, 
    unsigned int baseid, time_t off)
{
  char buf[TIME_BUFFER + 1];
  struct tm diff;

  strtime(client, off, buf);
  
  date_diff(CurrentTime, off, &diff);

  if(diff.tm_year > 0)
    reply_user(service, service, client, baseid, buf,
      diff.tm_year, diff.tm_mon, diff.tm_mday-1, diff.tm_hour, diff.tm_min, 
      diff.tm_sec);
  else if(diff.tm_mon > 0)
    reply_user(service, service, client, baseid+1, buf,
      diff.tm_mon, diff.tm_mday-1, diff.tm_hour, diff.tm_min, diff.tm_sec);
  else if(diff.tm_mday > 1)
    reply_user(service, service, client, baseid+2, buf,
      diff.tm_mday-1, diff.tm_hour, diff.tm_min, diff.tm_sec);
  else
    reply_user(service, service, client, baseid+3, buf,
      diff.tm_hour, diff.tm_min, diff.tm_sec);
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
    if(ServicesState.debugmode)
      ilog(L_DEBUG, "Was going to send: %s to %s", t, client->name);
    else
      execute_callback(send_notice_cb, me.uplink, source->name, client->name, 
          *t != '\0' ? t : " ");
  }
}

void
kill_user(struct Service *service, struct Client *client, const char *reason)
{
  if(ServicesState.debugmode)
    ilog(L_DEBUG, "Was going to kill: %s (%s)", client->name, reason);
  else
  {
    execute_callback(send_kill_cb, me.uplink, service, client, reason);
    exit_client(client, &me, reason);
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
  if(!ServicesState.debugmode)
  {
    if(!IsMe(client->from))
      execute_callback(send_nick_cb, me.uplink, client, newnick);
  }
}

void
send_akill(struct Service *service, char *setter, struct ServiceBan *akill)
{
  if(!ServicesState.debugmode)
    execute_callback(send_akill_cb, me.uplink, service, setter, akill->mask,
        akill->reason);
}

void
remove_akill(struct Service *service, struct ServiceBan *akill)
{
  execute_callback(send_unakill_cb, me.uplink, service, akill->mask);
}

void
set_limit(struct Service *service, struct Channel *chptr, int limit)
{
  char limitstr[16];

  if(ServicesState.debugmode)
    return;

  snprintf(limitstr, 16, "%d", limit);
  execute_callback(send_cmode_cb, me.uplink, service->name, chptr->chname,
      "+l", limitstr);
  chptr->mode.limit = limit;
}

void
send_cmode(struct Service *service, struct Channel *chptr, const char *mode,
    const char *param)
{
  if(ServicesState.debugmode)
    return;

  execute_callback(send_cmode_cb, me.uplink, service->name, chptr->chname, 
      mode, param);
}

void
send_topic(struct Service *service, struct Channel *chptr, 
    struct Client *client, const char *topic)
{
  if(ServicesState.debugmode)
    return;

  execute_callback(send_topic_cb, me.uplink, service, chptr, client,
      topic);
}

void
kick_user(struct Service *service, struct Channel *chptr, const char *client, 
    const char *reason)
{
  if(ServicesState.debugmode)
    return;

  execute_callback(send_kick_cb, me.uplink, service->name, chptr->chname, 
      client, reason);
}

static void 
opdeop_user(struct Channel *chptr, struct Client *client, int op)
{
  struct Membership *member;

  if ((member = find_channel_link(client, chptr)) == NULL)
    return;

  if((op && has_member_flags(member, CHFL_CHANOP)) || (!op && 
      !has_member_flags(member, CHFL_CHANOP)))
    return;

  if(op)
  {
    AddMemberFlag(member, CHFL_CHANOP);
    DelMemberFlag(member, CHFL_DEOPPED | CHFL_HALFOP);
  }
  else
    DelMemberFlag(member, CHFL_CHANOP);
}

void
op_user(struct Service *service, struct Channel *chptr, struct Client *client)
{
  if(ServicesState.debugmode)
    return;

  send_cmode(service, chptr, "+o", client->name);
  opdeop_user(chptr, client, 1);
}

void
deop_user(struct Service *service, struct Channel *chptr, struct Client *client)
{
  if(ServicesState.debugmode)
    return;

  send_cmode(service, chptr, "-o", client->name);
  opdeop_user(chptr, client, 0);
}

void
devoice_user(struct Service *service, struct Channel *chptr, 
    struct Client *client)
{
  if(ServicesState.debugmode)
    return;

  send_cmode(service, chptr, "-v", client->name);
}

void
invite_user(struct Service *service, struct Channel *chptr, struct Client *client)
{
  if(ServicesState.debugmode)
    return;

  execute_callback(send_invite_cb, me.uplink, service, chptr, client);
}

void
ban_mask(struct Service *service, struct Channel *chptr, const char *mask)
{
  struct Client *client = find_client(service->name);
  if(ServicesState.debugmode)
    return;

  send_cmode(service, chptr, "+b", mask);
  add_id(client, chptr, (char*)mask, CHFL_BAN);
}

void
unban_mask(struct Service *service, struct Channel *chptr, const char *mask)
{
  if(ServicesState.debugmode)
    return;

  send_cmode(service, chptr, "-b", mask);
  del_id(chptr, (char*)mask, CHFL_BAN);
}
  
void
identify_user(struct Client *client)
{
  struct Nick *nick = client->nickname;

  if(nick->admin && IsOper(client))
    client->access = ADMIN_FLAG;
  else if(!nick->admin && IsOper(client))
    client->access = OPER_FLAG;
  else
    client->access = IDENTIFIED_FLAG;

  if(nick->cloak[0] != '\0' && nick->cloak_on)
    cloak_user(client, nick->cloak);

  client->num_badpass = 0;

  execute_callback(on_identify_cb, me.uplink, client);
}

void
cloak_user(struct Client *client, char *cloak)
{
  if(ServicesState.debugmode)
    return;

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

    if(!(msg->flags & SFLG_CHANARG))
    {
      if(client->access < msg->access)
      {
        reply_user(service, NULL, client, SERV_HELP_NOT_AVAIL, command);
        return;
      }
    }


    sub = msg->sub;
    
    if(parc > 1)
    { 
      while(sub != NULL && sub->cmd != NULL)
      {
        if(strncasecmp(sub->cmd, parv[2], strlen(sub->cmd)) == 0)
        {
          reply_user(service, NULL, client, SERV_SUB_HELP_HEADER, msg->cmd,
              sub->cmd);
          reply_user(service, service, client, sub->help_long, "");
          reply_user(service, NULL, client, SERV_SUB_HELP_FOOTER, msg->cmd,
              sub->cmd);
          return;   
        }
        sub++;
      }
      reply_user(service, NULL, client, SERV_SUB_HELP_NOT_AVIL, command, parv[2]);
      return;
    }

    reply_user(service, NULL, client, SERV_HELP_HEADER, msg->cmd);
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
    
    reply_user(service, NULL, client, SERV_HELP_FOOTER, msg->cmd);
    
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
      MyFree(entry);
      db_list_done(first);
      return TRUE;
    }
    
    ilog(L_DEBUG, "check_list_entry: Not Found match: %s %s", entry->value, 
        value);
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
      MyFree(nick);
      return TRUE;
    }
    MyFree(nick);
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

unsigned int
get_mode_from_letter(char letter)
{
  int i;

  for(i = 0; ServerModeList[i].letter != '\0'; i++)
  {
    if(ServerModeList[i].letter == letter)
      return ServerModeList[i].mode;
  }
  return 0;
}

void
get_modestring(unsigned int modes, char *modbuf, int len)
{
  int i, j;

  for(i = 0, j = 0; ServerModeList[i].letter != '\0'; i++)
  {
    if(ServerModeList[i].mode & modes)
      modbuf[j++] = ServerModeList[i].letter;
    if(j >= len)
      break;
  }
  modbuf[j] = '\0';
}

/* 
 * The reason this function looks a bit overkill is it doubles as a valdation
 * and setting function which can be called both by the chanserv module when
 * someone does set #foo mlock, but can also be called to enforce the mlock
 * without changing it in the db. It's because of this oddness it's full of
 * comments to clarify it somewhat.
 */
int
set_mode_lock(struct Service *service, struct Channel *chptr, 
    struct Client *client, const char *lock, char **value)
{
  const char *parv[3] = { NULL, NULL, NULL };
  char *p;
  unsigned int setmodes, delmodes, currmode;
  int i, dir, para = 1, limit;
  char mode, *c;
  char key[KEYLEN+1];
  char modebuf[MODEBUFLEN+1], parabuf[MODEBUFLEN+1];
  char setstr[MODEBUFLEN/2+1], delstr[MODEBUFLEN/2+1]; 
  char mlockbuf[MODEBUFLEN+1];
  int k, l, s, d;

  k = l = s = d = 0;

  setmodes = delmodes = dir = limit = 0;
  memset(key, 0, sizeof(key));

  parv[0] = lock;

  /* Split the mlock up into its component parts, modes, key, limit */
  if((p = strchr(lock, ' ')) != NULL)
  {
    *p++ = '\0';
    lock = p;

    parv[1] = lock;
    if((p = strchr(lock, ' ')) != NULL)
    {
      *p++ = '\0';
      lock = p;

      parv[2] = lock;
      if((p = strchr(lock, ' ')) != NULL)
        *p++ = '\0';
    }
  }

  /* 
   * Now check the mlock is valid for this server/protocol configuration and
   * build up 2 integers which represent the set and cleared modes.
   */
  for(i = 0, mode = parv[0][i]; mode != '\0'; i++, mode = parv[0][i])
  {
    /*
     * l and k are both special cases because they arent listed in the
     * protocol's mode list because they take paramters, but we can MLOCK
     * them.
     */
    if(mode != '+' && mode != '-' && mode != 'l' && mode != 'k')
    {
      if((currmode = get_mode_from_letter(mode)) <= 0)
      {
        if(client != NULL)
          reply_user(service, service, client, CS_BAD_MLOCK, mode);
        return FALSE;
      }
      if(dir)
      {
        setmodes |= currmode;
        delmodes &= ~currmode;
      }
      else
      {
        delmodes |= currmode;
        setmodes &= ~currmode;
      }
    }

    switch(mode)
    {
      case '+':
        dir = 1;
        break;
      case '-':
        dir = 0;
        break;
      case 'l':
        if(dir)
        {
          if(parv[para] == NULL)
          {
            if(client != NULL)
              reply_user(service, service, client, CS_NEED_LIMIT);
            return FALSE;
          }
          limit = atoi(parv[para++]);
          if(limit <= 0)
          {
            if(client != NULL)
              reply_user(service, service, client, CS_NEED_LIMIT);
            return FALSE;
          }
        }
        break;
      case 'k':
        if(dir)
          {
            if(parv[para] == NULL)
            {
              if(client != NULL)
                reply_user(service, service, client, CS_NEED_KEY);
              return FALSE;
            }
            strlcpy(key, parv[para++], sizeof(key));
          }
        break;
    }
  }

  /* 
   * Now send out the mlock, by converting that integer back to a mode string
   * again. The reason we put it into an integer and then straight back to a
   * string is to cheaply prevent duplicate modes, and also it allows us to
   * split it into two, one for setting, one for unsetting.
   */
  
  /* First just set the mlock */
  get_modestring(setmodes, setstr, MODEBUFLEN/2);
  get_modestring(delmodes, delstr, MODEBUFLEN/2);

  /* If we've been asked to update the db, then we should do so. */
  if(value != NULL)
  {
    char *lk = "";

    k = l = s = d = FALSE;

    if(*setstr != '\0')
      s = TRUE;
    if(delstr[0] != '\0')
      d = TRUE;
    if(limit > 0)
    {
      l = TRUE;
      if(key[0] != '\0')
      {
        k = TRUE;
        lk = "lk";
        snprintf(parabuf, MODEBUFLEN, " %d %s", limit, key);
      }
      else
      {
        lk = "l";
        snprintf(parabuf, MODEBUFLEN, " %d", limit);
      }
    }
    else if(key[0] != '\0')
    {
      k = TRUE;
      lk = "k";
      snprintf(parabuf, MODEBUFLEN, " %s", key);
    }

    snprintf(mlockbuf, MODEBUFLEN, "%s%s%s%s%s%s",
        s || l || k ? "+": "", 
        s ? setstr : "", 
        lk,
        d ? "-" : "",
        d ? delstr : "",
        l || k ? parabuf : "");

    if(!db_set_string(SET_CHAN_MLOCK, chptr->regchan->id, 
          *mlockbuf == '\0' ? NULL : mlockbuf))
      return FALSE;

    if(*mlockbuf == '\0')
    {
      MyFree(*value);
      *value = NULL;
    }
    else
      *value = replace_string(*value, mlockbuf);
  }

  /* Now only set the mode that needs to be set */
  c = setstr;
  while(*c != '\0')
  {
    mode = get_mode_from_letter(*c);
    if(mode <= 0)
      continue;
    if(chptr->mode.mode & mode)
      setmodes &= ~mode;
    c++;
  }

  c = delstr;
  while(*c != '\0')
  {
    mode = get_mode_from_letter(*c);
    if(mode <= 0)
      continue;
    if(!(chptr->mode.mode & mode))
      delmodes &= ~mode;
    c++;
  }

  get_modestring(setmodes, setstr, MODEBUFLEN/2);
  get_modestring(delmodes, delstr, MODEBUFLEN/2);
  chptr->mode.mode |= setmodes;
  chptr->mode.mode &= ~delmodes;
  if(l)
    chptr->mode.limit = limit;
  if(k)
    strcpy(chptr->mode.key, key);
 
  /* 
   * Set up the modestring and paramter(s) and set them. This could probably
   * be written a little better.
   */
  if(limit > 0 && key[0] != '\0')
  {
    snprintf(parabuf, MODEBUFLEN, "%d %s", limit, key);
    snprintf(modebuf, MODEBUFLEN, "%s%slk%s%s", 
        (setstr[0] == '\0' && (limit > 0 || key != NULL)) ?
         "+" : "", setstr, delstr[0] ? "-" : "", delstr);
    send_cmode(service, chptr, modebuf, parabuf);
 }
  else if(limit > 0)
  {
    snprintf(parabuf, MODEBUFLEN, "%d", limit);
    snprintf(modebuf, MODEBUFLEN, "%s%sl%s%s", setstr[0] ? "+" : "", 
        setstr, delstr[0] ? "-" : "", delstr);
    send_cmode(service, chptr, modebuf, parabuf);
  }
  else if(key[0] != '\0')
  {
    snprintf(parabuf, MODEBUFLEN, "%s", key);
    snprintf(modebuf, MODEBUFLEN, "%s%sk%s%s", setstr[0] ? "+" : "", 
        setstr, delstr[0] ? "-" : "", delstr);
    send_cmode(service, chptr, modebuf, key);
  }
  else
  {
    memset(parabuf, 0, sizeof(parabuf));
    snprintf(modebuf, MODEBUFLEN, "%s%s%s%s", setstr[0] ? "+" : "", 
        setstr, delstr[0] ? "-" : "", delstr);
    if(*modebuf != '\0')
      send_cmode(service, chptr, modebuf, "");
  }

  return TRUE;
}

void
free_regchan(struct RegChannel *regchptr)
{
  MyFree(regchptr->description);
  MyFree(regchptr->entrymsg);
  MyFree(regchptr->url);
  MyFree(regchptr->email);
  MyFree(regchptr->topic);
  MyFree(regchptr);
}

void
free_nick(struct Nick *nick)
{
  ilog(L_DEBUG, "Freeing nick %p for %s", nick, nick->nick);
  MyFree(nick->email);
  MyFree(nick->url);
  MyFree(nick->last_quit);
  MyFree(nick->last_host);
  MyFree(nick->last_realname);
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
  
  snprintf(fullpass, sizeof(fullpass), "%s%s", password, nick->salt);
  
  pass = crypt_pass(fullpass, 1);
  if(strncasecmp(nick->pass, pass, PASSLEN*2) == 0)
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
  buffer[length - 1] = '\0';
}

char *
generate_hmac(const char *data)
{
  unsigned char hash[EVP_MAX_MD_SIZE] = {0};
  unsigned int len;
  char *key;
  char *hexdata;

  key = crypt_pass(ServicesInfo.hmac_secret, 0);

  HMAC(EVP_sha1(), key, DIGEST_LEN, (unsigned char*)data, strlen(data), hash, 
      &len);

  hexdata = MyMalloc(len*2 + 1);
  base16_encode(hexdata, len*2+1, (char*)hash, len);

  MyFree(key);
  return hexdata;
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
