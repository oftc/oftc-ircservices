#include "stdinc.h"

static void ms_ping(struct Client *, struct Client *, int, char *[]);
static void ms_server(struct Client *, struct Client *, int, char*[]);
static void ms_nick(struct Client *, struct Client *, int, char*[]);

struct Message ping_msgtab = {
  "PING", 0, 0, 1, 0, MFLG_SLOW, 0,
  { ms_ping, m_ignore, ms_ping }
};

struct Message server_msgtab = {
  "SERVER", 0, 0, 3, 0, MFLG_SLOW, 0,
  { ms_server, m_ignore, ms_server }
};

struct Message nick_msgtab = {
  "NICK", 0, 0, 1, 0, MFLG_SLOW, 0,
  { ms_nick, m_ignore, ms_nick }
};

static dlink_node *connected_hook;
static void *irc_server_connected(va_list);

INIT_MODULE(irc, "$Revision: 470 $")
{
  connected_hook = install_hook(connected_cb, irc_server_connected);
  mod_add_cmd(&ping_msgtab);
  mod_add_cmd(&server_msgtab);
  mod_add_cmd(&nick_msgtab);
}

CLEANUP_MODULE
{
  uninstall_hook(connected_cb, irc_server_connected);
  mod_del_cmd(&ping_msgtab);
  mod_del_cmd(&server_msgtab);
  mod_del_cmd(&nick_msgtab);
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

static void
ms_server(struct Client *source, struct Client *client, int parc, char *parv[])
{
  if(IsConnecting(client))
  {
    sendto_server(client, "SVINFO 5 5 0: %lu", CurrentTime);
    sendto_server(client, ":%s PING :%s", me.name, me.name);
    SetServer(client);
    hash_add_client(client);
    printf("Completed server connection to %s\n", source->name);
    ClearConnecting(client);
  }
  else
  {
    struct Client *newclient = make_client();

    strlcpy(newclient->name, parv[1], sizeof(newclient->name));
    strlcpy(newclient->info, parv[3], sizeof(newclient->info));
    newclient->node = make_dlink_node();
    newclient->from = client;
    newclient->hopcount = atoi(parv[2]);
    SetServer(newclient);
    dlinkAdd(newclient, newclient->node, &global_client_list);
    hash_add_client(newclient);
    printf("Got server %s from hub %s\n", parv[1], client->name);
  }
}

static void
ms_nick(struct Client *source, struct Client *client, int parc, char *parv[])
{
  struct Client *newclient;

  /* NICK from server */
  if(parc == 9)
  {
    if((newclient = find_client(parv[1])) != NULL)
    {
      printf("Already got this nick! %s\n", parv[1]);
      return;
    }
    newclient = make_client();
    newclient->from = client;
    strlcpy(newclient->name, parv[1], sizeof(newclient->name));
    strlcpy(newclient->username, parv[5], sizeof(newclient->username));
    strlcpy(newclient->host, parv[6], sizeof(newclient->host));
    strlcpy(newclient->info, parv[8], sizeof(newclient->info));
    newclient->hopcount = atoi(parv[2]);
    newclient->tsinfo = atoi(parv[3]);

    newclient->node = make_dlink_node();
    SetClient(newclient);
    dlinkAdd(newclient, newclient->node, &global_client_list);
    hash_add_client(newclient);
  }
  /* Client changing nick with TS */
  else if(parc == 3)
  {
    printf("Nick change: %s!%s@%s -> %s\n", client->name, client->username,
        client->host, parv[1]);
    hash_del_client(client);
    strlcpy(client->name, parv[1], sizeof(client->name));
    client->tsinfo = atoi(parv[2]);
    hash_add_client(client);
  }
}
