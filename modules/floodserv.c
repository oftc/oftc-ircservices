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
 *  $Id: floodservc 864 2007-04-19 00:02:51Z tjfontaine $
 */

/*
 * FloodServ is a very unintelligent beast, and has the potential for mass annoyance
 *
 * Basic data structure is a MessageQueue which keeps the list of messages and the
 * last time a message was added to the queue, and the allowable metrics for the
 * queue
 *
 * A FloodEntry which contains the TS and Message
 *
 * New messages are added to the end of the dlink_list and when the list has
 * more than the max entries it can contain it pops entries off the front
 *
 * For each user that is joined to a channel that FloodServ inhabits there exist
 * two queues, one per channel that they share with FloodServ and a larger global
 * queue. For each channel that has FloodServ there exists queue just for that
 * channel as well.
 *
 * Enforcement: when a new message arrives for a channel that FloodServ is in
 * the message is added to the users global queue and checked for network level
 * enforcement if this is triggered they're akilled from the network. The
 * message is then added to the channel queue and checked for enforcement in
 * which the offending host will be +q'd, after this the entry is added to the
 * uses queue and checked for enforcement and will also be +q'd.
 *
 * Every so often FloodServ runs a garbage collection routine and removes
 * old queues that haven't been used in a while. Also, FloodServ will unenforce
 * any +q's it determines it has set.
 */

#include "stdinc.h"
#include "nickname.h"
#include "dbchannel.h"
#include "client.h"
#include "dbm.h"
#include "language.h"
#include "parse.h"
#include "msg.h"
#include "interface.h"
#include "channel_mode.h"
#include "channel.h"
#include "conf/modules.h"
#include "hash.h"
#include "floodserv.h"
#include "mqueue.h"
#include "akill.h"
#include "servicemask.h"

static struct Service *floodserv = NULL;
static struct Client  *fsclient  = NULL;

static dlink_node *fs_join_hook;
static dlink_node *fs_part_hook;
static dlink_node *fs_channel_created_hook;
static dlink_node *fs_channel_destroy_hook;
static dlink_node *fs_privmsg_hook;
static dlink_node *fs_notice_hook;
static dlink_node *fs_chan_drop_hook;

static void *fs_on_client_join(va_list);
static void *fs_on_client_part(va_list);
static void *fs_on_channel_created(va_list);
static void *fs_on_channel_destroy(va_list);
static void *fs_on_privmsg(va_list);
static void *fs_on_chan_drop(va_list);

static void floodserv_gc_routine(void *);
static void floodserv_gc_hash(struct MessageQueue **, dlink_list *,
  const char *);

static void floodserv_unenforce_routine(void *);
static void floodserv_cleanup_channels(void *);

static void m_help(struct Service *, struct Client *, int, char *[]);

static void setup_channel(struct Channel *);

static void mqueue_add_message(struct MessageQueue *, const char *);
static int mqueue_enforce(struct MessageQueue *);

static void floodserv_free_channels();
static void floodserv_free_channel(DBChannel *);

static struct MessageQueue **global_msg_queue;
static dlink_list global_msg_list;

static struct ServiceMessage help_msgtab = {
  NULL, "HELP", 0, 0, 2, SFLG_UNREGOK, CHUSER_FLAG, FS_HELP_SHORT,
  FS_HELP_LONG, m_help
};

