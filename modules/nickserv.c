/*
 *  oftc-ircservices: an exstensible and flexible IRC Services package
 *  nickserv.c: A C implementation of Nickname Services 
 *
 *  Copyright (C) 2006 Stuart Walsh and the OFTC Coding department
 *
 *  Some parts:
 *
 *  Copyright (C) 2002 by the past and present ircd coders, and others.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 *
 *  $Id$
 */

#include "stdinc.h"

static struct Service *nickserv = NULL;

static dlink_node *ns_umode_hook;
static dlink_node *ns_nick_hook;
static dlink_node *ns_newuser_hook;
static dlink_node *ns_quit_hook;

static dlink_list nick_enforce_list = { NULL, NULL, 0 };

static int guest_number;

static void process_enforce_list(void *);

static void *ns_on_umode_change(va_list);
static void *ns_on_newuser(va_list);
static void *ns_on_nick_change(va_list);
static void *ns_on_quit(va_list);
static void m_drop(struct Service *, struct Client *, int, char *[]);
static void m_help(struct Service *, struct Client *, int, char *[]);
static void m_identify(struct Service *, struct Client *, int, char *[]);
static void m_register(struct Service *, struct Client *, int, char *[]);
static void m_set(struct Service *, struct Client *, int, char *[]);
static void m_access(struct Service *, struct Client *, int, char *[]);
static void m_ghost(struct Service *, struct Client *, int, char *[]);
static void m_link(struct Service *, struct Client *, int, char *[]);
static void m_unlink(struct Service *, struct Client *, int, char *[]);
static void m_info(struct Service *,struct Client *, int, char *[]);
static void m_forbid(struct Service *,struct Client *, int, char *[]);

static void m_set_language(struct Service *, struct Client *, int, char *[]);
static void m_set_password(struct Service *, struct Client *, int, char *[]);
static void m_set_enforce(struct Service *, struct Client *, int, char *[]);
static void m_set_secure(struct Service *, struct Client *, int, char *[]);
static void m_set_url(struct Service *, struct Client *, int, char *[]);
static void m_set_email(struct Service *, struct Client *, int, char *[]);
static void m_set_cloak(struct Service *, struct Client *, int, char *[]);
static void m_set_cloakstring(struct Service *, struct Client *, int, char *[]);
static void m_access_add(struct Service *, struct Client *, int, char *[]);
static void m_access_list(struct Service *, struct Client *, int, char *[]);
static void m_access_del(struct Service *, struct Client *, int, char *[]);

static struct ServiceMessage register_msgtab = {
  NULL, "REGISTER", 0, 2, NS_HELP_REG_SHORT, NS_HELP_REG_LONG,
  { m_register, m_alreadyreg, m_alreadyreg, m_alreadyreg }
};

static struct ServiceMessage identify_msgtab = {
  NULL, "IDENTIFY", 0, 1, NS_HELP_ID_SHORT, NS_HELP_ID_LONG,
  { m_identify, m_identify, m_identify, m_identify }
};

static struct ServiceMessage id_msgtab = {
  NULL, "ID", 0, 1, NS_HELP_ID_SHORT, NS_HELP_ID_LONG,
  { m_identify, m_identify, m_identify, m_identify }
};

static struct ServiceMessage help_msgtab = {
  NULL, "HELP", 0, 0, NS_HELP_SHORT, NS_HELP_LONG,
  { m_help, m_help, m_help, m_help }
};

static struct ServiceMessage drop_msgtab = {
  NULL, "DROP", 0, 0, NS_HELP_DROP_SHORT, NS_HELP_DROP_LONG,
  { m_notid, m_drop, m_drop, m_drop }
};

