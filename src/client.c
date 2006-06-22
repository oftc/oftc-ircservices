#include "stdinc.h"

static dlink_list global_client_list;
static dlink_list global_server_list;

client_t *
make_client()
{
  client_t *client = MyMalloc(sizeof(client_t));

  return client;
}

server_t *
make_server()
{
  server_t *server = MyMalloc(sizeof(server_t));
 
  return server;
}
