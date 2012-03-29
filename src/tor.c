#include "stdinc.h"
#include "conf/conf.h"
#include "conf/servicesinfo.h"
#include "hash.h"
#include "tor.h"

static dlink_list tornode_list;

static BlockHeap *tornode_heap = NULL;

static dlink_node *config_loaded_hook;

static void tornode_add(const char*);
static void tornode_clear();

static void*
config_loaded(va_list args)
{
  int cold = va_arg(args, int);
  FBFILE *t = NULL;
  char buffer[256];

  if(!EmptyString(ServicesInfo.tor_list_fname)) {
    ilog(L_DEBUG, "Opening tor list: %s", ServicesInfo.tor_list_fname);
    t = fbopen(ServicesInfo.tor_list_fname, "r");
    if (t != NULL) {
      tornode_clear();
      while(fbgets(buffer, sizeof(buffer), t) != NULL)
      {
        tornode_add(buffer);
      }
      fbclose(t);
    }
  }

  return pass_callback(config_loaded_hook, cold);
}

void
init_tor()
{
  tornode_heap = BlockHeapCreate("tornode", sizeof(struct TorNode),
    TORNODE_HEAP_SIZE);

  config_loaded_hook = install_hook(on_config_loaded_cb, config_loaded);
}

void
cleanup_tor()
{
  tornode_clear();
  BlockHeapDestroy(tornode_heap);

  uninstall_hook(on_config_loaded_cb, config_loaded);
}

void
tornode_add(const char *host)
{
  if (find_tor(host) == NULL)
  {
    struct TorNode *node = BlockHeapAlloc(tornode_heap);
    strlcpy(node->host, host, sizeof(node->host));

    if (node->host[strlen(node->host)-1] == '\n')
      node->host[strlen(node->host)-1] = '\0';

    hash_add_tor(node);
    dlinkAdd(node, &node->node, &tornode_list);

    ilog(L_DEBUG, "Added %s to tor exit node list", node->host);
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
    dlinkDelete(this, &tornode_list);
    tornode_free(node);
  }
}