static struct SubMessage set_sub[9] = {
  { "LANGUAGE"    , 0, 0, -1, -1, 
    { m_notid, m_set_language, m_set_language, m_set_language }
  },
  { "PASSWORD"    , 0, 1, -1, -1, 
    { m_notid, m_set_password, m_set_password, m_set_password }
  },
  { "URL"         , 0, 0, -1, -1, 
    { m_notid, m_set_url, m_set_url, m_set_url }
  },
  { "EMAIL"       , 0, 0, -1, -1, 
    { m_notid, m_set_email, m_set_email, m_set_email }
  },
  { "ENFORCE"     , 0, 0, -1, -1, 
    { m_notid, m_set_enforce, m_set_enforce, m_set_enforce }
  },
  { "SECURE"      , 0, 0, -1, -1, 
    { m_notid, m_set_secure, m_set_secure, m_set_secure }
  },
  { "CLOAK"       , 0, 0, -1, -1, 
    { m_notid, m_set_cloak, m_set_cloak, m_set_cloak }
  },
  { "CLOAKSTRING" , 0, 0, -1, -1, 
    { m_notid, m_set_cloakstring, m_set_cloakstring, m_set_cloakstring }
  },
  { NULL        , 0, 0, 0, 0, { NULL, NULL, NULL, NULL } }
};

static struct ServiceMessage set_msgtab = {
  set_sub, "SET",  0, 0, NS_HELP_SET_SHORT, NS_HELP_SET_LONG,
  { m_notid, m_set, m_set, m_set }
};

static struct SubMessage access_sub[4] = {
  { "ADD", 0, 1, -1, -1, 
    { m_notid, m_access_add, m_access_add, m_access_add }
  },
  { "LIST", 0, 0, -1, -1, 
    { m_notid, m_access_list, m_access_list, m_access_list }
  },
  { "DEL", 0, 0, -1, -1, 
    { m_notid, m_access_del, m_access_del, m_access_del }
  },
  { NULL, 0, 0, 0, 0, { NULL, NULL, NULL, NULL } }
};

static struct ServiceMessage access_msgtab = {
  access_sub, "ACCESS", 0, 0, NS_HELP_ACCESS_SHORT, NS_HELP_ACCESS_LONG,
  { m_notid, m_access, m_access, m_access }
};

static struct ServiceMessage ghost_msgtab = {
  NULL, "GHOST", 0, 2, NS_HELP_GHOST_SHORT, NS_HELP_GHOST_LONG,
  { m_ghost, m_ghost, m_ghost, m_ghost }
};

static struct ServiceMessage link_msgtab = {
  NULL, "LINK", 0, 2, NS_HELP_LINK_SHORT, NS_HELP_LINK_LONG,
  { m_notid, m_link, m_link, m_link }
};

static struct ServiceMessage unlink_msgtab = {
  NULL, "UNLINK", 0, 0, NS_HELP_UNLINK_SHORT, NS_HELP_UNLINK_LONG,
  { m_notid, m_unlink, m_unlink, m_unlink }
};

static struct ServiceMessage info_msgtab = {
  NULL, "INFO", 0, 0, NS_HELP_INFO_SHORT, NS_HELP_INFO_LONG,
  { m_info, m_info, m_info, m_info }
};

static struct ServiceMessage forbid_msgtab = {
  NULL, "FORBID", 0, 1, NS_HELP_FORBID_SHORT, NS_HELP_FORBID_LONG,
  { m_notadmin, m_notadmin, m_notadmin, m_forbid }
};

INIT_MODULE(nickserv, "$Revision$")
{
  nickserv = make_service("NickServ");
  clear_serv_tree_parse(&nickserv->msg_tree);
  dlinkAdd(nickserv, &nickserv->node, &services_list);
  hash_add_service(nickserv);
  introduce_service(nickserv);
  load_language(nickserv, "nickserv.en");
//  load_language(nickserv, "nickserv.rude");
//  load_language(nickserv, "nickserv.de");

  mod_add_servcmd(&nickserv->msg_tree, &drop_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &identify_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &help_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &register_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &set_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &access_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &ghost_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &link_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &unlink_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &info_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &forbid_msgtab);
  mod_add_servcmd(&nickserv->msg_tree, &id_msgtab);
  
  ns_umode_hook       = install_hook(on_umode_change_cb, ns_on_umode_change);
  ns_nick_hook        = install_hook(on_nick_change_cb, ns_on_nick_change);
  ns_newuser_hook     = install_hook(on_newuser_cb, ns_on_newuser);
  ns_quit_hook        = install_hook(on_quit_cb, ns_on_quit);
  
  guest_number = 0;

  eventAdd("process nickserv enforce list", process_enforce_list, NULL, 10);
}

