#include "stdinc.h"

static void ms_ping(struct Client *, struct Client *, int, char *[]);

struct Message ping_msgtab = {
  "PING", 0, 0, 1, 0, MFLG_SLOW, 0,
  { ms_ping, m_ignore, ms_ping }
};

static dlink_node *connected_hook;
static void *irc_server_connected(va_list);

INIT_MODULE(irc, "$Revision: 470 $")
{
  connected_hook = install_hook(connected_cb, irc_server_connected);
  mod_add_cmd(&ping_msgtab);
}

CLEANUP_MODULE
{
  uninstall_hook(connected_cb, irc_server_connected);
  mod_del_cmd(&ping_msgtab);
}

static void *
irc_server_connected(va_list args)
{
  struct Client *client = va_arg(args, struct Client *);
  
  sendto_server(client, "PASS %s TS 5", client->server->pass);
  sendto_server(client, "CAPAB :KLN PARA EOB QS UNKLN GLN ENCAP TBURST CHW IE EX");
  sendto_server(client, "SERVER %s 1 :%s", me.name, me.info);
  send_queued_write(client);
}

static void 
ms_ping(struct Client *source, struct Client *client, int parc, char *parv[])
{
  sendto_server(source, ":%s PONG %s :%s", me.name, me.name, source->name);
}

