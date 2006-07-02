#include "stdinc.h"

dlink_list services_list = { 0 };
struct Callback *newuser_cb;
struct Callback *privmsg_cb;
struct Callback *notice_cb;
struct Callback *gnotice_cb;
static BlockHeap *services_heap  = NULL;

void
init_interface()
{
  services_heap = BlockHeapCreate("services", sizeof(struct Service), SERVICES_HEAP_SIZE);
  newuser_cb = register_callback("introduce user", NULL);
  privmsg_cb = register_callback("message user", NULL);
  notice_cb  = register_callback("NOTICE user", NULL);
  gnotice_cb = register_callback("Global Notice", NULL);
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

void
tell_user(struct Service *service, struct Client *client, char *text)
{
  execute_callback(privmsg_cb, me.uplink, service->name, client->name, text);
}

void
reply_user(struct Service *service, struct Client *client, char *text)
{
  execute_callback(notice_cb, me.uplink, service->name, client->name, text);
}

void
global_notice(struct Service *service, char *text, ...)
{
  va_list arg;
  char buf[4096]; //extra buffer, check?
  
  va_start(arg, text);
  vsnprintf(buf, 4096, text, arg);
  va_end(arg);
  execute_callback(gnotice_cb, me.uplink, service->name, buf);
}