CLEANUP_MODULE
{
  uninstall_hook(on_umode_change_cb, ns_on_umode_change);
  uninstall_hook(on_nick_change_cb, ns_on_nick_change);
  uninstall_hook(on_newuser_cb, ns_on_newuser);
  uninstall_hook(on_quit_cb, ns_on_quit);
  dlinkDelete(&nickserv->node, &services_list);
  eventDelete(process_enforce_list, NULL);
  exit_client(find_client(nickserv->name), &me, "Service unloaded");
  hash_del_service(nickserv);
}

static void
process_enforce_list(void *param)
{
  dlink_node *ptr, *ptr_next;
  
  DLINK_FOREACH_SAFE(ptr, ptr_next, nick_enforce_list.head)
  {
    struct Client *user = (struct Client *)ptr->data;  

    if(CurrentTime > user->enforce_time)
    {
      char newname[NICKLEN+1];
      
      dlinkDelete(ptr, &nick_enforce_list);
      free_dlink_node(ptr);
      user->enforce_time = 0;

      snprintf(newname, NICKLEN, "%s%d", "Guest", guest_number++);
      while((find_client(newname)) != NULL)
      {
        snprintf(newname, NICKLEN, "%s%d", "Guest", guest_number++);
      }
      send_nick_change(nickserv, user, newname);
    }
  }
}

/**
 * Someone wants to register with nickserv
 */
static void 
m_register(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  struct Nick *nick;
  char *pass;
  char password[PASSLEN+SALTLEN+1];
 
  if(strncasecmp(client->name, "guest", 5) == 0)
  {
    reply_user(service, client, NS_NOREG_GUEST, client->name);
    return;
  }
    
  if((nick = db_find_nick(client->name)) != NULL)
  {
    reply_user(service, client, NS_ALREADY_REG, client->name); 
    free_nick(nick);
    return;
  }

  nick = MyMalloc(sizeof(struct Nick));

  make_random_string(nick->salt, SALTLEN+1);
  snprintf(password, sizeof(password), "%s%s", parv[1], nick->salt);

  pass = crypt_pass(password);
  strlcpy(nick->pass, pass, sizeof(nick->pass));
  strlcpy(nick->nick, client->name, sizeof(nick->nick));
  DupString(nick->email, parv[2]);
  MyFree(pass);

  if(db_register_nick(nick))
  {
    client->nickname = nick;
    identify_user(client);

    reply_user(service, client, NS_REG_COMPLETE, client->name);
    global_notice(NULL, "%s!%s@%s registered nick %s\n", client->name, 
        client->username, client->host, nick->nick);

    return;
  }
  reply_user(service, client, NS_REG_FAIL, client->name);
}


/**
 * someone wants to drop a nick
 */
static void
m_drop(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  if(!IsIdentified(client)) 
  {
    reply_user(service, client, NS_NEED_IDENTIFY, client->name);
  }

  if(db_delete_nick(client->name)) 
  {
    client->service_handler = UNREG_HANDLER;
    ClearIdentified(client);
    send_umode(nickserv, client, "-R");

    reply_user(service, client, NS_NICK_DROPPED, client->name);
    global_notice(NULL, "%s!%s@%s dropped nick %s\n", client->name, 
      client->username, client->host, client->name);
  } 
  else
  {
    global_notice(NULL, "Error: %s!%s@%s could not DROP nick %s\n", client->name, 
      client->username, client->host, client->name);
    reply_user(service, client, NS_NICK_DROPFAIL, client->name);
  }
}

/**
 * someone wants to identify with services
 */
