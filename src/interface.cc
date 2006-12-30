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
#include <string>
#include <stdexcept>

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
  struct Service *service = (struct Service *)BlockHeapAlloc(services_heap);  

  strlcpy(service->name, name, sizeof(service->name));

  return service;
}

#if 0
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
set_limit(struct Service *service, struct Channel *chptr, int limit)
{
  char limitstr[16];

  snprintf(limitstr, 16, "%d", limit);
  execute_callback(send_cmode_cb, me.uplink, service->name, chptr->chname,
      "+l", limitstr);
  chptr->mode.limit = limit;
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
    Client *client, const char *topic)
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
op_user(struct Service *service, struct Channel *chptr, Client *client)
{
  send_cmode(service, chptr, "+o", client->name);
}

void
deop_user(struct Service *service, struct Channel *chptr, Client *client)
{
  send_cmode(service, chptr, "-o", client->name);
}

void
devoice_user(struct Service *service, struct Channel *chptr, 
    Client *client)
{
  send_cmode(service, chptr, "-v", client->name);
}

void
invite_user(struct Service *service, struct Channel *chptr, Client *client)
{
  execute_callback(send_invite_cb, me.uplink, service, chptr, client);
}

void
ban_mask(struct Service *service, struct Channel *chptr, const char *mask)
{
  Client *client = find_client(service->name);

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
identify_user(Client *client)
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

  client->num_badpass = 0;

  execute_callback(on_identify_cb, me.uplink, client);
}

void
cloak_user(Client *client, char *cloak)
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
do_help(struct Service *service, Client *client, 
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
  ptr = (char *)MyMalloc(size+1);
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
    Client *client)
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
    struct Membership *ms = (struct Membership *)ptr->data;
    Client *client = ms->client_p;

    numkicks += enforce_client_serviceban(service, chptr, client, akick);
  }
  return numkicks;
}

int
enforce_client_serviceban(struct Service *service, struct Channel *chptr, 
    Client *client, struct ServiceBan *sban)
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
    Client *client, const char *lock, char **value)
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
    int k, l, s, d;
    char *lk = "";

    k = l = s = d = FALSE;

    if(setstr[0] != '\0')
      s = TRUE;
    if(delstr[0] != '\0');
      s = TRUE;
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

    if(!db_set_string(SET_CHAN_MLOCK, chptr->regchan->id, mlockbuf))
    {
      return FALSE;
    }
    replace_string(*value, mlockbuf);
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
  chptr->mode.limit = limit;
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
  nick->email = NULL;
  MyFree(nick->url);
  nick->url = NULL;
  MyFree(nick->last_quit);
  nick->last_quit = NULL;
  MyFree(nick->last_host);
  nick->last_host = NULL;
  MyFree(nick->last_realname);
  nick->last_realname = NULL;
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

#endif
service::service()
{
}

service::service(std::string const &n) 
{
  name = n;
}

void
service::introduce()
{
  if(name.length() == 0)
    throw std::runtime_error("Need a service name");

  client = new Client(name, "services", name, me->c_name());

  client->set_ts(CurrentTime);
  client->introduce();
}

void
service::notice_client(Client *client, unsigned int, std::string notice)
{
  
}
