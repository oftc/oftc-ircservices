#include "stdinc.h"

static void ms_ping(client_t *, client_t *, int, char *[]);

message_t ping_msgtab = {
  "PING", 0, 0, 1, 0, MFLG_SLOW, 0,
  { ms_ping, m_ignore, ms_ping }
};


static dlink_node *connected_hook;
static void *irc_server_connected(va_list);

#ifndef STATIC_MODULES
void
_modinit(void)
{
  connected_hook = install_hook(connected_cb, irc_server_connected);
  mod_add_cmd(&ping_msgtab);
}

void
_moddeinit(void)
{
  uninstall_hook(connected_cb, irc_server_connected);
  mod_del_cmd(&ping_msgtab);
}

static void *
irc_server_connected(va_list args)
{
  client_t *client = va_arg(args, client_t *);
  
  sendto_server(client, "PASS %s TS 6 %s", client->server->pass, me.id);
  sendto_server(client, "CAPAB :TS6");
  sendto_server(client, "SERVER %s 1 :%s", me.name, me.info);
  send_queued_write(client);
}

static void ms_ping(client_t *source, client_t *client, int parc, char *parv[])
{
  sendto_server(source, ":%s PONG %s :%s", me.id, me.name, source->name);
}

const char *_version = "$Revision: 2 $";
#endif

