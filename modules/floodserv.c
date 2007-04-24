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

static void m_help(struct Service *, struct Client *, int, char *[]);

static void setup_channel(struct Channel *);

static struct MessageQueue *mqueue_new(struct MessageQueue **, const char *,
  unsigned int);
static void mqueue_add_message(struct MessageQueue *, const char *);
static int mqueue_enforce(struct MessageQueue *);
static void mqueue_hash_free(struct Channel *);
static void mqueue_free(struct MessageQueue *);

static void floodmsg_free(struct FloodMsg *);
static struct FloodMsg *floodmsg_new(const char *);

static struct ServiceMessage help_msgtab = {
  NULL, "HELP", 0, 0, 2, SFLG_UNREGOK, CHUSER_FLAG, FS_HELP_SHORT,
  FS_HELP_LONG, m_help
};

INIT_MODULE(floodserv, "$Revision: 864 $")
{
  dlink_node *ptr = NULL, *next_ptr = NULL;

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
}

CLEANUP_MODULE
{
  dlink_node *ptr = NULL, *next_ptr = NULL;

  uninstall_hook(on_join_cb, fs_on_client_join);
  uninstall_hook(on_part_cb, fs_on_client_part);
  uninstall_hook(on_channel_created_cb, fs_on_channel_created);
  uninstall_hook(on_channel_destroy_cb, fs_on_channel_destroy);
  uninstall_hook(on_privmsg_cb, fs_on_privmsg);
  uninstall_hook(on_notice_cb, fs_on_notice);

  serv_clear_messages(floodserv);

  DLINK_FOREACH_SAFE(ptr, next_ptr, global_channel_list.head)
    mqueue_hash_free(ptr->data);

  unload_languages(floodserv->languages);

  exit_client(find_client(floodserv->name), &me, "Service unloaded");
  hash_del_service(floodserv);
  dlinkDelete(&floodserv->node, &services_list);
  ilog(L_DEBUG, "Unloaded floodserv");
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
  }
}

static struct MessageQueue *
mqueue_new(struct MessageQueue **hash, const char *name, unsigned int type)
{
  struct MessageQueue *queue;
  int i;
  queue = MyMalloc(sizeof(struct MessageQueue));
  DupString(queue->name, name);

  if(type == MQUEUE_CHAN)
  {
    queue->last = FS_MSG_COUNT - 1;
    queue->max  = FS_MSG_COUNT;
    queue->msg_enforce_time = FS_MSG_TIME;
    queue->lne_enforce_time = FS_LNE_TIME;
  }
  else
  {
    queue->last = FS_GMSG_COUNT - 1;
    queue->max  = FS_GMSG_COUNT;
    queue->msg_enforce_time = FS_GMSG_TIME;
    queue->lne_enforce_time = FS_GMSG_TIME * 2;
  }

  for(i = 0; i < queue->max; ++i)
    queue->entries[i] = NULL;

  hash_add_mqueue(hash, queue);
  return queue;
}

static void
mqueue_add_message(struct MessageQueue *queue, const char *message)
{
  struct FloodMsg *entry;
  int i;

  if(queue->last == 0 && (entry = queue->entries[queue->last]))
  {
    for(i = queue->max - 1; i > 0; --i)
      queue->entries[i] = queue->entries[i - 1];
    floodmsg_free(entry);
  }

  entry = floodmsg_new(message);
  queue->entries[queue->last] = entry;

  if(queue->last > 0)
    --queue->last;
}

static int
mqueue_enforce(struct MessageQueue *queue)
{
  struct FloodMsg *lentry, *fentry, *tmp;
  time_t age = 0;
  int i = 0;
  int enforce = 1;

  lentry = queue->entries[queue->max-1];
  fentry = queue->entries[0];

  if(queue->last != 0 || lentry == NULL || fentry == NULL)
    return MQUEUE_NONE;

  age = fentry->time - lentry->time;

  if(age <= queue->lne_enforce_time)
    return MQUEUE_LINE;

  if(age <= queue->msg_enforce_time)
  {
    for(i = 0; i < queue->max; ++i)
    {
      tmp = queue->entries[i];
      if(ircncmp(fentry->message, tmp->message, IRC_BUFSIZE) != 0)
      {
        enforce = 0;
        break;
      }
    }
  }

  if(enforce)
    return MQUEUE_MESG;
  else
    return MQUEUE_NONE;
}

static void
mqueue_hash_free(struct Channel *chptr)
{
  dlink_node *ptr = NULL, *next_ptr = NULL;
  if(chptr->regchan != NULL && chptr->regchan->flood_hash != NULL)
  {
    DLINK_FOREACH_SAFE(ptr, next_ptr, chptr->regchan->flood_list.head)
    {
      hash_del_mqueue(chptr->regchan->flood_hash, ptr->data);
      mqueue_free(ptr->data);
      dlinkDelete(ptr, &chptr->regchan->flood_list);
    }
    MyFree(chptr->regchan->flood_hash);
    chptr->regchan->flood_hash = NULL;
  }
}

static void
mqueue_free(struct MessageQueue *queue)
{
  int i;
  if(queue != NULL)
  {
    MyFree(queue->name);

    for(i = 0; i < queue->max; ++i)
      floodmsg_free(queue->entries[i]);

    MyFree(queue);
  }
}

static void
floodmsg_free(struct FloodMsg *entry)
{
  if(entry != NULL)
  {
    if(!EmptyString(entry->message))
      MyFree(entry->message);
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

  if(ircncmp(client->name, fsclient->name, NICKLEN) != 0)
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

  if(ircncmp(client->name, fsclient->name, NICKLEN) != 0)
  {
    mqueue_hash_free(channel);
    ilog(L_DEBUG, "FloodServ removed from %s by %s", channel->chname,
      source->name);
  }

  if(channel->members.length == 1)
    part_channel(client, channel->chname, "");

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
    mqueue_hash_free(chan);

  return pass_callback(fs_channel_destroy_hook, chan);
}

static void *
fs_on_privmsg(va_list args)
{
  struct Client *source = va_arg(args, struct Client *);
  struct Channel *channel = va_arg(args, struct Channel *);
  char *message = va_arg(args, char *);
  struct MessageQueue *queue;
  int enforce;

  if(channel->regchan != NULL && channel->regchan->flood_hash != NULL)
  {
    queue = hash_find_mqueue_host(channel->regchan->flood_hash, source->host);

    if(queue == NULL)
    {
      queue = mqueue_new(channel->regchan->flood_hash, source->host,
        MQUEUE_CHAN);
      dlinkAdd(queue, &queue->node, &channel->regchan->flood_list);
    }

    mqueue_add_message(queue, message);
    enforce = mqueue_enforce(queue);
    switch(enforce)
    {
      case MQUEUE_LINE:
        ilog(L_NOTICE, "%s@%s TRIGGERED LINE FLOOD in %s", source->name,
          source->host, channel->chname);
        break;
      case MQUEUE_MESG:
        ilog(L_NOTICE, "%s@%s TRIGGERED MESSAGE FLOOD in %s", source->name,
          source->host, channel->chname);
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
