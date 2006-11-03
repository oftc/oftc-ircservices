/*
 *  oftc-ircservices: an exstensible and flexible IRC Services package
 *  client.c: Client functions
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

dlink_list global_client_list;
dlink_list global_server_list;
static int clean_nick_name(char *, int);
static int clean_user_name(char *);
static int clean_host_name(char *);

static BlockHeap *client_heap  = NULL;

unsigned int user_modes[256] =
{
  /* 0x00 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x0F */
  /* 0x10 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x1F */
  /* 0x20 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x2F */
  /* 0x30 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x3F */
  0,                  /* @ */
  0,                  /* A */
  0,                  /* B */
  0,                  /* C */
  UMODE_DEAF,         /* D */
  0,                  /* E */
  0,                  /* F */
  UMODE_SOFTCALLERID, /* G */
  0,                  /* H */
  0,                  /* I */
  0,                  /* J */
  0,                  /* K */
  0,                  /* L */
  0,                  /* M */
  0,                  /* N */
  0,                  /* O */
  0,                  /* P */
  0,                  /* Q */
  0,                  /* R */
  0,                  /* S */
  0,                  /* T */
  0,                  /* U */
  0,                  /* V */
  0,                  /* W */
  0,                  /* X */
  0,                  /* Y */
  0,                  /* Z 0x5A */
  0, 0, 0, 0, 0,      /* 0x5F   */
  0,                  /* 0x60   */
  UMODE_ADMIN,        /* a */
  UMODE_BOTS,         /* b */
  UMODE_CCONN,        /* c */
  UMODE_DEBUG,        /* d */
  0,                  /* e */
  UMODE_FULL,         /* f */
  UMODE_CALLERID,     /* g */
  0,                  /* h */
  UMODE_INVISIBLE,    /* i */
  0,                  /* j */
  UMODE_SKILL,        /* k */
  UMODE_LOCOPS,       /* l */
  0,                  /* m */
  UMODE_NCHANGE,      /* n */
  UMODE_OPER,         /* o */
  0,                  /* p */
  0,                  /* q */
  UMODE_REJ,          /* r */
  UMODE_SERVNOTICE,   /* s */
  0,                  /* t */
  UMODE_UNAUTH,       /* u */
  0,                  /* v */
  UMODE_WALLOP,       /* w */
  UMODE_EXTERNAL,     /* x */
  UMODE_SPY,          /* y */
  UMODE_OPERWALL,     /* z      0x7A */
  0,0,0,0,0,          /* 0x7B - 0x7F */

  /* 0x80 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x8F */
  /* 0x90 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x9F */
  /* 0xA0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xAF */
  /* 0xB0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xBF */
  /* 0xC0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xCF */
  /* 0xD0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xDF */
  /* 0xE0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xEF */
  /* 0xF0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  /* 0xFF */
};

void
init_client()
{
  client_heap = BlockHeapCreate("client", sizeof(struct Client), CLIENT_HEAP_SIZE);
}

struct Client *
make_client(struct Client *from)
{
  struct Client *client = BlockHeapAlloc(client_heap);

  if(from == NULL)
    client->from = client;
  else
    client->from = from;

  client->hnext  = client;
  strlcpy(client->username, "unknown", sizeof(client->username));

  return client;
}

struct Server *
make_server(struct Client *client)
{
  if(client->server == NULL)
    client->server = MyMalloc(sizeof(struct Server));

  return client->server;
}

/* find_person()
 *
 * inputs - pointer to name
 * output - return client pointer
 * side effects - find person by (nick)name
 */
struct Client *
find_person(const struct Client *source, const char *name)
{
  struct Client *target = NULL;

  if(IsDigit(*name) && IsServer(source->from))
    target = hash_find_id(name);
  else
    target = find_client(name);

  return(target && IsClient(target)) ? target : NULL;
}

/*
 * find_chasing - find the client structure for a nick name (user)
 *      using history mechanism if necessary. If the client is not found,
 *      an error message (NO SUCH NICK) is generated. If the client was found
 *      through the history, chasing will be 1 and otherwise 0.
 */
struct Client *
find_chasing(struct Client *source_p, const char *user, int *chasing)
{
  struct Client *who = find_person(source_p, user);

  if (chasing)
    *chasing = 0;

  if (who)
    return who;

  if (IsDigit(*user))
    return NULL;

  return NULL;
}


/*
 * dead_link_on_write - report a write error if not already dead,
 *      mark it as dead then exit it
 */
void
dead_link_on_write(struct Client *client, int ierrno)
{
  if (IsDefunct(client->server))
    return;

  dbuf_clear(&client->server->buf_recvq);
  dbuf_clear(&client->server->buf_sendq);
}
/*
 * close_connection
 *        Close the physical connection. This function sets client_p->from == NU
 */
