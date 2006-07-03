#include "stdinc.h"

dlink_list global_client_list;
dlink_list global_server_list;
static int clean_nick_name(char *, int);
static int clean_user_name(char *);
static int clean_host_name(char *);


static BlockHeap *client_heap  = NULL;

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
  struct Client *to;
  struct ConfItem *conf;
  struct AccessItem *aconf;
  static char myname[HOSTLEN+1];
  dlink_node *ptr;

/*  DLINK_FOREACH(ptr, serv_list.head)
  {
    to = ptr->data;

    if ((conf = to->serv->sconf) != NULL)
    {
      aconf = &conf->conf.AccessItem;
      strlcpy(myname, my_name_for_link(aconf), sizeof(myname));
    }
    else
      strlcpy(myname, me.name, sizeof(myname));
    recurse_send_quits(source_p, source_p, from, to,
                       comment, splitstr, myname);
  }
*/
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
  dlink_node *m = NULL;

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