static void
m_identify(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Nick *nick;

  if((nick = db_find_nick(client->name)) == NULL)
  {
    reply_user(service, client, NS_REG_FIRST, client->name);
    return;
  }

  if(!check_nick_pass(nick, parv[1]))
  {
    free_nick(nick);
    reply_user(service, client, NS_IDENT_FAIL, client->name);
    return;
  }
  client->nickname = nick;
  dlinkFindDelete(&nick_enforce_list, client);

  identify_user(client);
  reply_user(service, client, NS_IDENTIFIED, client->name);
}

static void
m_help(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  do_help(service, client, parv[1], parc, parv);
}

static void
m_set(struct Service *service, struct Client *client, int parc, char *parv[])
{
  reply_user(service, client, 0, "Unknown SET option");
}

static void
m_set_password(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  struct Nick *nick;
  char *pass;
  char password[PASSLEN*2+1];
  char salt[SALTLEN+1];
  
  nick = client->nickname;

  memset(salt, 0, sizeof(salt));
  
  make_random_string(salt, SALTLEN+1);
  snprintf(password, sizeof(password), "%s%s", parv[1], salt);
  
  pass = crypt_pass(password);
  if(db_set_string(SET_NICK_PASSWORD, client->nickname->id, pass))
  {
    reply_user(service, client, NS_SET_SUCCESS, "PASSWORD", "hidden");
  }
  else
  {
    reply_user(service, client, NS_SET_FAILED, "PASSWORD", "hidden");
    MyFree(pass);
    return;
  }
  strlcpy(nick->pass, pass, sizeof(nick->pass));
  MyFree(pass);
}

static void
m_set_language(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  /* No paramters, list languages */
  if(parc == 0)
  {
    int i;

    reply_user(service, client, NS_CURR_LANGUAGE,
        service->language_table[client->nickname->language][0],
        client->nickname->language);

    reply_user(service, client, NS_AVAIL_LANGUAGE);

    for(i = 0; i < LANG_LAST; i++)
    {
      reply_user(service, client, NS_LANG_LIST, i, 
          service->language_table[i][0]);
    }
  }
  else
  {
    int lang = atoi(parv[1]);
    
    if(db_set_number(SET_NICK_LANGUAGE, client->nickname->id, lang))
    {
      client->nickname->language = lang;
      reply_user(service, client, NS_LANGUAGE_SET,
          service->language_table[lang][0], lang); 
    }
    else
      reply_user(service, client, 0, ":(");
  }
}

static void
m_set_url(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  struct Nick *nick = client->nickname;
  
  if(parc == 0)
  {
    reply_user(service, client, NS_SET_VALUE, 
        "URL", nick->url);
    return;
  }
  
  if(db_set_string(SET_NICK_URL, nick->id, parv[1]))
  {
    nick->url = replace_string(nick->url, parv[1]);
    reply_user(service, client, NS_SET_SUCCESS, "URL", nick->url);
  }
  else
    reply_user(service, client, NS_SET_FAILED, "URL", parv[1]);
}

static void
m_set_email(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  struct Nick *nick = client->nickname;
  
  if(parc == 0)
  {
    reply_user(service, client, NS_SET_VALUE, "EMAIL", nick->email);
    return;
  }
    
  if(db_set_string(SET_NICK_EMAIL, nick->id, parv[1]))
  {
    nick->email = replace_string(nick->email, parv[1]);
    reply_user(service, client, NS_SET_SUCCESS, "EMAIL", nick->email);
  }
  else
    reply_user(service, client, NS_SET_FAILED, "EMAIL", parv[1]);
}

