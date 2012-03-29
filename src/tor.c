#include "stdinc.h"
#include "hash.h"
#include "tor.h"

static dlink_list tornode_list;

static BlockHeap *tornode_heap = NULL;

void
init_tor()
{
  tornode_heap = BlockHeapCreate("tornode", sizeof(struct TorNode),
    TORNODE_HEAP_SIZE);
}

void
cleanup_tor()
{
  tornode_clear();
  BlockHeapDestroy(tornode_heap);
}

void
tornode_add(const char *host)
{
  if (find_tor(host) == NULL)
  {
    struct TorNode *node = BlockHeapAlloc(tornode_heap);
    strlcpy(node->host, host, sizeof(node->host));
    hash_add_tor(node);
    dlinkAdd(node, &node->node, &tornode_list);
  }
}

static void
tornode_free(struct TorNode *node)
{
  if (node != NULL)
  {
    hash_del_tor(node);
    BlockHeapFree(tornode_heap, node);
  }
}

void
tornode_clear()
{
  struct TorNode *node;
  dlink_node *this = NULL, *next = NULL;

  DLINK_FOREACH_SAFE(this, next, tornode_list.head)
  {
    node = this->data;
    tornode_free(node);
    dlinkDelete(&node->node, &tornode_list);
  }
}
