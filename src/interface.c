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
#include "dbm.h"
#include "language.h"
#include "parse.h"
#include "nickserv.h"
#include "chanserv.h"
#include "interface.h"
#include "crypt.h"
#include "msg.h"
#include "hash.h"
#include "client.h"
#include "conf/servicesinfo.h"
#include "conf/mail.h"
#include "mqueue.h"
#include "hostmask.h"
#include "nickname.h"
#include "dbchannel.h"
#include "channel_mode.h"
#include "channel.h"

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
struct Callback *send_resv_cb;
struct Callback *send_unresv_cb;
struct Callback *send_newserver_cb;
struct Callback *send_join_cb;
struct Callback *send_part_cb;
struct Callback *send_nosuchsrv_cb;
struct Callback *send_chops_notice_cb;
struct Callback *send_squit_cb;

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
struct Callback *on_channel_created_cb;
struct Callback *on_channel_destroy_cb;
struct Callback *on_topic_change_cb;
struct Callback *on_privmsg_cb;
struct Callback *on_notice_cb;
struct Callback *on_burst_done_cb;
struct Callback *on_certfp_cb;
struct Callback *on_db_init_cb;

struct Callback *on_nick_reg_cb;
struct Callback *on_chan_reg_cb;
struct Callback *on_nick_drop_cb;
struct Callback *on_chan_drop_cb;

struct Callback *on_ctcp_cb;

struct Callback *do_event_cb;

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
  send_resv_cb        = register_callback("Send RESV", NULL);
  send_unresv_cb      = register_callback("Send UNRESV", NULL);
  send_newserver_cb   = register_callback("Introduce new server", NULL);
  send_join_cb        = register_callback("Send JOIN", NULL);
  send_part_cb        = register_callback("Send PART", NULL);
  send_nosuchsrv_cb   = register_callback("Send No such server", NULL);
  send_chops_notice_cb = register_callback("Send NOTICE to channel ops", NULL);
  send_squit_cb       = register_callback("Send SQUIT Message", NULL);
  on_nick_change_cb   = register_callback("Propagate NICK", NULL);
  on_join_cb          = register_callback("Propagate JOIN", NULL);
  on_part_cb          = register_callback("Propagate PART", NULL);
  on_quit_cb          = register_callback("Propagate QUIT", NULL);
  on_umode_change_cb  = register_callback("Propagate UMODE", NULL);
  on_cmode_change_cb  = register_callback("Propagate CMODE", NULL);
  on_squit_cb         = register_callback("Propagate SQUIT", NULL);
  on_identify_cb      = register_callback("Identify Callback", NULL);
  on_newuser_cb       = register_callback("New user coming to us", NULL);
  on_channel_created_cb = register_callback("Channel is being created", NULL);
  on_channel_destroy_cb = register_callback("Channel is being destroyed", NULL);
  on_topic_change_cb  = register_callback("Topic changed", NULL);
  on_privmsg_cb       = register_callback("Privmsg for channel received", NULL);
  on_notice_cb        = register_callback("Notice for channel received", NULL);
  on_burst_done_cb    = register_callback("Notification that burst is complete", 
      NULL);
  on_certfp_cb        = register_callback("Client certificate recieved for this user", NULL);
  on_nick_drop_cb     = register_callback("Nick Dropped", NULL);
  on_chan_drop_cb     = register_callback("Chan Dropped", NULL);
  on_db_init_cb       = register_callback("On Database Init", NULL);
  on_ctcp_cb          = register_callback("On CTCP Message", NULL);
  on_nick_reg_cb      = register_callback("Newly Registered Nick", NULL);
  on_chan_reg_cb      = register_callback("Newly Registered Chan", NULL);
  do_event_cb         = register_callback("Event Loop Callback", NULL);

  load_language(ServicesLanguages, "services.en");
}