static void
close_connection(struct Client *client_p)
{
  assert(NULL != client_p);

  if (!IsDead(client_p))
  {
    /* attempt to flush any pending dbufs. Evil, but .. -- adrian */
    /* there is still a chance that we might send data to this socket
     * even if it is marked as blocked (COMM_SELECT_READ handler is called
     * before COMM_SELECT_WRITE). Let's try, nothing to lose.. -adx
     */
    send_queued_write(client_p);
  }

  if (client_p->server->fd.flags.open)
    fd_close(&client_p->server->fd);

  dbuf_clear(&client_p->server->buf_sendq);
  dbuf_clear(&client_p->server->buf_recvq);

  client_p->from = NULL; /* ...this should catch them! >:) --msa */

  printf("Closed connection to %s\n", client_p->name);
}

/*
 * Exit one client, local or remote. Assuming all dependents have
 * been already removed, and socket closed for local client.
 *
 * The only messages generated are QUITs on channels.
 */
static void
exit_one_client(struct Client *source_p)
{
  dlink_node *lp = NULL, *next_lp = NULL;

  assert(!IsMe(source_p) && (source_p != &me));

  if (IsClient(source_p))
  {
    assert(source_p->servptr);

   dlinkDelete(&source_p->lnode, &source_p->servptr->client_list);

    /*
     * If a person is on a channel, send a QUIT notice to every
     * client (person) on the same channel (so that the client
     * can show the "**signoff" message).  (Note: The notice is
     * to the local clients *only*)
     */
    DLINK_FOREACH_SAFE(lp, next_lp, source_p->channel.head)
      remove_user_from_channel(lp->data);
  }

  if (IsServer(source_p))
  {
    dlinkDelete(&source_p->lnode, &source_p->servptr->server_list);

/*    if ((lp = dlinkFindDelete(&global_serv_list, source_p)) != NULL)
    free_dlink_node(lp);*/
  }

  if (source_p->name[0])
    hash_del_client(source_p);

  /* remove from global client list
   * NOTE: source_p->node.next cannot be NULL if the client is added
   *       to global_client_list (there is always &me at its end)
   */
  if (source_p != NULL && source_p->node.next != NULL)
    dlinkDelete(&source_p->node, &global_client_list);

  printf("exited: %s\n", source_p->name);
}

/*
 * Remove all clients that depend on source_p; assumes all (S)QUITs have
 * already been sent.  we make sure to exit a server's dependent clients
 * and servers before the server itself; exit_one_client takes care of
 * actually removing things off llists.   tweaked from +CSr31  -orabidoo
 */
static void
recurse_remove_clients(struct Client *source_p)
{
  dlink_node *ptr = NULL, *next = NULL;

  DLINK_FOREACH_SAFE(ptr, next, source_p->client_list.head)
    exit_one_client(ptr->data);

  DLINK_FOREACH_SAFE(ptr, next, source_p->server_list.head)
  {
    recurse_remove_clients(ptr->data);
    exit_one_client(ptr->data);
  }
}


/*
** Remove *everything* that depends on source_p, from all lists, and sending
** all necessary QUITs and SQUITs.  source_p itself is still on the lists,
** and its SQUITs have been sent except for the upstream one  -orabidoo
*/
static void
remove_dependents(struct Client *source_p, struct Client *from,
                  const char *comment)
{
  recurse_remove_clients(source_p);
}


/*
 * exit_client - exit a client of any type. Generally, you can use
 * this on any struct Client, regardless of its state.
 *
 * Note, you shouldn't exit remote _users_ without first doing
 * SetKilled and propagating a kill or similar message. However,
 * it is perfectly correct to call exit_client to force a _server_
 * quit (either local or remote one).
 *
 * inputs:       - a client pointer that is going to be exited
 *               - for servers, the second argument is a pointer to who
 *                 is firing the server. This side won't get any generated
 *                 messages. NEVER NULL!
 * output:       none
 * side effects: the client is delinked from all lists, disconnected,
 *               and the rest of IRC network is notified of the exit.
 *               Client memory is scheduled to be freed
 */
void
exit_client(struct Client *source_p, struct Client *from, const char *comment)
{
  if (source_p->server != NULL)
  {
    /*
     * DO NOT REMOVE. exit_client can be called twice after a failed
     * read/write.
     */
    if (IsClosing(source_p))
      return;

    SetClosing(source_p);

    if (!IsDead(source_p))
    {
    /*
     * Close the Client connection first and mark it so that no
     * messages are attempted to send to it.  Remember it makes
     * source_p->from == NULL.
     */
      close_connection(source_p);
    }
  }

  if (IsServer(source_p))
  {
    remove_dependents(source_p, from->from, comment);
  }
    
  if(IsMe(source_p->from))
  {
    sendto_server(me.uplink, ":%s QUIT :%s", source_p->name, comment);
  }

  exit_one_client(source_p);
}