INIT_MODULE(floodserv, "$Revision$")
{
  dlink_node *ptr = NULL, *next_ptr = NULL;
  int i;

  floodserv = make_service("FloodServ");
  clear_serv_tree_parse(&floodserv->msg_tree);
  dlinkAdd(floodserv, &floodserv->node, &services_list);
  hash_add_service(floodserv);
  fsclient = introduce_client(floodserv->name, floodserv->name, TRUE);
  load_language(floodserv->languages, "floodserv.en");

  mod_add_servcmd(&floodserv->msg_tree, &help_msgtab);

  fs_join_hook = install_hook(on_join_cb, fs_on_client_join);
  fs_part_hook = install_hook(on_part_cb, fs_on_client_part);
  fs_channel_created_hook = install_hook(on_channel_created_cb, fs_on_channel_created);
  fs_channel_destroy_hook = install_hook(on_channel_destroy_cb, fs_on_channel_destroy);
  fs_privmsg_hook = install_hook(on_privmsg_cb, fs_on_privmsg);
  fs_notice_hook = install_hook(on_notice_cb, fs_on_privmsg);
  fs_chan_drop_hook = install_hook(on_chan_drop_cb, fs_on_chan_drop);

  DLINK_FOREACH_SAFE(ptr, next_ptr, global_channel_list.head)
    setup_channel(ptr->data);

  global_msg_queue = new_mqueue_hash();
  for(i = 0; i < HASHSIZE; ++i)
    global_msg_queue[i] = NULL;

  eventAdd("floodserv gc routine", floodserv_gc_routine, NULL, FS_GC_EVENT_TIMER);
  eventAdd("floodserv unenforce routine", floodserv_unenforce_routine, NULL, 10);
  eventAdd("floodserv cleanup channels", floodserv_cleanup_channels, NULL, 60);
  return floodserv;
}

CLEANUP_MODULE
{
  uninstall_hook(on_join_cb, fs_on_client_join);
  uninstall_hook(on_part_cb, fs_on_client_part);
  uninstall_hook(on_channel_created_cb, fs_on_channel_created);
  uninstall_hook(on_channel_destroy_cb, fs_on_channel_destroy);
  uninstall_hook(on_privmsg_cb, fs_on_privmsg);
  uninstall_hook(on_notice_cb, fs_on_privmsg);

  serv_clear_messages(floodserv);

  mqueue_hash_free(global_msg_queue, &global_msg_list);
  floodserv_free_channels();
  global_msg_queue = NULL;

  unload_languages(floodserv->languages);

  eventDelete(floodserv_gc_routine, NULL);
  eventDelete(floodserv_unenforce_routine, NULL);
  eventDelete(floodserv_cleanup_channels, NULL);

  exit_client(fsclient, &me, "Service unloaded");
  hash_del_service(floodserv);
  dlinkDelete(&floodserv->node, &services_list);
  ilog(L_DEBUG, "Unloaded floodserv");
}

static void
floodserv_cleanup_channels(void *param)
{
  dlink_node *ptr = NULL, *next_ptr = NULL;

  DLINK_FOREACH_SAFE(ptr, next_ptr, fsclient->channel.head)
  {
    struct Membership *ms = ptr->data;
    struct Channel *chptr = ms->chptr;

    if(chptr->members.length == 1)
    {
      ilog(L_DEBUG, "%s should be removed from %s", fsclient->name, chptr->chname);
      part_channel(fsclient, chptr->chname, "");
    }
  }
}

static void
floodserv_unenforce_routine(void *param)
{
  dlink_node *ptr;
  dlink_node *bptr, *bnptr;

  DLINK_FOREACH(ptr, fsclient->channel.head)
  {
    struct Membership *ms = ptr->data;
    struct Channel *chptr = ms->chptr;
    dlink_list quiet_masks = { 0 };

    if(chptr->regchan != NULL && !dbchannel_get_expirebans(chptr->regchan))
    {
      DLINK_FOREACH_SAFE(bptr, bnptr, chptr->quietlist.head)
      {
        struct Ban *banptr = bptr->data;
        char ban[IRC_BUFSIZE+1];
        time_t delta = CurrentTime - banptr->when;
        time_t maxtime = 1*60*60;

        if(delta > maxtime && ircncmp(banptr->who, fsclient->name, strlen(fsclient->name)) ==0)
        {
          snprintf(ban, IRC_BUFSIZE, "%s!%s@%s", banptr->name,
            banptr->username, banptr->host);
          ilog(L_DEBUG, "FloodServ: UNENFORCE %s %d %s", chptr->chname,
            (int)delta, ban);
          dlinkAdd(ban, make_dlink_node(), &quiet_masks);
        }
      }
    }

    unquiet_mask_many(floodserv, chptr, &quiet_masks);
    db_string_list_free(&quiet_masks);
  }
}

