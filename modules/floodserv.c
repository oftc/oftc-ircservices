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

#include "stdinc.h"

static struct Service *floodserv = NULL;
static struct Client  *fsclient  = NULL;

static dlink_node *fs_join_hook;
static dlink_node *fs_part_hook;
static dlink_node *fs_channel_created_hook;
static dlink_node *fs_channel_destroy_hook;
static dlink_node *fs_privmsg_hook;
static dlink_node *fs_notice_hook;

static void *fs_on_client_join(va_list);
static void *fs_on_client_part(va_list);
static void *fs_on_channel_created(va_list);
static void *fs_on_channel_destroy(va_list);
static void *fs_on_privmsg(va_list);
static void *fs_on_notice(va_list);

static void floodserv_gc_routine(void *);

static void m_help(struct Service *, struct Client *, int, char *[]);

static void setup_channel(struct Channel *);

static struct MessageQueue *mqueue_new(const char *, unsigned int, int, int,
  int);
static void mqueue_add_message(struct MessageQueue *, const char *);
static int mqueue_enforce(struct MessageQueue *);
static void mqueue_hash_free(struct MessageQueue **, dlink_list);
static void mqueue_free(struct MessageQueue *);

static void floodmsg_free(struct FloodMsg *);
static struct FloodMsg *floodmsg_new(const char *);

static void floodserv_free_channels();
static void floodserv_free_channel(struct RegChannel *);

static struct MessageQueue **global_msg_queue;
static dlink_list global_msg_list;

static struct ServiceMessage help_msgtab = {
  NULL, "HELP", 0, 0, 2, SFLG_UNREGOK, CHUSER_FLAG, FS_HELP_SHORT,
  FS_HELP_LONG, m_help
};

INIT_MODULE(floodserv, "$Revision: 864 $")
{
  dlink_node *ptr = NULL, *next_ptr = NULL;
  int i;

  floodserv = make_service("FloodServ");
  clear_serv_tree_parse(&floodserv->msg_tree);
  dlinkAdd(floodserv, &floodserv->node, &services_list);
  hash_add_service(floodserv);
  fsclient = introduce_client(floodserv->name);
  load_language(floodserv->languages, "floodserv.en");

  mod_add_servcmd(&floodserv->msg_tree, &help_msgtab);

  fs_join_hook = install_hook(on_join_cb, fs_on_client_join);
  fs_part_hook = install_hook(on_part_cb, fs_on_client_part);
  fs_channel_created_hook = install_hook(on_channel_created_cb, fs_on_channel_created);
  fs_channel_destroy_hook = install_hook(on_channel_destroy_cb, fs_on_channel_destroy);
  fs_privmsg_hook = install_hook(on_privmsg_cb, fs_on_privmsg);
  fs_notice_hook = install_hook(on_notice_cb, fs_on_notice);

  DLINK_FOREACH_SAFE(ptr, next_ptr, global_channel_list.head)
    setup_channel(ptr->data);

  global_msg_queue = new_mqueue_hash();
  for(i = 0; i < HASHSIZE; ++i)
    global_msg_queue[i] = NULL;

  eventAdd("floodserv gc routine", floodserv_gc_routine, NULL, FS_GC_EVENT_TIMER);
}

CLEANUP_MODULE
{
  uninstall_hook(on_join_cb, fs_on_client_join);
  uninstall_hook(on_part_cb, fs_on_client_part);
  uninstall_hook(on_channel_created_cb, fs_on_channel_created);
  uninstall_hook(on_channel_destroy_cb, fs_on_channel_destroy);
  uninstall_hook(on_privmsg_cb, fs_on_privmsg);
  uninstall_hook(on_notice_cb, fs_on_notice);

  serv_clear_messages(floodserv);

  mqueue_hash_free(global_msg_queue, global_msg_list);
  floodserv_free_channels();
  global_msg_queue = NULL;

  unload_languages(floodserv->languages);

  eventDelete(floodserv_gc_routine, NULL);

  exit_client(find_client(floodserv->name), &me, "Service unloaded");
  hash_del_service(floodserv);
  dlinkDelete(&floodserv->node, &services_list);
  ilog(L_DEBUG, "Unloaded floodserv");
}