/* check_clean_nick()
 *
 * input  - pointer to source
 *    -
 *    - nickname
 *    - truncated nickname
 *    - origin of client
 *    - pointer to server nick is coming from
 * output - none
 * side effects - if nickname is erroneous, or a different length to
 *                truncated nickname, return 1
 */
int
check_clean_nick(struct Client *client_p, struct Client *source_p,
                 char *nick, char *newnick, struct Client *server_p)
{
  /* the old code did some wacky stuff here, if the nick is invalid, kill it
   * and dont bother messing at all
   */
  if (!clean_nick_name(nick, 0) || strcmp(nick, newnick))
  {
    printf("Bad Nick: %s From: %s(via %s)",
        nick, server_p->name, client_p->name);

    /* bad nick change */
    if (source_p != client_p)
    {
      exit_client(source_p, &me, "Bad Nickname");
    }

    return 1;
  }

  return 0;
}

/* check_clean_user()
 *
 * input  - pointer to client sending data
 *              - nickname
 *              - username to check
 *    - origin of NICK
 * output - none
 * side effects - if username is erroneous, return 1
 */
int
check_clean_user(struct Client *client_p, char *nick,
                 char *user, struct Client *server_p)
{
  if (strlen(user) > USERLEN)
  {
    printf("Long Username: %s Nickname: %s From: %s(via %s)",
        user, nick, server_p->name, client_p->name);

    return 1;
  }

  if (!clean_user_name(user))
    printf("Bad Username: %s Nickname: %s From: %s(via %s)",
       user, nick, server_p->name, client_p->name);

  return 0;
}

/* check_clean_host()
 *
 * input  - pointer to client sending us data
 *              - nickname
 *              - hostname to check
 *    - source name
 * output - none
 * side effects - if hostname is erroneous, return 1
 */
int
check_clean_host(struct Client *client_p, char *nick,
                 char *host, struct Client *server_p)
{
  if (strlen(host) > HOSTLEN)
  {
    printf("Long Hostname: %s Nickname: %s From: %s(via %s)",
        host, nick, server_p->name, client_p->name);

    return 1;
  }

  if (!clean_host_name(host))
    printf("Bad Hostname: %s Nickname: %s From: %s(via %s)",
        host, nick, server_p->name, client_p->name);

  return 0;
}

/* clean_nick_name()
 *
 * input  - nickname
 *              - whether it's a local nick (1) or remote (0)
 * output - none
 * side effects - walks through the nickname, returning 0 if erroneous
 */
static int
clean_nick_name(char *nick, int local)
{
  assert(nick);

  /* nicks cant start with a digit or - or be 0 length */
  /* This closer duplicates behaviour of hybrid-6 */
  if (*nick == '-' || (IsDigit(*nick) && local) || *nick == '\0')
    return 0;

  for (; *nick; ++nick)
    if (!IsNickChar(*nick))
      return 0;

  return 1;
}

/* clean_user_name()
 *
 * input  - username
 * output - none
 * side effects - walks through the username, returning 0 if erroneous
 */
static int
clean_user_name(char *user)
{
  assert(user);

  for (; *user; ++user)
    if (!IsUserChar(*user))
      return 0;

  return 1;
}

/* clean_host_name()
 * input  - hostname
 * output - none
 * side effects - walks through the hostname, returning 0 if erroneous
 */
static int
clean_host_name(char *host)
{
  assert(host);

  for (; *host; ++host)
    if (!IsHostChar(*host))
      return 0;

  return 1;
}

/* set_user_mode()
 *
 * added 15/10/91 By Darren Reed.
 * parv[0] - sender
 * parv[1] - username to change mode for
 * parv[2] - modes to change
 */