static void
floodserv_gc_routine(void *param)
{
  dlink_node *ptr = NULL, *next_ptr = NULL;

  DLINK_FOREACH_SAFE(ptr, next_ptr, fsclient->channel.head)
  {
    struct Membership *ms = ptr->data;
    struct Channel *chptr = ms->chptr;

    if(chptr != NULL)
    {
      /* HUH? */
      if(chptr->regchan != NULL)
        floodserv_gc_hash(dbchannel_get_flood_hash(chptr->regchan),
          dbchannel_get_flood_list(chptr->regchan), chptr->chname);

    }
  }

  if(global_msg_queue != NULL)
    floodserv_gc_hash(global_msg_queue, &global_msg_list, "GLOBAL");
}

static void
floodserv_gc_hash(struct MessageQueue **hash, dlink_list *list, const char *name)
{
  dlink_node *ptr = NULL, *next_ptr = NULL;
  struct MessageQueue *queue = NULL;
  int age = 0;

  DLINK_FOREACH_SAFE(ptr, next_ptr, list->head)
  {
    queue = ptr->data;
    age = CurrentTime - queue->last_used;
    if(age >= FS_GC_EXPIRE_TIME)
    {
      ilog(L_DEBUG, "FloodServ GC Freeing %s in %s age: %d", queue->name, name, age);
      hash_del_mqueue(hash, queue);
      dlinkDelete(ptr, list);
      mqueue_free(queue);
    }
    queue = NULL;
  }
}

static void
floodserv_free_channels()
{
  dlink_node *ptr = NULL, *next_ptr = NULL;
  struct Channel *chptr = NULL;

  DLINK_FOREACH_SAFE(ptr, next_ptr, global_channel_list.head)
  {
    chptr = ptr->data;
    if(chptr->regchan != NULL && dbchannel_get_flood_hash(chptr->regchan) != NULL)
      floodserv_free_channel(chptr->regchan);
  }
}

static void
floodserv_free_channel(DBChannel *chptr)
{
  mqueue_hash_free(dbchannel_get_flood_hash(chptr), dbchannel_get_flood_list(chptr));
  dbchannel_set_flood_hash(chptr, NULL);
  mqueue_free(dbchannel_get_gqueue(chptr));
  dbchannel_set_gqueue(chptr, NULL);
}

static void
setup_channel(struct Channel *chptr)
{
  DBChannel *regchan = chptr->regchan == NULL ? dbchannel_find(chptr->chname) : chptr->regchan;

  if(regchan != NULL && dbchannel_get_floodserv(regchan) && !IsMember(fsclient, chptr))
  {
    if(dbchannel_get_flood_hash(regchan) == NULL)
      dbchannel_set_flood_hash(regchan, new_mqueue_hash());

    if(!IsMember(fsclient, chptr))
      join_channel(fsclient, chptr);

    if(dbchannel_get_gqueue(regchan) == NULL)
      dbchannel_set_gqueue(regchan, mqueue_new(chptr->chname, MQUEUE_GLOB, FS_GMSG_COUNT,
        FS_GMSG_TIME, 0));
  }

  if(regchan != NULL && regchan != chptr->regchan)
    dbchannel_free(regchan);
}

static void
mqueue_add_message(struct MessageQueue *queue, const char *message)
{
  struct FloodMsg *entry = NULL;

  assert(queue->name != NULL);

  /* We have too many messages, remove the oldest which is at the head */
  if(queue->entries.length >= queue->max)
  {
    entry = queue->entries.head->data;
    dlinkDelete(queue->entries.head, &queue->entries);
    floodmsg_free(entry);
  }

  /* Add new messages to the end of the queue */
  dlinkAddTail(floodmsg_new(message), make_dlink_node(), &queue->entries);

  queue->last_used = CurrentTime;
}

