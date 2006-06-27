#include "stdinc.h"

static BlockHeap *channel_heap = NULL;
dlink_list global_channel_list = { NULL, NULL, 0 };

void
channel_init(void)
{
  channel_heap = BlockHeapCreate("channel", sizeof(struct Channel), CHANNEL_HEAP_SIZE);
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