void
set_user_mode(struct Client *client_p, struct Client *source_p,
              int parc, char *parv[])
{
  unsigned int flag, setflags;
  char **p, *m; 
  struct Client *target_p;
  int what = MODE_ADD;

  assert(!(parc < 2));

  if ((target_p = find_person(client_p, parv[1])) == NULL)
  {
    return;
  }

  if (IsServer(source_p))
  {
     return;
  }

  if (source_p != target_p || target_p->from != source_p->from)
  {
     return;
  }

  //execute_callback(entering_umode_cb, client_p, source_p);

  /* find flags already set for user */
  setflags = source_p->umodes;

  /* parse mode change string(s) */
  for (p = &parv[2]; p && *p; p++)
  {
    for (m = *p; *m; m++)
    {
      switch (*m)
      {
        case '+':
          what = MODE_ADD;
          break;
        case '-':
          what = MODE_DEL;
          break;
        case 'o':
          if (what == MODE_ADD)
          {
            if (IsServer(client_p) && !IsOper(source_p))
            {
              printf("Setting %s!%s@%s as oper\n", source_p->name,
                  source_p->username, source_p->host);
              SetOper(source_p);
            }
          }
          else
          {
            /* Only decrement the oper counts if an oper to begin with
             * found by Pat Szuta, Perly , perly@xnet.com
             */
            if (!IsOper(source_p))
              break;

            ClearOper(source_p);

          }

          break;

        /* we may not get these,
         * but they shouldnt be in default
         */
        case ' ' :
        case '\n':
        case '\r':
        case '\t':
          break;


        default:
          if ((flag = user_modes[(unsigned char)*m]))
          {
/*            else
              execute_callback(umode_cb, client_p, source_p, what, flag);*/
          }
          break;
      }
    }
  }

  //send_umode_out(client_p, source_p, setflags);
}

/* register_remote_user()
 *
 * inputs       - client_p directly connected client
 *              - source_p remote or directly connected client
 *              - username to register as
 *              - host name to register as
 *              - server name
 *              - realname (gecos)
 * output - NONE
 * side effects - This function is called when a remote client
 *      is introduced by a server.
 */
void
register_remote_user(struct Client *client_p, struct Client *source_p,
                     const char *username, const char *host, const char *server,
                     const char *realname)
{
  struct Client *target_p = NULL;

  assert(source_p != NULL);
  assert(source_p->username != username);

  strlcpy(source_p->host, host, sizeof(source_p->host));
  strlcpy(source_p->info, realname, sizeof(source_p->info));
  strlcpy(source_p->username, username, sizeof(source_p->username));

  /*
   * coming from another server, take the servers word for it
   */
  source_p->servptr = find_server(server);

  /* Super GhostDetect:
   * If we can't find the server the user is supposed to be on,
   * then simply blow the user away.        -Taner
   */
  if (source_p->servptr == NULL)
  {
    printf("No server %s for user %s[%s@%s] from %s",
        server, source_p->name, source_p->username,
        source_p->host, source_p->from->name);
    exit_client(source_p, &me, "Ghosted Client");
    return;
  }

  if ((target_p = source_p->servptr) && target_p->from != source_p->from)
  {
    printf("Bad User [%s] :%s USER %s@%s %s, != %s[%s]",
        client_p->name, source_p->name, source_p->username,
        source_p->host, source_p->servptr->name,
        target_p->name, target_p->from->name);
    exit_client(source_p, &me, "USER server wrong direction");
    return;
  }

  /* Increment our total user count here */

  SetClient(source_p);
  dlinkAdd(source_p, &source_p->lnode, &source_p->servptr->client_list);
  printf("Adding client %s!%s@%s from %s\n", source_p->name, source_p->username,
      source_p->host, server);
}

/*
 * nick_from_server()
 */
void
nick_from_server(struct Client *client_p, struct Client *source_p, int parc,
                 char *parv[], time_t newts, char *nick, char *ngecos)
{
  int samenick = 0;

  if (IsServer(source_p))
  {
    /* A server introducing a new client, change source */
    source_p = make_client(client_p);
    dlinkAdd(source_p, &source_p->node, &global_client_list);

    if (parc > 2)
      source_p->hopcount = atoi(parv[2]);
    if (newts)
      source_p->tsinfo = newts;
    else
    {
      newts = source_p->tsinfo = CurrentTime;
      printf("Remote nick %s (%s) introduced without a TS", nick, parv[0]);
    }

    /* copy the nick in place */
    strlcpy(source_p->name, nick, sizeof(source_p->name));
    hash_add_client(source_p);

    if (parc > 8)
    {
      unsigned int flag;
      char *m;

      /* parse usermodes */
      m = &parv[4][1];

      while (*m)
      {
        flag = user_modes[(unsigned char)*m];

        source_p->umodes |= flag;
        printf("Setting umode %c on %s\n", *m, source_p->name);
        m++;
      }

      register_remote_user(client_p, source_p, parv[5], parv[6],
                           parv[7], ngecos);
      return;
    }
  }
  else if (source_p->name[0])
  {
    samenick = !irccmp(parv[0], nick);


    /* client changing their nick */
    if (!samenick)
    {
      source_p->tsinfo = newts ? newts : CurrentTime;
    }
  }

  /* set the new nick name */
  assert(source_p->name[0]);

  hash_del_client(source_p);
  strcpy(source_p->name, nick);
  hash_add_client(source_p);
}

