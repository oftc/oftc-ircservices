#include "stdinc.h"

dlink_list services_list = { 0 };
struct Callback *newuser_cb;
static BlockHeap *services_heap  = NULL;

void
init_interface()
{
  services_heap = BlockHeapCreate("services", sizeof(struct Service), SERVICES_HEAP_SIZE);
  newuser_cb = register_callback("introduce user", NULL);
}

struct Service *
make_service(char *name)
{
  struct Service *service = BlockHeapAlloc(services_heap);  

  strlcpy(service->name, name, sizeof(service->name));

  return service;
}

void
introduce_service(struct Service *service)
{
  struct Client *client = make_client(&me);

  client->tsinfo = CurrentTime;
  dlinkAdd(client, &client->node, &global_client_list);

  /* copy the nick in place */
  strlcpy(client->name, service->name, sizeof(client->name));
  hash_add_client(client);

  register_remote_user(&me, client, "services", me.name, me.name, service->name);

  /* If we are not connected yet, the service will be sent as part of burst */
  if(me.uplink != NULL)
  {
    execute_callback(newuser_cb, me.uplink, service->name, "services", me.name,
      service->name, "o");
  }
}