void
cleanup_interface()
{
  BlockHeapDestroy(services_heap);
  unregister_callback(send_newuser_cb);
  unregister_callback(send_privmsg_cb);
  unregister_callback(send_notice_cb);
  unregister_callback(send_gnotice_cb);
  unregister_callback(send_umode_cb);
  unregister_callback(send_cloak_cb);
  unregister_callback(send_nick_cb);
  unregister_callback(send_akill_cb);
  unregister_callback(send_unakill_cb);
  unregister_callback(send_kick_cb);
  unregister_callback(send_cmode_cb);
  unregister_callback(send_invite_cb);
  unregister_callback(send_topic_cb);
  unregister_callback(send_kill_cb);
  unregister_callback(send_resv_cb);
  unregister_callback(send_unresv_cb);
  unregister_callback(send_newserver_cb);
  unregister_callback(send_join_cb);
  unregister_callback(send_part_cb);
  unregister_callback(send_nosuchsrv_cb);
  unregister_callback(send_chops_notice_cb);
  unregister_callback(send_squit_cb);
  unregister_callback(on_nick_change_cb);
  unregister_callback(on_join_cb);
  unregister_callback(on_part_cb);
  unregister_callback(on_quit_cb);
  unregister_callback(on_umode_change_cb);
  unregister_callback(on_cmode_change_cb);
  unregister_callback(on_identify_cb);
  unregister_callback(on_newuser_cb);
  unregister_callback(on_channel_created_cb);
  unregister_callback(on_channel_destroy_cb);
  unregister_callback(on_topic_change_cb);
  unregister_callback(on_privmsg_cb);
  unregister_callback(on_notice_cb);
  unregister_callback(on_burst_done_cb);
  unregister_callback(on_certfp_cb);
  unregister_callback(on_nick_drop_cb);
  unregister_callback(on_chan_drop_cb);
  unregister_callback(on_ctcp_cb);
  unregister_callback(on_nick_reg_cb);
  unregister_callback(on_chan_reg_cb);
  unregister_callback(do_event_cb);

  unload_languages(ServicesLanguages);
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

struct Client *
introduce_client(const char *name, const char *gecos, char isservice)
{
  struct Client *client;
  char *umode;
  dlink_node *ptr;


  client = find_client(name);
  if(client == NULL)
  {
    client = make_client(&me);
    client->firsttime = client->tsinfo = CurrentTime;
    dlinkAdd(client, &client->node, &global_client_list);

    /* copy the nick in place */
    strlcpy(client->name, name, sizeof(client->name));
    hash_add_client(client);

    register_remote_user(&me, client, "services", me.name, me.name, gecos);
  }

  /* If we are not connected yet, the service will be sent as part of burst */
  if(me.uplink != NULL)
  {
    if(isservice)
      umode = "oP";
    else
      umode = "P";

    execute_callback(send_newuser_cb, me.uplink, name, "services", me.name,
      gecos, umode);
    DLINK_FOREACH(ptr, client->channel.head)
    {
      struct Membership *ms = ptr->data;
      join_channel(client, ms->chptr);
    }
  }

  return client;
}

struct Client*
introduce_server(const char *name, const char *gecos)
{
  struct Client *client = find_server(name);
  ilog(L_DEBUG, "Introducing Server %s [%s]", name, gecos);

  if(client == NULL)
  {
    ilog(L_DEBUG, "Server %s not found", name);
    client = make_client(&me);
    SetServer(client);

    client->servptr = &me;
    dlinkAdd(client, &client->lnode, &client->servptr->server_list);

    client->tsinfo = CurrentTime;

    strlcpy(client->name, name, sizeof(client->name));
    strlcpy(client->info, gecos, sizeof(client->info));
    hash_add_client(client);

    dlinkAdd(client, &client->node, &global_client_list);
  }


  if(me.uplink != NULL)
  {
    ilog(L_DEBUG, "Sending New Server %s [%s]", client->name, client->info);
    execute_callback(send_newserver_cb, client);
  }
  else
  {
    ilog(L_DEBUG, "Waiting to introduce %s until connected", client->name);
  }

  return client;
}

struct Channel*
join_channel(struct Client *service, struct Channel *channel)
{
  if(channel != NULL)
  {
    add_user_to_channel(channel, service, 0, 0);
    execute_callback(send_join_cb, me.uplink, me.name, channel->chname,
      channel->channelts, 0, service->name);
    execute_callback(on_join_cb, service, channel->chname);
  }
  else
    ilog(L_DEBUG, "Trying to join to a null channel pointer");

  return channel;
}

void
squit_server(const char *server, const char *reason)
{
  struct Client *serv = find_server(server);
  if(serv != NULL)
  {
    execute_callback(send_squit_cb, &me, server, reason);
    exit_client(serv, &me, reason);
  }
}

void
part_channel(struct Client *service, const char *chname, const char *reason)
{
  execute_callback(send_part_cb, me.uplink, service, chname, reason);
}

void
ctcp_user(struct Service *service, struct Client *client, const char *text)
{
  char buffer[IRC_BUFSIZE];
  snprintf(buffer, IRC_BUFSIZE, "\001%s\001", text);
  execute_callback(send_privmsg_cb, me.uplink, service->name, client->name, buffer);
}

void
sendto_channel(struct Service *service, struct Channel *channel, const char *text)
{
  execute_callback(send_privmsg_cb, me.uplink, service->name, channel->chname, text);
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
    return 0;
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

  if(off == 0)
    reply_user(service, service, client, baseid+4);
  else if(diff.tm_year > 0)
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
  char *buf;
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
  vasprintf(&buf, langstr, ap);
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

  MyFree(buf);
}

void
reply_mail(struct Service *service, struct Client *client,
    unsigned int subjectid, unsigned int langid, ...)
{
  char *buf, *bufptr;
  char *langstr = NULL;
  char *subjectstr;
  struct LanguageFile *languages;
  va_list ap;
  FILE *ptr;
  int count = 0;

  if(service == NULL)
    languages = ServicesLanguages;
  else
    languages = service->languages;
  
  if(langid != 0)
  {
    langstr = languages[client->nickname->language].entries[langid];
    subjectstr = languages[client->nickname->language].entries[subjectid];
  }
  else
  {
    langstr = "%s";
    subjectstr = languages[client->nickname->language].entries[subjectid];
  }

  va_start(ap, langid);
  vasprintf(&buf, langstr, ap);
  va_end(ap);

  if((ptr = popen(Mail.command, "w")) == NULL)
  {
    MyFree(buf);
    return;
  }

  fprintf(ptr, "To: %s\n", client->nickname->email);
  fprintf(ptr, "From: %s\n", Mail.from_address);
  fprintf(ptr, "Subject: %s\n", subjectstr);
  fprintf(ptr, "Precedence: junk\n");

  bufptr = buf;
  while(*bufptr != '\0')
  {
    if(*bufptr == '\n')
    {
      bufptr++;
      if(*bufptr == '\n')
      {
        fputc('\n', ptr);
        count = 0;
      }
    }

    if(count == 72)
    {
      fputc('\n', ptr);
      count = -1;
    }
    fputc(*bufptr++, ptr);
    count++;
  }

  fputc('\n', ptr);
  MyFree(buf);
  pclose(ptr);
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
send_chops_notice(struct Service *service, struct Channel *chptr, 
    const char *format, ...)
{
  va_list ap;
  char *buf;

  if(chptr == NULL || chptr->regchan == NULL || !chptr->regchan->verbose)
    return;

  va_start(ap, format);
  vasprintf(&buf, format, ap);
  va_end(ap);

  execute_callback(send_chops_notice_cb, me.uplink, service, chptr, buf);
  MyFree(buf);
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
  {
    int duration = (akill->time_set + akill->duration) - CurrentTime;

    if(duration < 0)
    {
      ilog(L_CRIT, "Trying to akill expired ban on %s set %s", akill->mask, smalldate(akill->time_set));
      return;
    }

    if(duration < 60)
      duration = 60;

    execute_callback(send_akill_cb, me.uplink, service, setter, akill->mask, akill->reason, duration);
  }
}

void
send_resv(struct Service *service, char *resv, char *reason, time_t duration)
{
  ilog(L_DEBUG, "%s set RESV on %s for %ld because %s", service->name, resv, duration, reason);
  execute_callback(send_resv_cb, me.uplink, service, resv, reason, duration);
}

void
send_unresv(struct Service *service, char *resv)
{
  execute_callback(send_unresv_cb, me.uplink, service, resv);
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
  struct Membership *ms;
  struct Client *ptr = find_client(client);

  if(ServicesState.debugmode)
    return;

  if(IsMe(ptr->from))
    return;

  if((ms = find_channel_link(ptr, chptr)) == NULL)
  {
    ilog(L_CRIT, "Tried to remove %s from channel %s they werent on",
        ptr->name, chptr->chname);
    return;
  }

  execute_callback(send_kick_cb, me.uplink, service->name, chptr->chname, 
      client, reason);

  remove_user_from_channel(ms);
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

static void 
voicedevoice_user(struct Channel *chptr, struct Client *client, int voice)
{
  struct Membership *member;

  if ((member = find_channel_link(client, chptr)) == NULL)
    return;

  if((voice && has_member_flags(member, CHFL_VOICE)) || (!voice && 
      !has_member_flags(member, CHFL_VOICE)))
    return;

  if(voice)
    AddMemberFlag(member, CHFL_VOICE);
  else
    DelMemberFlag(member, CHFL_VOICE);
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
voice_user(struct Service *service, struct Channel *chptr,
    struct Client *client)
{
  if(ServicesState.debugmode)
    return;

  send_cmode(service, chptr, "+v", client->name);
  voicedevoice_user(chptr, client, 1);
}

void
devoice_user(struct Service *service, struct Channel *chptr, 
    struct Client *client)
{
  if(ServicesState.debugmode)
    return;

  send_cmode(service, chptr, "-v", client->name);
  voicedevoice_user(chptr, client, 0);
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
quiet_mask(struct Service *service, struct Channel *chptr, const char *mask)
{
  struct Client *client = find_client(service->name);
  if(ServicesState.debugmode)
    return;

  send_cmode(service, chptr, "+q", mask);
  add_id(client, chptr, (char *)mask, CHFL_QUIET);
}

void
unquiet_mask(struct Service *service, struct Channel *chptr, const char *mask)
{
  if(ServicesState.debugmode)
    return;

  send_cmode(service, chptr, "-q", mask);
  del_id(chptr, (char*)mask, CHFL_QUIET);
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
  if(*client->realhost == '\0')
    strlcpy(client->realhost, client->host, sizeof(client->realhost));
  strlcpy(client->host, cloak, sizeof(client->host));
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
      if((msg->access >= OPER_FLAG) && (client->access < msg->access))
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
      reply_user(service, NULL, client, SERV_SUB_HELP_NOT_AVAIL, command, parv[2]);
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
enforce_matching_serviceban(struct Service *service, struct Channel *chptr, 
    struct Client *client)
{
  struct ServiceBan *sban;
  void *ptr, *first;

  if(chptr != NULL && chptr->regchan == NULL)
    return FALSE;

  if(chptr != NULL)
    first = ptr = db_list_first(AKICK_LIST, chptr->regchan->id, (void**)&sban);

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
  }

  db_list_done(first);

  return FALSE;
}

int
enforce_akick(struct Service *service, struct Channel *chptr, 
    struct ServiceBan *akick)
{
  dlink_node *ptr;
  dlink_node *next_ptr;
  int numkicks = 0;

  DLINK_FOREACH_SAFE(ptr, next_ptr, chptr->members.head)
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
    char *nick = nickname_nick_from_id(sban->target, TRUE);
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
      case HM_IPV4:
        if (client->aftype == AF_INET)
          if (match_ipv4(&client->ip, &addr, bits))
            found = 1;
        break;
#ifdef IPV6
      case HM_IPV6:
        if (client->aftype == AF_INET6)
          if (match_ipv6(&client->ip, &addr, bits))
            found = 1;
        break;
#endif
    }
    if(found)
    {
      if(sban->type == AKICK_BAN)
      {
        char banmask[IRC_BUFSIZE+1];

        ircsprintf(banmask, "%s", sban->mask);
        ban_mask(service, chptr, banmask);
        kick_user(service, chptr, client->name, sban->reason);
      }
      else if(sban->type == AKILL_BAN)
      {
        char *setter = nickname_nick_from_id(sban->setter, TRUE);

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
set_mode_lock(struct Service *service, const char *channel, 
    struct Client *client, const char *lock, char **value)
{
  const char *parv[3] = { NULL, NULL, NULL };
  char *p;
  unsigned int setmodes, delmodes, currmode, mode;
  int i, dir, para = 1, limit;
  char *c;
  char key[KEYLEN+1];
  char modebuf[MODEBUFLEN+1], parabuf[MODEBUFLEN+1];
  char setstr[MODEBUFLEN/2+1], delstr[MODEBUFLEN/2+1]; 
  char mlockbuf[MODEBUFLEN+1];
  struct RegChannel *regchptr;
  struct Channel *chptr;
  int k, l, s, d;

  k = l = s = d = 0;

  setmodes = delmodes = dir = limit = 0;
  memset(key, 0, sizeof(key));

  chptr = hash_find_channel(channel);
  regchptr = chptr == NULL ? dbchannel_find(channel) : chptr->regchan;

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
     * protocol's mode list because they take parameters, but we can MLOCK
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
        else
          l = -1;
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
        else
          k = -1;
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

  if(l < 0)
    strlcat(delstr, "l", MODEBUFLEN/2);

  if(k < 0)
    strlcat(delstr, "k", MODEBUFLEN/2);

  ilog(L_DEBUG, "MLOCK -%s+%s", delstr, setstr);

  /* If we've been asked to update the db, then we should do so. */
  if(value != NULL)
  {
    char *lk = "";

    s = d = FALSE;

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
        s || l > 0 || k > 0 ? "+": "", 
        s ? setstr : "", 
        lk,
        d ? "-" : "",
        d ? delstr : "",
        l > 0 || k > 0? parabuf : "");

    if(regchptr->autolimit && l < 0)
      regchptr->autolimit = FALSE;

    if(!db_set_string(SET_CHAN_MLOCK, regchptr->id, 
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

  // Channel doesnt exist on the network, skip this
  if(chptr == NULL)
  {
    free_regchan(regchptr);
    return TRUE;
  }

  /* Now only set the mode that needs to be set */
  c = setstr;
  while(*c != '\0')
  {
    mode = get_mode_from_letter(*c);
    if(mode <= 0)
    {
      c++;
      continue;
    }
    if(chptr->mode.mode & mode)
      setmodes &= ~mode;
    c++;
  }

  c = delstr;
  while(*c != '\0')
  {
    if(*c == 'l')
    {
      chptr->mode.limit = 0;
      c++;
      continue;
    }

    if(*c == 'k')
    {
      chptr->mode.key[0] = '\0';
      c++;
      continue;
    }

    mode = get_mode_from_letter(*c);
    if(mode <= 0)
    {
      c++;
      continue;
    }
    if(!((chptr->mode.mode & mode)))
      delmodes &= ~mode;
    c++;
  }

  get_modestring(setmodes, setstr, MODEBUFLEN/2);
  get_modestring(delmodes, delstr, MODEBUFLEN/2);
  chptr->mode.mode |= setmodes;
  chptr->mode.mode &= ~delmodes;

  if(l > 0)
    chptr->mode.limit = limit;
  else if (l < 0)
    strlcat(delstr, "l", MODEBUFLEN/2);

  if(k > 0)
    strcpy(chptr->mode.key, key);
  else if (l < 0)
    strlcat(delstr, "k", MODEBUFLEN/2);

  /*
   * Set up the modestring and parameter(s) and set them. This could probably
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
  MyFree(regchptr->mlock);
  mqueue_hash_free(regchptr->flood_hash, &regchptr->flood_list);
  regchptr->flood_hash = NULL;
  mqueue_free(regchptr->gqueue);
  regchptr->gqueue = NULL;
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
  MyFree(ban);
}

void
free_jupeentry(struct JupeEntry *entry)
{
	ilog(L_DEBUG, "Freeing JupeEntry %p for %s", entry, entry->name);
	MyFree(entry->name);
	MyFree(entry->reason);
  entry->name = NULL;
  entry->reason = NULL;
	MyFree(entry);
}

int 
check_nick_pass(struct Client *client, struct Nick *nick, const char *password)
{
  char fullpass[PASSLEN*2+1];
  char *pass;
  int ret;

  assert(nick);
  assert(nick->salt);

  if(*client->certfp != '\0')
  {
    if(nickname_cert_check(nick, client->certfp))
      return 1;
  }
  
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

int
valid_wild_card(const char *arg) 
{
  char tmpch;
  int nonwild = 0;
  int anywild = 0;

  /*
   * Now we must check the user and host to make sure there
   * are at least NONWILDCHARS non-wildcard characters in
   * them, otherwise assume they are attempting to kline
   * *@* or some variant of that. This code will also catch
   * people attempting to kline *@*.tld, as long as NONWILDCHARS
   * is greater than 3. In that case, there are only 3 non-wild
   * characters (tld), so if NONWILDCHARS is 4, the kline will
   * be disallowed.
   * -wnder
   */
  while ((tmpch = *arg++))
  {
    if (!IsKWildChar(tmpch))
    {
      /*
       * If we find enough non-wild characters, we can
       * break - no point in searching further.
       */
      if (++nonwild >= ServicesInfo.min_nonwildcard)
        return 1;
    }
    else
      anywild = 1;
  }

  /* There are no wild characters in the ban, allow it */
  if(!anywild)
    return 1;

  return 0;
}

char *
check_masterless_channels(unsigned int accid)
{
  struct InfoChanList *chan;
  void *listptr, *first;

  first = listptr = NULL;

  if((listptr = db_list_first(NICKCHAN_LIST, accid, (void**)&chan)) != NULL)
  {
    first = listptr;

    while(listptr != NULL)
    {
      if(db_get_num_channel_accesslist_entries(chan->channel_id) == 1)
      {
        char *nick = nickname_nick_from_id(accid, TRUE);
        struct Channel *chptr;

        ilog(L_NOTICE, "Dropping channel %s because its access list would be "
            "left empty by drop of nickname %s", chan->channel, nick);
        MyFree(nick);
//        dbchannel_delete(chan);

        chptr = hash_find_channel(chan->channel);
        if(chptr != NULL)
        {
          if(chptr->regchan != NULL)
          {
            free_regchan(chptr->regchan);
            chptr->regchan = NULL;
          }
        }

        MyFree(chan);
        listptr = db_list_next(listptr, NICKCHAN_LIST, (void**)&chan);
        continue;
      }
      if(chan->ilevel == MASTER_FLAG && db_get_num_masters(chan->channel_id) <= 1)
      {
        char *cname;
        
        DupString(cname, chan->channel);
        db_list_done(listptr);

        MyFree(chan);
        return cname;
      }
      MyFree(chan);
      listptr = db_list_next(listptr, NICKCHAN_LIST, (void**)&chan);
    }
    MyFree(chan);
    db_list_done(first);
  }
  else
  {
    MyFree(chan);
    if(listptr != NULL)
      db_list_done(listptr);
  }

  return 0;
}

int
drop_nickname(struct Service *service, struct Client *client, const char *target)
{
  char *channel;
  struct Nick *nick = nickname_find(target);

  if((channel = check_masterless_channels(nick->id)) != NULL)
  {
    if(client != NULL)
    {
      /* TODO This should come from services language file */
      reply_user(service, service, client, NS_DROP_FAIL_MASTERLESS, target, channel);
    }
    else
    {
      ilog(L_NOTICE, "%s Failed to drop %s because %s would have no master", service->name, target, channel);
    }
    MyFree(channel);
    return 0;
  }

  if(nickname_delete(nick))
  {
    struct Client *user = find_client(target);

    if(user != NULL)
    {
      ClearIdentified(user);
      if(user->nickname != NULL)
        free_nick(user->nickname);
      user->nickname = NULL;
      user->access = USER_FLAG;
      send_umode(service, user, "-R");
    }

    if(client != NULL)
    {
      ilog(L_NOTICE, "%s!%s@%s dropped nick %s", client->name, client->username,
        client->host, target);
    }
    else
    {
      ilog(L_NOTICE, "%s dropped nick %s", service->name, target);
    }

    return 1;
  }

  return 0;
}

