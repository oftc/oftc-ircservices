#include "stdinc.h"


INIT_MODULE(oftc, "$Revision: 470 $")
{
}

CLEANUP_MODULE
{
}

static void 
irc_sendmsg_svscloak(struct Client *client, struct Client *target, char *cloakstring) 
{
  sendto_server(client, ":%s SVSCLOAK %s :%s", 
    me.name, target->nick, cloakstring);
}

static void
irc_sendmsg_svsmode(struct Client *client, char *target, char *modes)
{
  sendto_server(client, ":%s SVSMODE %s :%s", me.name, target, modes);
}

static void
irc_sendmsg_svsnick(struct Client *client, struct Client *target, char *newnick)
{
  sendto_server(client, ":%s SVSNICK %s :%s", me.name, target->name, newnick);
}