static void
floodserv_gc_routine(void *param)
{
  dlink_node *ptr = NULL, *next_ptr = NULL;
  dlink_node *qptr = NULL, *qnext_ptr = NULL;
  struct Channel *chptr = NULL;
  struct MessageQueue *queue = NULL;
  int age = 0;

  ilog(L_DEBUG, "FloodServ GC Routine Invoked");

  DLINK_FOREACH_SAFE(ptr, next_ptr, global_channel_list.head)
  {
    chptr = ptr->data;
    if(chptr->regchan != NULL && chptr->regchan->floodserv
      && chptr->regchan->flood_hash != NULL
      && chptr->regchan->flood_list.length > FS_GC_LIST_LENGTH)
    {
      ilog(L_DEBUG, "FloodServ GC Iterating %s MessageQueue", chptr->chname);
      DLINK_FOREACH_SAFE(qptr, qnext_ptr, chptr->regchan->flood_list.head)
      {
        queue = qptr->data;
        age = CurrentTime - queue->last_used;
        if(age >= FS_GC_EXPIRE_TIME)
        {
          ilog(L_DEBUG, "FloodServ GC Freeing %s for %s age: %d", queue->name,
            chptr->chname, age);
          hash_del_mqueue(chptr->regchan->flood_hash, queue);
          dlinkDelete(qptr, &chptr->regchan->flood_list);
          mqueue_free(queue);
        }
      }
    }
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
    if(chptr->regchan != NULL && chptr->regchan->flood_hash != NULL)
      floodserv_free_channel(chptr->regchan);
  }
}

static void
floodserv_free_channel(struct RegChannel *chptr)
{
  mqueue_hash_free(chptr->flood_hash, chptr->flood_list);
  chptr->flood_hash = NULL;
  mqueue_free(chptr->gqueue);
  chptr->gqueue = NULL;
}

static void
setup_channel(struct Channel *chptr)
{
  struct RegChannel *regchan = chptr->regchan == NULL ? db_find_chan(chptr->chname) : chptr->regchan;

  if(regchan != NULL && regchan->floodserv)
  {
    if(regchan->flood_hash == NULL)
      regchan->flood_hash = new_mqueue_hash();

    if(!IsMember(fsclient, chptr))
      join_channel(fsclient, chptr);

    if(regchan->gqueue == NULL)
      regchan->gqueue = mqueue_new(chptr->chname, MQUEUE_GLOB, FS_GMSG_COUNT,
        FS_GMSG_TIME, 0);
  }
}

static struct MessageQueue *
mqueue_new(const char *name, unsigned int type, int max, int msg_time,
int lne_time)
{
  struct MessageQueue *queue;
  int i;
  queue = MyMalloc(sizeof(struct MessageQueue));

  DupString(queue->name, name);
  assert(queue->name != NULL);

  queue->last = max - 1;
  queue->max  = max;
  queue->msg_enforce_time = msg_time;
  queue->lne_enforce_time = lne_time;
  queue->type = type;

  queue->entries = MyMalloc(sizeof(struct FloodMsg *) * queue->max);

  for(i = 0; i < queue->max; ++i)
    queue->entries[i] = NULL;

  assert(queue->name != NULL);
  return queue;
}

static void
mqueue_add_message(struct MessageQueue *queue, const char *message)
{
  struct FloodMsg *entry = NULL;
  int i;

  assert(queue->name != NULL);

  if(queue->last == 0 && queue->entries[0] != NULL)
  {
    entry = queue->entries[queue->max-1];
    for(i = queue->max - 1; i > 0; --i)
      queue->entries[i] = queue->entries[i - 1];
    floodmsg_free(entry);
    queue->entries[0] = NULL;
  }

  queue->entries[queue->last] = floodmsg_new(message);
  queue->last_used = CurrentTime;

  if(queue->last > 0)
    --queue->last;
}

