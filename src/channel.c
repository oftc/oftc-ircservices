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

  chptr = BlockHeapAlloc(channel_heap);

  /* doesn't hurt to set it here */
  chptr->channelts = CurrentTime;

  strlcpy(chptr->chname, chname, sizeof(chptr->chname));
  dlinkAdd(chptr, &chptr->node, &global_channel_list);

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
  struct Client *client_p = member->client_p;
  struct Channel *chptr = member->chptr;

  dlinkDelete(&member->channode, &chptr->members);
  dlinkDelete(&member->usernode, &client_p->channel);

  BlockHeapFree(member_heap, member);
  printf("Removing %s from channel %s\n", client_p->name, chptr->chname);

  if (chptr->members.head == NULL)
  {
    assert(dlink_list_length(&chptr->members) == 0);  
    printf("Destroying empty channel %s\n", chptr->chname);
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
    remove_ban(ptr->data, list);

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
find_channel_link(struct Client *client, struct Channel *chptr)
{
  dlink_node *ptr = NULL;

  if (!IsClient(client))
    return NULL;

  DLINK_FOREACH(ptr, client->channel.head)
    if (((struct Membership *)ptr->data)->chptr == chptr)
      return (struct Membership *)ptr->data;

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
add_user_to_channel(struct Channel *chptr, struct Client *who,
                    unsigned int flags, int flood_ctrl)
{
  struct Membership *ms = NULL;

  ms = BlockHeapAlloc(member_heap);
  ms->client_p = who;
  ms->chptr = chptr;
  ms->flags = flags;

  printf("Adding %s to %s\n", who->name, chptr->chname);

  dlinkAdd(ms, &ms->channode, &chptr->members);
  dlinkAdd(ms, &ms->usernode, &who->channel);
}

