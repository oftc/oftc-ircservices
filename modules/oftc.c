#include "stdinc.h"

struct Message gnotice_msgtab = {
  "GNOTICE", 0, 0, 3, 0, MFLG_SLOW, 0,
  { m_ignore, m_ignore }
};

INIT_MODULE(oftc, "$Revision: 470 $")
{
  mod_add_cmd(&gnotice_msgtab);
}

CLEANUP_MODULE
{
  mod_del_cmd(&gnotice_msgtab);
}

static void 
irc_sendmsg_svscloak(struct Client *client, struct Client *target, 
    char *cloakstring) 
{
  sendto_server(client, ":%s SVSCLOAK %s :%s", 
    me.name, target->name, cloakstring);
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
