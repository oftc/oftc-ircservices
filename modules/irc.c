#include "stdinc.h"

static void ms_ping(struct Client *, struct Client *, int, char *[]);
static void ms_nick(struct Client *, struct Client *, int, char*[]);
static void ms_server(struct Client *, struct Client *, int, char*[]);
static void ms_sjoin(struct Client *, struct Client *, int, char*[]);

static void do_user_modes(struct Client *client, const char *modes);

struct Message ping_msgtab = {
  "PING", 0, 0, 1, 0, MFLG_SLOW, 0,
  { ms_ping, m_ignore, ms_ping }
};

struct Message eob_msgtab = {
  "EOB", 0, 0, 0, 0, MFLG_SLOW, 0,
  { m_ignore, m_ignore, m_ignore }
};

struct Message server_msgtab = {
  "SERVER", 0, 0, 3, 0, MFLG_SLOW, 0,
  { ms_server, m_ignore, ms_server }
};

struct Message nick_msgtab = {
  "NICK", 0, 0, 1, 0, MFLG_SLOW, 0,
  { ms_nick, m_ignore, ms_nick }
};

struct Message sjoin_msgtab = {
  "SJOIN", 0, 0, 4, 0, MFLG_SLOW, 0,
  { ms_sjoin, m_ignore, ms_sjoin }
};

static dlink_node *connected_hook;
static void *irc_server_connected(va_list);

INIT_MODULE(irc, "$Revision: 470 $")
{
  connected_hook = install_hook(connected_cb, irc_server_connected);
  mod_add_cmd(&ping_msgtab);
  mod_add_cmd(&server_msgtab);
  mod_add_cmd(&nick_msgtab);
  mod_add_cmd(&sjoin_msgtab);
  mod_add_cmd(&eob_msgtab);
}

CLEANUP_MODULE
{
  uninstall_hook(connected_cb, irc_server_connected);
  mod_del_cmd(&ping_msgtab);
  mod_del_cmd(&server_msgtab);
  mod_del_cmd(&nick_msgtab);
  mod_del_cmd(&sjoin_msgtab);
  mod_del_cmd(&eob_msgtab);
}

/** Introduce a new server; currently only useful for connect and jupes
 * @param
 * prefix prefix, usually me.name
 * name server to introduce
 * info Server Information string
 */
static void 
irc_sendmsg_server(struct Client *client, char *prefix, char *name, char *info) {
  if (prefix == NULL) 
  {
    sendto_server(client, "SERVER %s 1 :%s", name, info);
  } 
  else 
  {
    sendto_server(client, ":%s SERVER %s 2 :%s", prefix, name, info);
  }
}

/** Introduce a new user
 * @param
 * nick Nickname of user
 * user username ("identd") of user
 * host hostname of that user
 * info Realname Information
 * umode usermode to add (i.e. "ao")
 */
static void
irc_sendmsg_nick(struct Client *client, char *nick, char *user, char *host,
  char *info, char *umode)
{
  sendto_server(client, "NICK %s 1 0 +%s %s %s %s :%s", nick, umode, user, host, me.name, info);
}

static void *
irc_server_connected(va_list args)
{
  struct Client *client = va_arg(args, struct Client *);
  
  sendto_server(client, "PASS %s TS 5", client->server->pass);
  sendto_server(client, "CAPAB :KLN PARA EOB QS UNKLN GLN ENCAP TBURST CHW IE EX");
  irc_sendmsg_server(client, NULL, me.name, me.info);
  send_queued_write(client);

  return NULL;
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
    struct Client *newclient = make_client(source);

    strlcpy(newclient->name, parv[1], sizeof(newclient->name));
    strlcpy(newclient->info, parv[3], sizeof(newclient->info));
    newclient->hopcount = atoi(parv[2]);
    SetServer(newclient);
    dlinkAdd(newclient, &newclient->node, &global_client_list);
    hash_add_client(newclient);
    printf("Got server %s from hub %s\n", parv[1], client->name);
  }
}

//SJOIN 1151079915 #test +nt :cryogen
static void
ms_sjoin(struct Client *source, struct Client *client, int parc, char *parv[])
{
  struct Channel *chan = hash_find_channel(parv[2]);

  if(chan == NULL)
  {
    /* Don't know about this channel, it's new.. */
    chan = make_channel(parv[2]);
    chan->channelts = atoi(parv[1]);

    printf("Adding channel %s\n", parv[2]);
  }
  else
  {
    /* This channel exists, but we need to add more people to it */
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
    newclient = make_client(client);
    strlcpy(newclient->name, parv[1], sizeof(newclient->name));
    strlcpy(newclient->username, parv[5], sizeof(newclient->username));
    strlcpy(newclient->host, parv[6], sizeof(newclient->host));
    strlcpy(newclient->info, parv[8], sizeof(newclient->info));
    newclient->hopcount = atoi(parv[2]);
    newclient->tsinfo = atoi(parv[3]);
    do_user_modes(newclient, parv[4]);

    SetClient(newclient);
    dlinkAdd(newclient, &newclient->node, &global_client_list);
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

static void
do_user_modes(struct Client *client, const char *modes)
{
  char *ch = (char*)modes;
  int dir = MODE_ADD;

  while(*ch)
  {
    switch(*ch)
    {
      case '+':
        dir = MODE_ADD;
        break;
      case '-':
        dir = MODE_DEL;
        break;
      case 'o':
        if(dir == MODE_ADD)
        {
          SetOper(client);
          printf("Setting %s as operator(o)\n", client->name);
        }
        else
          ClearOper(client);
        break;
    }
    ch++;
  }
}