static int
mqueue_enforce(struct MessageQueue *queue)
{
  struct FloodMsg *oldest = NULL, *newest = NULL, *tmp = NULL;
  time_t age = 0;
  int i = 0;
  int enforce = 1;

  oldest = queue->entries[queue->max-1];
  newest = queue->entries[0];

  if(queue->last != 0 || oldest == NULL || newest == NULL)
    return MQUEUE_NONE;

  assert(newest != oldest);

  age = newest->time - oldest->time;

  if(queue->type != MQUEUE_GLOB && age <= queue->lne_enforce_time)
    return MQUEUE_LINE;

  if(age <= queue->msg_enforce_time)
  {
    for(i = 0; i < queue->max; ++i)
    {
      tmp = queue->entries[i];
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
mqueue_hash_free(struct MessageQueue **hash, dlink_list list)
{
  dlink_node *ptr = NULL, *next_ptr = NULL;
  struct MessageQueue *queue;
  if(hash != NULL)
  {
    DLINK_FOREACH_SAFE(ptr, next_ptr, list.head)
    {
      queue = ptr->data;
      if(queue->name == NULL)
      {
        ilog(L_DEBUG, "Trying to free already free'd MessageQueue");
        abort();
      }
      assert(queue->name != NULL);
      hash_del_mqueue(hash, queue);
      dlinkDelete(ptr, &list);
      mqueue_free(queue);
    }
    MyFree(hash);
  }
}

static void
mqueue_free(struct MessageQueue *queue)
{
  int i;
  if(queue != NULL)
  {
    MyFree(queue->name);
    queue->name = NULL;

    for(i = 0; i < queue->max; ++i)
    {
      floodmsg_free(queue->entries[i]);
      queue->entries[i] = NULL;
    }

    MyFree(queue->entries);
    queue->entries = NULL;

    MyFree(queue);
  }
}

static void
floodmsg_free(struct FloodMsg *entry)
{
  if(entry != NULL)
  {
    MyFree(entry->message);
    entry->message = NULL;
    MyFree(entry);
  }
}

static struct FloodMsg *
floodmsg_new(const char *message)
{
  struct FloodMsg *entry = MyMalloc(sizeof(struct FloodMsg));
  entry->time = CurrentTime;
  DupString(entry->message, message);
  return entry;
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
fs_on_client_part(va_list args)
{
  struct Client* client = va_arg(args, struct Client *);
  struct Client* source = va_arg(args, struct Client *);
  struct Channel* channel = va_arg(args, struct Channel *);
  char *reason = va_arg(args, char *);

  if(ircncmp(client->name, fsclient->name, NICKLEN) == 0)
  {
    ilog(L_DEBUG, "FloodServ removed from %s by %s", channel->chname,
      source->name);
    if(channel->regchan != NULL && channel->regchan->flood_hash != NULL)
      floodserv_free_channel(channel->regchan);
  }
  else
  {
    if(channel->regchan != NULL && channel->regchan->flood_hash != NULL)
    {
      if(channel->members.length == 2 && IsMember(fsclient, channel))
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

  if(chan->regchan != NULL && chan->regchan->flood_hash != NULL)
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
  int enforce = MQUEUE_NONE;
  char mask[IRC_BUFSIZE+1];

  if(channel->regchan != NULL && channel->regchan->flood_hash != NULL)
  {
    queue = hash_find_mqueue_host(channel->regchan->flood_hash, source->host);
    gqueue = hash_find_mqueue_host(global_msg_queue, source->host);

    if(queue == NULL)
    {
      queue = mqueue_new(source->host,MQUEUE_CHAN, FS_MSG_COUNT, FS_MSG_TIME,
        FS_LNE_TIME);
      hash_add_mqueue(channel->regchan->flood_hash, queue);
      dlinkAdd(queue, &queue->node, &channel->regchan->flood_list);
    }

    if(gqueue == NULL)
    {
      gqueue = mqueue_new(source->host, MQUEUE_GLOB, FS_GMSG_COUNT,
        FS_GMSG_TIME, 0);
      hash_add_mqueue(global_msg_queue, gqueue);
      dlinkAdd(gqueue, &gqueue->node, &global_msg_list);
    }

    mqueue_add_message(gqueue, message);
    enforce = mqueue_enforce(gqueue);

    switch(enforce)
    {
      case MQUEUE_MESG:
        ilog(L_NOTICE, "%s@%s TRIGGERED NETWORK MSG FLOOD Message: %s",
          source->name, source->host, message);
        snprintf(mask, IRC_BUFSIZE, "*!*@%s", source->host);
        akill_add(floodserv, fsclient, mask, FS_KILL_MSG, FS_KILL_DUR);
        return pass_callback(fs_privmsg_hook, source, channel, message);
        break;
    }

    mqueue_add_message(channel->regchan->gqueue, message);
    enforce = mqueue_enforce(channel->regchan->gqueue);

    switch(enforce)
    {
      case MQUEUE_MESG:
        ilog(L_NOTICE, "%s@%s TRIGGERED CHANNEL MSG FLOOD Message: %s",
          source->name, source->host, message);
        snprintf(mask, IRC_BUFSIZE, "*!*@%s", source->host);
        quiet_mask(floodserv, channel, mask);
        return pass_callback(fs_privmsg_hook, source, channel, message);
        break;
    }

    mqueue_add_message(queue, message);
    enforce = mqueue_enforce(queue);
    switch(enforce)
    {
      case MQUEUE_LINE:
        ilog(L_NOTICE, "%s@%s TRIGGERED LINE FLOOD in %s", source->name,
          source->host, channel->chname);
        snprintf(mask, IRC_BUFSIZE, "*!*@%s", source->host);
        quiet_mask(floodserv, channel, mask);
        break;
      case MQUEUE_MESG:
        ilog(L_NOTICE, "%s@%s TRIGGERED MESSAGE FLOOD in %s Message: %s",
          source->name, source->host, channel->chname, message);
        snprintf(mask, IRC_BUFSIZE, "*!*@%s", source->host);
        quiet_mask(floodserv, channel, mask);
        break;
    }
  }

  return pass_callback(fs_privmsg_hook, source, channel, message);
}

static void *
fs_on_notice(va_list args)
{
  struct Client *source = va_arg(args, struct Client *);
  struct Channel *channel = va_arg(args, struct Channel *);
  char *message = va_arg(args, char *);

  /* TODO Notice flood metrics, should be same as privmsg */

  return pass_callback(fs_notice_hook, source, channel, message);
}