static void
m_set_cloak(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  struct Nick *nick = client->nickname;
  unsigned char flag;
  
  if(parc == 0)
  {
    reply_user(service, client, NS_SET_VALUE, "CLOAK", 
        nick->cloak_on ? "ON" : "OFF");
    return;
  }

  if(strcasecmp(parv[1], "ON") == 0)
    flag = 1;
  else if(strcasecmp(parv[1], "OFF") == 0)
    flag = 0;
  else
  {
    reply_user(service, client, NS_SET_VALUE, "CLOAK", 
        nick->cloak_on ? "ON" : "OFF");
    return;
  }

  if(db_set_bool(SET_NICK_CLOAKON, nick->id, flag))
  {
    nick->cloak_on = flag;
    reply_user(service, client, NS_SET_SUCCESS, "CLOAK", flag ? "ON" : "OFF");
  }
  else
    reply_user(service, client, NS_SET_FAILED, "CLOAK", parv[1]);
}
static void
m_set_cloakstring(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  struct Nick *nick = client->nickname;
 
  if(parc == 0)
  {
    reply_user(service, client, NS_SET_VALUE, "CLOAKSTRING", nick->cloak);
    return;
  }
    
  /* Need admin to change, but can view no problem */
  if(!nick->admin)
  {
    m_notadmin(service, client, parc, parv);
    return;
  }
  if(db_set_string(SET_NICK_CLOAK, nick->id, parv[1]))
  {
    strlcpy(nick->cloak, parv[1], sizeof(nick->cloak));
    reply_user(service, client, NS_SET_SUCCESS, "CLOAKSTRING", nick->cloak);
  }
  else
    reply_user(service, client, NS_SET_FAILED, "CLOAKSTRING", parv[1]);
}

static void
m_set_secure(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Nick *nick = client->nickname;
  unsigned char flag;
  
  if(parc == 0)
  {
    reply_user(service, client, NS_SET_VALUE, "SECURE", 
        nick->secure ? "ON" : "OFF");
    return;
  }

  if(strcasecmp(parv[1], "ON") == 0)
    flag = 1;
  else if(strcasecmp(parv[1], "OFF") == 0)
    flag = 0;
  else
  {
    reply_user(service, client, NS_SET_VALUE, "SECURE", 
        nick->secure ? "ON" : "OFF");
    return;
  }

  if(db_set_bool(SET_NICK_SECURE, nick->id, flag))
  {
    nick->secure = flag;
    reply_user(service, client, NS_SET_SUCCESS, "SECURE", flag ? "ON" : "OFF");
  }
  else
    reply_user(service, client, NS_SET_FAILED, "SECURE", parv[1]);
}

static void
m_set_enforce(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Nick *nick = client->nickname;
  unsigned char flag;
  
  if(parc == 0)
  {
    reply_user(service, client, NS_SET_VALUE, "ENFORCE", 
       nick->enforce ? "ON" : "OFF");
    return;
  }

  if(strcasecmp(parv[1], "ON") == 0)
    flag = 1;
  else if(strcasecmp(parv[1], "OFF") == 0)
    flag = 0;
  else
  {
    reply_user(service, client, NS_SET_VALUE, "ENFORCE", 
        nick->enforce ? "ON" : "OFF");
    return;
  }

  if(db_set_bool(SET_NICK_ENFORCE, nick->id, flag))
  {
    nick->enforce = flag;
    reply_user(service, client, NS_SET_SUCCESS, "ENFORCE", flag ? "ON" : "OFF");
  }
  else
    reply_user(service, client, NS_SET_FAILED, "ENFORCE", parv[1]);
}

static void
m_access(struct Service *service, struct Client *client, int parc, char *parv[])
{
  reply_user(service, client, 0, 
      "Syntax: ACCESS {ADD | DEL | LIST} [mask | list]");
}

static void
m_access_add(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  struct Nick *nick = client->nickname;
  struct AccessEntry access;

  if(strchr(parv[1], '@') == NULL)
  {
    reply_user(service, client, NS_ACCESS_INVALID, parv[1]);
    return;
  }

  access.id = nick->id;
  access.value = parv[1];
  
  if(db_list_add(ACCESS_LIST, (void *)&access))
  {
    reply_user(service, client, NS_ACCESS_ADD, parv[1]);
  }
  else
  {
    reply_user(service, client, NS_ACCESS_ADDFAIL, parv[1]);
  }
}