static int
mqueue_enforce(struct MessageQueue *queue)
{
  struct FloodMsg *oldest = NULL, *newest = NULL, *tmp = NULL;
  time_t age = 0;
  int enforce = 1;

  /* Old entries at the head, New at the tail */
  oldest = queue->entries.head->data;
  newest = queue->entries.tail->data;

  /* we don't have enough entries to worry about checking for violators */
  if(queue->entries.length < queue->max
    || oldest == NULL || newest == NULL
    || oldest == newest)
    return MQUEUE_NONE;

  assert(newest != oldest);

  /* if you don't have the order right, this will always be true */
  age = newest->time - oldest->time;

/*  if(queue->type != MQUEUE_GLOB && age <= queue->lne_enforce_time)
    return MQUEUE_LINE;*/

  if(age <= queue->msg_enforce_time)
  {
    dlink_node *ptr = NULL;

    DLINK_FOREACH(ptr, queue->entries.head)
    {
      tmp = ptr->data;
      if(ircncmp(newest->message, tmp->message, IRC_BUFSIZE) != 0)
      {
        enforce = 0;
        break;
      }
    }
  }
  else
    return MQUEUE_NONE;

  if(enforce)
    return MQUEUE_MESG;
  else
    return MQUEUE_NONE;
}

static void
m_help(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  do_help(service, client, parv[1], parc, parv);
}

static void *
fs_on_client_join(va_list args)
{
  struct Client *client = va_arg(args, struct Client *);
  char          *name   = va_arg(args, char *);
  struct Channel *channel = hash_find_channel(name);

  if(ircncmp(client->name, fsclient->name, NICKLEN) == 0)
  {
    if(channel != NULL)
    {
      ilog(L_DEBUG, "FloodServ added to %s", name);
      setup_channel(channel);
    }
    else
      ilog(L_CRIT, "FloodServ added to %s but channel doesn't exist!", name);
  }

  /* TODO Join flood metrics */

  return pass_callback(fs_join_hook, client, name);
}

static void *
fs_on_chan_drop(va_list args)
{
  const char *chan = va_arg(args, const char *);

  struct Channel *channel = hash_find_channel(chan);

  if(channel != NULL && IsMember(fsclient, channel))
  {
    part_channel(fsclient, channel->chname, "");
  }

  return pass_callback(fs_chan_drop_hook, chan);
}

static void *
fs_on_client_part(va_list args)
{
  struct Client* client = va_arg(args, struct Client *);
  struct Client* source = va_arg(args, struct Client *);
  struct Channel* channel = va_arg(args, struct Channel *);
  char *reason = va_arg(args, char *);

  if(ircncmp(source->name, fsclient->name, NICKLEN) == 0)
  {
    ilog(L_DEBUG, "FloodServ removed from %s by %s", channel->chname,
      source->name);
    /* TODO The channel may already be destroyed we need to find out why */
    if(channel != NULL && channel->regchan != NULL
      && dbchannel_get_flood_hash(channel->regchan) != NULL)
    {
      floodserv_free_channel(channel->regchan);
    }
  }
  else
  {
    /* TODO The channel may already be destroyed we need to find out why */
    if(channel != NULL && channel->regchan != NULL
      && dbchannel_get_flood_hash(channel->regchan) != NULL)
    {
      if(channel->members.length <= 2 && IsMember(fsclient, channel))
        part_channel(fsclient, channel->chname, "");
    }
  }

  /* TODO Join/Part flood metrics */

  return pass_callback(fs_part_hook, client, source, channel, reason);
}

static void *
fs_on_channel_created(va_list args)
{
  struct Channel *chptr = va_arg(args, struct Channel *);

  setup_channel(chptr);

  return pass_callback(fs_channel_created_hook, chptr);
}

static void *
fs_on_channel_destroy(va_list args)
{
  struct Channel *chan = va_arg(args, struct Channel *);

  if(chan->regchan != NULL && dbchannel_get_flood_hash(chan->regchan) != NULL)
    floodserv_free_channel(chan->regchan);

  return pass_callback(fs_channel_destroy_hook, chan);
}

