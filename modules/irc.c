#include "stdinc.h"

static dlink_node *connected_hook;
static void *irc_server_connected(va_list);

#ifndef STATIC_MODULES
void
_modinit(void)
{
  connected_hook = install_hook(connected_cb, irc_server_connected);
}

void
_moddeinit(void)
{
  uninstall_hook(connected_cb, irc_server_connected);
}

static void *
irc_server_connected(va_list args)
{
  client_t *client = va_arg(args, client_t *);
  
  sendto_server(client, "PASS MOO TS 6 %s", me.id);
  sendto_server(client, "SERVER %s 1 :%s", me.name, me.info);
}
  

const char *_version = "$Revision: 2 $";
#endif