static void
m_access_list(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  struct Nick *nick;
  struct AccessEntry *entry;
  void *first, *listptr;
  int i = 1;

  nick = client->nickname;

  reply_user(service, client, NS_ACCESS_START);
 
  if((listptr = db_list_first(ACCESS_LIST, nick->id, (void**)&entry)) == NULL)
  {
    return;
  }

  first = listptr;

  while(listptr != NULL)
  {
    reply_user(service, client, NS_ACCESS_ENTRY, i++, entry->value);
    MyFree(entry->value);
    MyFree(entry);
    listptr = db_list_next(listptr, ACCESS_LIST, (void**)&entry);
  }

  db_list_done(first);
}

static void
m_access_del(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  struct Nick *nick = client->nickname;
  int index, ret;

  index = atoi(parv[1]);
  if(index > 0)
    ret = db_list_del_index(DELETE_NICKACCESS_IDX, nick->id, index);
  else
    ret = db_list_del(DELETE_NICKACCESS, nick->id, parv[1]);
  
  reply_user(service, client, NS_ACCESS_DEL, ret);
}

static void
m_ghost(struct Service *service, struct Client *client, int parc, char *parv[])
{
  struct Nick *nick;

  if((nick = db_find_nick(parv[1])) == NULL)
  {
    reply_user(service, client, NS_REG_FIRST, parv[1]);
    return;
  }

  if(strncmp(client->name, parv[1], NICKLEN) == 0)
  {
    free_nick(nick);
    reply_user(service, client, NS_GHOST_NOSELF, parv[1]);
    return;
  }

  if(!check_nick_pass(nick, parv[2]))
  {
    free_nick(nick);
    reply_user(service, client, NS_GHOST_FAILED, parv[1]);   
    return;
  }

  reply_user(service, client, NS_GHOST_SUCCESS, parv[1]);
  /* XXX Turn this into send_kill */
  sendto_server(me.uplink, ":%s KILL %s :%s (GHOST Command recieved from %s)", 
      service->name, parv[1], "services.oftc.net", client->name);
}

static void
m_link(struct Service *service, struct Client *client, int parc, char *parv[])
{
#if 0
  struct Nick *nick, *master_nick;

  nick = client->nickname;
  if((master_nick = db_find_nick(parv[1])) == NULL)
  {
    reply_user(service, client, NS_LINK_NOMASTER, parv[1]);
    return;
  }

  if(!check_nick_pass(master_nick, parv[2]))
  {
    free_nick(master_nick);
    reply_user(service, client, NS_LINK_BADPASS, parv[1]);
    return;
  }

  if(db_link_nicks(master_nick->id, nick->id) != 0)
  {
    free_nick(master_nick);
    reply_user(service, client, NS_LINK_FAIL, parv[1]);
    return;
  }
  strlcpy(master_nick->nick, nick->nick, sizeof(master_nick->nick));
  client->nickname = master_nick;
  reply_user(service, client, NS_LINK_OK, nick->nick, parv[1]);
  free_nick(nick);
  nick = NULL;
#endif
}

static void
m_unlink(struct Service *service, struct Client *client, int parc, char *parv[])
{
  struct Nick *nick = client->nickname;
  struct Nick *unlinked_nick;

//  if(!db_nick_is_linked(client->name))
//  {
 //   reply_user(service, client, NS_UNLINK_NOLINK, client->name);
 //   return;
 // }
/*  if((unlinked_nick = db_unlink_nick(client->name)) != NULL)
  {
    reply_user(service, client, NS_UNLINK_OK, client->name);
    free_nick(nick);
    client->nickname = unlinked_nick;
  }
  else
    reply_user(service, client, NS_UNLINK_FAILED, client->name);*/
}

