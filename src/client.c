#include "stdinc.h"

dlink_list global_client_list;
dlink_list global_server_list;

struct Client *
make_client()
{
  struct Client *client = MyMalloc(sizeof(struct Client));
  client->from = client;
  client->node = make_dlink_node();

  return client;
}

struct Server *
make_server()
{
  struct Server *server = MyMalloc(sizeof(struct Server));
 
  return server;
}

/* find_person()
 *
 * inputs - pointer to name
 * output - return client pointer
 * side effects - find person by (nick)name
 */
struct Client *
find_person(const struct Client *source, const char *name)
{
  struct Client *target = NULL;

  if(IsDigit(*name) && IsServer(source->from))
    target = hash_find_id(name);
  else
    target = find_client(name);

  return(target && IsClient(target)) ? target : NULL;
}

/*
 * dead_link_on_write - report a write error if not already dead,
 *      mark it as dead then exit it
 */
void
dead_link_on_write(struct Client *client, int ierrno)
{
  if (IsDefunct(client->server))
    return;

  dbuf_clear(&client->server->buf_recvq);
  dbuf_clear(&client->server->buf_sendq);
}

