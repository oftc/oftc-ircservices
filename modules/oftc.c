#include "stdinc.h"

static void *irc_sendmsg_gnotice(va_list);
static void *irc_sendmsg_svsmode(va_list args);
static dlink_node *gnotice_hook;
static dlink_node *umode_hook;

struct Message gnotice_msgtab = {
  "GNOTICE", 0, 0, 3, 0, MFLG_SLOW, 0,
  { m_ignore, m_ignore }
};

INIT_MODULE(oftc, "$Revision: 470 $")
{
  gnotice_hook = install_hook(gnotice_cb, irc_sendmsg_gnotice);
  umode_hook = install_hook(umode_cb, irc_sendmsg_svsmode);
  mod_add_cmd(&gnotice_msgtab);
}

CLEANUP_MODULE
{
  mod_del_cmd(&gnotice_msgtab);
}

static void *
irc_sendmsg_gnotice(va_list args)
{
  struct Client *client = va_arg(args, struct Client*);
  char          *source = va_arg(args, char *);
  char          *text   = va_arg(args, char *);
  
  // 1 is UMODE_ALL, aka UMODE_SERVERNOTICE
  sendto_server(client, ":%s GNOTICE %s 1 :%s", source, source, text);
  return NULL;
}

static void 
irc_sendmsg_svscloak(struct Client *client, struct Client *target, 
    char *cloakstring) 
{
  sendto_server(client, ":%s SVSCLOAK %s :%s", 
    me.name, target->name, cloakstring);
}

static void *
irc_sendmsg_svsmode(va_list args)
{
  struct Client *client = va_arg(args, struct Client*);
  char          *target = va_arg(args, char *);
  char          *mode   = va_arg(args, char *);

  sendto_server(client, ":%s SVSMODE %s :%s", me.name, target, mode);

  return NULL;
}

static void
irc_sendmsg_svsnick(struct Client *client, struct Client *target, char *newnick)
{
  sendto_server(client, ":%s SVSNICK %s :%s", me.name, target->name, newnick);
}
