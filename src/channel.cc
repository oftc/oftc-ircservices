/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  channel.c: Channel functions
 *
 *  Copyright (C) 2006 Stuart Walsh and the OFTC Coding department
 *
 *  Some parts:
 *
 *  Copyright (C) 2002 by the past and present ircd coders, and others.
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

static BlockHeap *channel_heap = NULL;
static BlockHeap *member_heap = NULL;
static BlockHeap *topic_heap = NULL;


BlockHeap *ban_heap            = NULL;
dlink_list global_channel_list = { NULL, NULL, 0 };

void
init_channel(void)
{
  channel_heap = BlockHeapCreate("channel", sizeof(struct Channel), CHANNEL_HEAP_SIZE);
  ban_heap = BlockHeapCreate("ban", sizeof(struct Ban), BAN_HEAP_SIZE);
  member_heap = BlockHeapCreate("member", sizeof(struct Membership), CHANNEL_HEAP_SIZE*2);
  topic_heap = BlockHeapCreate("topic", TOPICLEN+1 + USERHOST_REPLYLEN, TOPIC_HEAP_SIZE);
}

struct Channel *
make_channel(const char *chname)
{
  struct Channel *chptr = NULL;

  assert(!EmptyString(chname));

  chptr = (struct Channel *)BlockHeapAlloc(channel_heap);

  /* doesn't hurt to set it here */
  chptr->channelts = CurrentTime;

  strlcpy(chptr->chname, chname, sizeof(chptr->chname));
  dlinkAdd(chptr, &chptr->node, &global_channel_list);

  chptr->regchan = db_find_chan(chname);

  hash_add_channel(chptr);

  return chptr;
}

void
remove_ban(struct Ban *bptr, dlink_list *list)
{
  dlinkDelete(&bptr->node, list);

  MyFree(bptr->name);
  MyFree(bptr->username);
  MyFree(bptr->host);
  MyFree(bptr->who);

  BlockHeapFree(ban_heap, bptr);
}

/*! \brief deletes an user from a channel by removing a link in the
 *         channels member chain.
 * \param member pointer to Membership struct
 */
void
remove_user_from_channel(struct Membership *member)
{
  Client *client_p = member->client_p;
  struct Channel *chptr = member->chptr;

  dlinkDelete(&member->channode, &chptr->members);
//  dlinkDelete(&member->usernode, &client_p->channel);

  BlockHeapFree(member_heap, member);
  ilog(L_DEBUG, "Removing %s from channel %s", client_p->name().c_str(), chptr->chname);

  if (chptr->members.head == NULL)
  {
    assert(dlink_list_length(&chptr->members) == 0);  
    ilog(L_DEBUG, "Destroying empty channel %s", chptr->chname);
    execute_callback(on_channel_destroy_cb, chptr);
    destroy_channel(chptr);
  }
}

/* free_channel_list()
 *
 * inputs       - pointer to dlink_list
 * output       - NONE
 * side effects -
 */
void
free_channel_list(dlink_list *list)
{
  dlink_node *ptr = NULL, *next_ptr = NULL;

  DLINK_FOREACH_SAFE(ptr, next_ptr, list->head)
    remove_ban((struct Ban*)ptr->data, list);

  assert(list->tail == NULL && list->head == NULL);
}

void
free_topic(struct Channel *chptr)
{
  void *ptr = NULL;
  assert(chptr);
  if (chptr->topic == NULL)
    return;

  /*
   * If you change allocate_topic you MUST change this as well
   */
  ptr = chptr->topic;
  BlockHeapFree(topic_heap, ptr);
  chptr->topic      = NULL;
  chptr->topic_info = NULL;
}


/*! \brief walk through this channel, and destroy it.
 * \param chptr channel pointer
 */
void
destroy_channel(struct Channel *chptr)
{
  /* free ban/exception/invex lists */
  free_channel_list(&chptr->banlist);
  free_channel_list(&chptr->exceptlist);
  free_channel_list(&chptr->invexlist);

  free_topic(chptr);

  dlinkDelete(&chptr->node, &global_channel_list);
  hash_del_channel(chptr);

  if(chptr->regchan != NULL)
  {
    free_regchan(chptr->regchan);
    chptr->regchan = NULL;
  }

  BlockHeapFree(channel_heap, chptr);
}

int
has_member_flags(struct Membership *ms, unsigned int flags)
{
  if (ms != NULL)
    return ms->flags & flags;
  return 0;
}


struct Membership *
find_channel_link(Client *client, struct Channel *chptr)
{
  //dlink_node *ptr = NULL;

/*  DLINK_FOREACH(ptr, client->channel.head)
    if (((struct Membership *)ptr->data)->chptr == chptr)
      return (struct Membership *)ptr->data;
*/
  return NULL;
}

/*! \brief adds a user to a channel by adding another link to the
 *         channels member chain.
 * \param chptr      pointer to channel to add client to
 * \param who        pointer to client (who) to add
 * \param flags      flags for chanops etc
 * \param flood_ctrl whether to count this join in flood calculations
 */
void
add_user_to_channel(struct Channel *chptr, Client *who,
                    unsigned int flags, int flood_ctrl)
{
  struct Membership *ms = NULL;

  ms = (struct Membership *)BlockHeapAlloc(member_heap);
  ms->client_p = who;
  ms->chptr = chptr;
  ms->flags = flags;

  ilog(L_DEBUG, "Adding %s to %s", who->name().c_str(), chptr->chname);

  dlinkAdd(ms, &ms->channode, &chptr->members);
  //dlinkAdd(ms, &ms->usernode, &who->channel);
}

struct Ban *
find_bmask(Client *who, const dlink_list *const list)
{
  const dlink_node *ptr = NULL;

  DLINK_FOREACH(ptr, list->head)
  {
    struct Ban *bp = (struct Ban *)ptr->data;

  }
  return NULL;
}

/*! \brief Allocates a new topic
 * \param chptr Channel to allocate a new topic for
 */
static void
allocate_topic(struct Channel *chptr)
{
  void *ptr = NULL;

  ptr = BlockHeapAlloc(topic_heap);

  /*
   * Basically we allocate one large block for the topic and
   * the topic info.  We then split it up into two and shove it
   * in the chptr
   */
  chptr->topic       = (char*)ptr;
  chptr->topic_info  = ((char *)ptr) + TOPICLEN + 1;
  *chptr->topic      = '\0';
  *chptr->topic_info = '\0';
}

/*! \brief Sets the channel topic for chptr
 * \param chptr      Pointer to struct Channel
 * \param topic      The topic string
 * \param topic_info n!u\@h formatted string of the topic setter
 * \param topicts    timestamp on the topic
 */
void
set_channel_topic(struct Channel *chptr, const char *topic,
                  const char *topic_info, time_t topicts)
{
  if (!EmptyString(topic))
  {
    if (chptr->topic == NULL)
      allocate_topic(chptr);

    strlcpy(chptr->topic, topic, TOPICLEN+1);
    strlcpy(chptr->topic_info, topic_info, USERHOST_REPLYLEN);
  }
  else
  {
    if (chptr->topic != NULL)
      free_topic(chptr);
  }

  execute_callback(on_topic_change_cb, chptr, topic_info); 
}