static void *
fs_on_privmsg(va_list args)
{
  struct Client *source = va_arg(args, struct Client *);
  struct Channel *channel = va_arg(args, struct Channel *);
  char *message = va_arg(args, char *);
  struct MessageQueue *queue = NULL, *gqueue = NULL;
  struct ServiceMask *akill;
  int enforce = MQUEUE_NONE;
  char mask[IRC_BUFSIZE+1];
  char host[HOSTLEN+1];

  /* only continue if this is a floodserv channel */
  if(channel->regchan != NULL && dbchannel_get_flood_hash(channel->regchan) != NULL &&
    !IsOper(source))
  {
    /* if they have a realhost this means they have a cloak, we want their realhost */
    if(*source->realhost != '\0')
      strlcpy(host, source->realhost, sizeof(host));
    else
      strlcpy(host, source->host, sizeof(host));

    /* per user per channel queue */
    queue = hash_find_mqueue_host(dbchannel_get_flood_hash(channel->regchan), host);
    /* Network wide queue will result in an akill if enforced */
    gqueue = hash_find_mqueue_host(global_msg_queue, host);

    if(queue == NULL)
    {
      queue = mqueue_new(host, MQUEUE_CHAN, FS_MSG_COUNT, FS_MSG_TIME,
        FS_LNE_TIME);
      hash_add_mqueue(dbchannel_get_flood_hash(channel->regchan), queue);
      dlinkAdd(queue, &queue->node, dbchannel_get_flood_list(channel->regchan));
    }

    if(gqueue == NULL)
    {
      gqueue = mqueue_new(host, MQUEUE_GLOB, FS_GMSG_COUNT, FS_GMSG_TIME, 0);
      hash_add_mqueue(global_msg_queue, gqueue);
      dlinkAdd(gqueue, &gqueue->node, &global_msg_list);
    }

    mqueue_add_message(gqueue, message); /* add the message to the global queue */
    enforce = mqueue_enforce(gqueue); /* check for network enforcement */

    switch(enforce)
    {
      case MQUEUE_MESG:
        ilog(L_NOTICE, "Flood %s@%s TRIGGERED NETWORK MSG FLOOD Message: %s [%s]",
          source->name, host, message, channel->chname);
        snprintf(mask, IRC_BUFSIZE, "*@%s", host);

        if((akill = akill_find(mask)) != NULL)
        {
          ilog(L_NOTICE, "Flood AKILL Already Exists");
          free_servicemask(akill);
          return pass_callback(fs_privmsg_hook, source, channel, message);
        }

        akill = MyMalloc(sizeof(struct ServiceMask));

        akill->setter = 0;
        akill->time_set = CurrentTime;
        akill->duration = FS_KILL_DUR;
        DupString(akill->mask, mask);
        DupString(akill->reason, FS_KILL_MSG);

        akill_add(akill);
        send_akill(floodserv, fsclient->name, akill);

        if(akill != NULL)
          free_servicemask(akill);

        return pass_callback(fs_privmsg_hook, source, channel, message);
        break;
    }

    /* this is the channel 'global' queue if 6 people say the same thing this will trigger */
    mqueue_add_message(dbchannel_get_gqueue(channel->regchan), message);
    enforce = mqueue_enforce(dbchannel_get_gqueue(channel->regchan));

    switch(enforce)
    {
      case MQUEUE_MESG:
        ilog(L_NOTICE, "Flood %s@%s TRIGGERED CHANNEL MSG FLOOD in %s Message: %s",
          source->name, source->host, channel->chname, message);
        snprintf(mask, IRC_BUFSIZE, "*!*@%s", source->host);
        quiet_mask(floodserv, channel, mask);
        return pass_callback(fs_privmsg_hook, source, channel, message);
        break;
    }

    /* Finallly add the message to the per user per channel queue */
    mqueue_add_message(queue, message);
    enforce = mqueue_enforce(queue);
    switch(enforce)
    {
      case MQUEUE_LINE:
        ilog(L_NOTICE, "Flood %s@%s TRIGGERED LINE FLOOD in %s", source->name,
          source->host, channel->chname);
        snprintf(mask, IRC_BUFSIZE, "*!*@%s", source->host);
        quiet_mask(floodserv, channel, mask);
        break;
      case MQUEUE_MESG:
        ilog(L_NOTICE, "Flood %s@%s TRIGGERED MESSAGE FLOOD in %s Message: %s",
          source->name, source->host, channel->chname, message);
        snprintf(mask, IRC_BUFSIZE, "*!*@%s", source->host);
        quiet_mask(floodserv, channel, mask);
        break;
    }
  }

  return pass_callback(fs_privmsg_hook, source, channel, message);
}