static void
m_info(struct Service *service, struct Client *client, int parc, char *parv[])
{
  struct Nick *nick;
  char regtime[IRC_BUFSIZE/2+1];
  char quittime[IRC_BUFSIZE/2+1];

  if(parc == 0)
  {
    if(client->nickname != NULL)
      nick = client->nickname;
    else
    {
      if((nick = db_find_nick(client->name)) == NULL)
      {
        reply_user(service, client, NS_REG_FIRST, client->name);
        return;
      }
    }
  }
  else
  {
    if((nick = db_find_nick(parv[1])) == NULL)
    {
      reply_user(service, client, NS_REG_FIRST,
          parv[1]);
      return;
    }
  }

  strftime(regtime, IRC_BUFSIZE/2, "%a,  %d  %b  %Y  %H:%M:%S  %z", 
      gmtime(&nick->reg_time));
  strftime(quittime, IRC_BUFSIZE/2, "%a,  %d  %b  %Y  %H:%M:%S  %z", 
      gmtime(&nick->last_quit_time));
      
  reply_user(service, client, NS_INFO, regtime, nick->last_quit, quittime, 
      nick->email);
  
  if(nick != client->nickname)
    free_nick(nick);
}

static void
m_forbid(struct Service *service, struct Client *client, int parc, char *parv[])
{
  struct Nick *nick;
  char *pass;
  char password[PASSLEN+SALTLEN+1];
  char randpass[PASSLEN+1];

  if((nick = db_find_nick(parv[1])) != NULL)
  {
    db_set_bool(SET_NICK_FORBIDDEN, nick->id, TRUE);
    free_nick(nick);
    reply_user(service, client, NS_FORBID_OK, parv[1]);
    return;
  }

  nick = MyMalloc(sizeof(struct Nick));

  make_random_string(nick->salt, SALTLEN+1);
  make_random_string(randpass, PASSLEN);
  snprintf(password, sizeof(password), "%s%s", randpass, nick->salt);

  pass = crypt_pass(password);
  strlcpy(nick->pass, pass, sizeof(nick->pass));
  strlcpy(nick->nick, parv[1], sizeof(nick->nick));
  DupString(nick->email, "Forbidden Nick");
  MyFree(pass);
  
  if(!db_register_nick(nick))
  {
    reply_user(service, client, NS_FORBID_FAIL, parv[1]);
    return;
  }

  db_set_bool(SET_NICK_FORBIDDEN, nick->id, TRUE);
  reply_user(service, client, NS_FORBID_OK, parv[1]);
  free_nick(nick);
}

static void*
ns_on_umode_change(va_list args) 
{
  struct Client *user = va_arg(args, struct Client *);
  int what            = va_arg(args, int);
  int umode           = va_arg(args, int);

  return pass_callback(ns_nick_hook, user, what, umode);
}

static void *
ns_on_nick_change(va_list args)
{
  struct Client *user = va_arg(args, struct Client *);
  char *oldnick       = va_arg(args, char *);
  struct Nick *nick_p;
  char userhost[USERHOSTLEN+1]; 

  ilog(L_DEBUG, "%s changing nick to %s", oldnick, user->name);
 
  dlinkFindDelete(&nick_enforce_list, user);
  if(IsIdentified(user))
  {
    /* XXX Check links */
    user->service_handler = UNREG_HANDLER;
    ClearIdentified(user);
    /* XXX Use unidentify event */
    send_umode(nickserv, user, "-R");
  }

  if((nick_p = db_find_nick(user->name)) == NULL)
  {
    ilog(L_DEBUG, "Nick Change: %s->%s(nick not registered)", oldnick, user->name);
    return pass_callback(ns_nick_hook, user, oldnick);
  }

  if(nick_p->forbidden)
  {
    reply_user(nickserv, user, NS_NICK_FORBID_IWILLCHANGE, user->name);
    user->enforce_time = CurrentTime + 60; /* XXX configurable? */
    dlinkAdd(user, make_dlink_node(), &nick_enforce_list);
    free_nick(nick_p);
    return pass_callback(ns_nick_hook, user, oldnick);
  }

  snprintf(userhost, USERHOSTLEN, "%s@%s", user->username, user->host);
  if(check_list_entry("nickname_access", nick_p->id, userhost))
  {
    ilog(L_DEBUG, "%s changed nick to %s(found access entry)", oldnick, user->name);
    SetOnAccess(user);
    if(!nick_p->secure)
    {
      user->nickname = nick_p;
      identify_user(user);
    }
  }
  else
  {
    if(nick_p->enforce)
    {
      reply_user(nickserv, user, NS_NICK_IN_USE_IWILLCHANGE, user->name);
      user->enforce_time = CurrentTime + 60; /* XXX configurable? */
      dlinkAdd(user, make_dlink_node(), &nick_enforce_list);
    }
    else
    {
      reply_user(nickserv, user, NS_NICK_IN_USE, user->name);
    }
 
    ilog(L_DEBUG, "%s changed nick to %s(no access entry)", oldnick, user->name);
  }
  
  return pass_callback(ns_nick_hook, user, oldnick);
}

