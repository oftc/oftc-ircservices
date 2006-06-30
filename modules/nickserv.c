#include "stdinc.h"

static struct Service *nickserv = NULL;

INIT_MODULE(nickserv, "$Revision: 470 $")
{
  nickserv = make_service("NickServ");
  clear_tree_parse(&nickserv->msg_tree);
  dlinkAdd(nickserv, &nickserv->node, &services_list);
  hash_add_service(nickserv);
  introduce_service(nickserv);
}

CLEANUP_MODULE
{
}