static void *
ns_on_newuser(va_list args)
{
  struct Client *newuser = va_arg(args, struct Client*);
  struct Nick *nick_p;
  char userhost[USERHOSTLEN+1];
  
  if(IsMe(newuser->from))
    return pass_callback(ns_newuser_hook, newuser);

  ilog(L_DEBUG, "New User: %s!", newuser->name);

  if((nick_p = db_find_nick(newuser->name)) == NULL)
  {
    ilog(L_DEBUG, "New user: %s(nick not registered)", newuser->name);
    return pass_callback(ns_newuser_hook, newuser);
  }

  if(nick_p->forbidden)
  {
    reply_user(nickserv, newuser, NS_NICK_FORBID_IWILLCHANGE, newuser->name);
    newuser->enforce_time = CurrentTime + 60; /* XXX configurable? */
    dlinkAdd(newuser, make_dlink_node(), &nick_enforce_list);
    free_nick(nick_p);
    return pass_callback(ns_newuser_hook, newuser);
  }

  if(IsIdentified(newuser))
  {
    reply_user(nickserv, newuser, NS_NICK_AUTOID, newuser->name);
    newuser->nickname = nick_p;
    identify_user(newuser);
    return pass_callback(ns_newuser_hook, newuser);
  }

  snprintf(userhost, USERHOSTLEN, "%s@%s", newuser->username, newuser->host);
  if(check_list_entry("nickname_access", nick_p->id, userhost))
  {
    ilog(L_DEBUG, "new user: %s(found access entry)", newuser->name);
    SetOnAccess(newuser);
    if(!nick_p->secure)
    {
      newuser->nickname = nick_p;
      identify_user(newuser);
    }
  }
  else
  {
    if(nick_p->enforce)
    {
      reply_user(nickserv, newuser, NS_NICK_IN_USE_IWILLCHANGE, newuser->name);
      newuser->enforce_time = CurrentTime + 60; /* XXX configurable? */
      dlinkAdd(newuser, make_dlink_node(), &nick_enforce_list);
    }
    else
    {
      reply_user(nickserv, newuser, NS_NICK_IN_USE, newuser->name);
    }
    ilog(L_DEBUG, "new user:%s(no access entry)", newuser->name);
  }
  
  return pass_callback(ns_newuser_hook, newuser);
}

static void *
ns_on_quit(va_list args)
{
  struct Client *user     = va_arg(args, struct Client *);
  char          *comment  = va_arg(args, char *);
  struct Nick *nick = user->nickname;

  if(nick)
  {
    db_set_string(SET_NICK_LAST_QUIT, nick->id, comment);
    db_set_string(SET_NICK_LAST_HOST, nick->id, user->host);
    db_set_string(SET_NICK_LAST_REALNAME, nick->id, user->info);
    db_set_number(SET_NICK_LAST_QUITTIME, nick->id, CurrentTime);
    db_set_number(SET_NICK_LAST_SEEN, nick->id, CurrentTime);
    free_nick(nick);
  }

  return pass_callback(ns_quit_hook, user, comment);
}
