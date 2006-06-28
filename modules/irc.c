#include "stdinc.h"

static void m_ping(struct Client *, struct Client *, int, char *[]);
static void m_nick(struct Client *, struct Client *, int, char*[]);
static void m_server(struct Client *, struct Client *, int, char*[]);
static void m_sjoin(struct Client *, struct Client *, int, char*[]);
static void m_part(struct Client *, struct Client *, int, char*[]);
static void m_quit(struct Client *, struct Client *, int, char*[]);
static void m_squit(struct Client *, struct Client *, int, char*[]);

static void do_user_modes(struct Client *client, const char *modes);
static void set_final_mode(struct Mode *, struct Mode *);
static void remove_our_modes(struct Channel *, struct Client *);
static void remove_a_mode(struct Channel *, struct Client *, int, char);

static char modebuf[MODEBUFLEN];
static char parabuf[MODEBUFLEN];
static char sendbuf[MODEBUFLEN];
static const char *para[MAXMODEPARAMS];
static char *mbuf;
static int pargs;

struct Message part_msgtab = {
  "PART", 0, 0, 2, 0, MFLG_SLOW, 0,
  { m_part, m_ignore }
};

struct Message ping_msgtab = {
  "PING", 0, 0, 1, 0, MFLG_SLOW, 0,
  { m_ping, m_ignore }
};

struct Message eob_msgtab = {
  "EOB", 0, 0, 0, 0, MFLG_SLOW, 0,
  { m_ignore, m_ignore }
};

struct Message server_msgtab = {
  "SERVER", 0, 0, 3, 0, MFLG_SLOW, 0,
  { m_server, m_ignore }
};

struct Message nick_msgtab = {
  "NICK", 0, 0, 1, 0, MFLG_SLOW, 0,
  { m_nick, m_ignore }
};

struct Message sjoin_msgtab = {
  "SJOIN", 0, 0, 4, 0, MFLG_SLOW, 0,
  { m_sjoin, m_ignore }
};

struct Message quit_msgtab = {
  "QUIT", 0, 0, 0, 0, MFLG_SLOW, 0,
  { m_quit, m_ignore }
};

struct Message squit_msgtab = {
  "SQUIT", 0, 0, 1, 0, MFLG_SLOW, 0,
  { m_squit, m_ignore }
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
  mod_add_cmd(&part_msgtab);
  mod_add_cmd(&quit_msgtab);
  mod_add_cmd(&squit_msgtab);
}

CLEANUP_MODULE
{
  uninstall_hook(connected_cb, irc_server_connected);
  mod_del_cmd(&ping_msgtab);
  mod_del_cmd(&server_msgtab);
  mod_del_cmd(&nick_msgtab);
  mod_del_cmd(&sjoin_msgtab);
  mod_del_cmd(&eob_msgtab);
  mod_del_cmd(&part_msgtab);
  mod_del_cmd(&quit_msgtab);
  mod_del_cmd(&squit_msgtab);
}

/** Introduce a new server; currently only useful for connect and jupes
 * @param
 * prefix prefix, usually me.name
 * name server to introduce
 * info Server Information string
 */
static void 
irc_sendmsg_server(struct Client *client, char *prefix, char *name, char *info) 
{
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

/* set_final_mode
 *
 * inputs - channel mode
 *    - old channel mode
 * output - NONE
 * side effects - walk through all the channel modes turning off modes
 *      that were on in oldmode but aren't on in mode.
 *      Then walk through turning on modes that are on in mode
 *      but were not set in oldmode.
 */

static const struct mode_letter
{
  unsigned int mode;
  unsigned char letter;
} flags[] = {
  { MODE_NOPRIVMSGS, 'n' },
  { MODE_TOPICLIMIT, 't' },
  { MODE_SECRET,     's' },
  { MODE_MODERATED,  'm' },
  { MODE_INVITEONLY, 'i' },
  { MODE_PARANOID,    'p' },
  { 0, '\0' }
};

static void
set_final_mode(struct Mode *mode, struct Mode *oldmode)
{
  char *pbuf = parabuf;
  int len;
  int i;

  *mbuf++ = '-';

  for (i = 0; flags[i].letter; i++)
  {
    if ((flags[i].mode & oldmode->mode) &&
        !(flags[i].mode & mode->mode))
      *mbuf++ = flags[i].letter;
  }

  if (oldmode->limit != 0 && mode->limit == 0)
    *mbuf++ = 'l';

  if (oldmode->key[0] && !mode->key[0])
  {
    *mbuf++ = 'k';
    len = ircsprintf(pbuf, "%s ", oldmode->key);
    pbuf += len;
    if ((flags[i].mode & mode->mode) &&
        !(flags[i].mode & oldmode->mode))
      *mbuf++ = flags[i].letter;
  }

  if (mode->limit != 0 && oldmode->limit != mode->limit)
  {
    *mbuf++ = 'l';
    len = ircsprintf(pbuf, "%d ", mode->limit);
    pbuf += len;
    pargs++;
  }

  if (mode->key[0] && strcmp(oldmode->key, mode->key))
  {
    *mbuf++ = 'k';
    len = ircsprintf(pbuf, "%s ", mode->key);
    pbuf += len;
    pargs++;
  }
  if (*(mbuf-1) == '+')
    *(mbuf-1) = '\0';
  else
    *mbuf = '\0';
}

/* remove_our_modes()
 *
 * inputs - pointer to channel to remove modes from
 *    - client pointer
 * output - NONE
 * side effects - Go through the local members, remove all their
 *      chanop modes etc., this side lost the TS.
 */
static void
remove_our_modes(struct Channel *chptr, struct Client *source)
{
  remove_a_mode(chptr, source, CHFL_CHANOP, 'o');
#ifdef HALFOPS
  remove_a_mode(chptr, source, CHFL_HALFOP, 'h');
#endif
  remove_a_mode(chptr, source, CHFL_VOICE, 'v');
}
/* remove_a_mode()
 *
 * inputs - pointer to channel
 *    - server or client removing the mode
 *    - mask o/h/v mask to be removed
 *    - flag o/h/v to be removed
 * output - NONE
 * side effects - remove ONE mode from all members of a channel
 */
static void
remove_a_mode(struct Channel *chptr, struct Client *source,
             int mask, char flag)
{
  dlink_node *ptr;
  struct Membership *ms;
  char lmodebuf[MODEBUFLEN];
  char *sp=sendbuf;
  const char *lpara[MAXMODEPARAMS];
  int count = 0;
  int i;
  int l;

  mbuf = lmodebuf;
  *mbuf++ = '-';
  *sp = '\0';

  DLINK_FOREACH(ptr, chptr->members.head)
  {
    ms = ptr->data;

    if ((ms->flags & mask) == 0)
      continue;

    ms->flags &= ~mask;

    lpara[count++] = ms->client_p->name;

    *mbuf++ = flag;

    if (count >= MAXMODEPARAMS)
    {
      for(i = 0; i < MAXMODEPARAMS; i++)
      {
        l = ircsprintf(sp, " %s", lpara[i]);
        sp += l;
      }
      *mbuf = '\0';
    }
  }
}

/* part_one_client()
 *
 * inputs - pointer to server
 *    - pointer to source client to remove
 *    - char pointer of name of channel to remove from
 * output - none
 * side effects - remove ONE client given the channel name
 */
static void
part_one_client(struct Client *client, struct Client *source, char *name)
{
  struct Channel *chptr = NULL;
  struct Membership *ms = NULL;

  if ((chptr = hash_find_channel(name)) == NULL)
  {
    printf("Trying to part %s from %s which doesnt exist\n", source->name,
        name);
    return;
  }

  if ((ms = find_channel_link(source, chptr)) == NULL)
  {
    printf("Trying to part %s from %s which they aren't on\n", source->name,
        chptr->chname);
    return;
  }

  remove_user_from_channel(ms);
}

static void 
m_ping(struct Client *client, struct Client *source, int parc, char *parv[])
{
  sendto_server(source, ":%s PONG %s :%s", me.name, me.name, source->name);
}

static void
m_server(struct Client *client, struct Client *source, int parc, char *parv[])
{
  if(IsConnecting(client))
  {
    sendto_server(client, "SVINFO 5 5 0: %lu", CurrentTime);
    sendto_server(client, ":%s PING :%s", me.name, me.name);
    SetServer(client);
    hash_add_client(client);
    printf("Completed server connection to %s\n", source->name);
    ClearConnecting(client);
    client->servptr = &me;
  }
  else
  {
    struct Client *newclient = make_client(client);

    strlcpy(newclient->name, parv[1], sizeof(newclient->name));
    strlcpy(newclient->info, parv[3], sizeof(newclient->info));
    newclient->hopcount = atoi(parv[2]);
    SetServer(newclient);
    dlinkAdd(newclient, &newclient->node, &global_client_list);
    hash_add_client(newclient);
    newclient->servptr = source;
    dlinkAdd(newclient, &newclient->lnode, &newclient->servptr->server_list);
    printf("Got server %s from hub %s\n", parv[1], source->name);
  }
}

static void
m_sjoin(struct Client *client, struct Client *source, int parc, char *parv[])
{
  struct Channel *chptr;
  struct Client  *target;
  time_t         newts;
  time_t         oldts;
  time_t         tstosend;
  struct Mode mode, *oldmode;
  int            args = 0;
  char           keep_our_modes = YES;
  char           keep_new_modes = YES;
  char           have_many_nicks = NO;
  int            lcount;
  char           nick_prefix[4];
  char           *np;
  int            len_nick = 0;
  int            isnew = 0;
  int            buflen = 0;
  int          slen;
  unsigned       int fl;
  char           *s;
  char     *sptr;
  char nick_buf[IRC_BUFSIZE]; /* buffer for modes and prefix */
  char           *nick_ptr; 
  char           *p; /* pointer used making sjbuf */

  if (IsClient(source) || parc < 5)
    return;

  /* SJOIN's for local channels can't happen. */
  if (*parv[2] != '#')
    return;

  modebuf[0] = '\0';
  mbuf = modebuf;
  pargs = 0;
  newts = atol(parv[1]);

  mode.mode = 0;
  mode.limit = 0;
  mode.key[0] = '\0';
  s = parv[3];

  while (*s)
  {
    switch (*(s++))
    {
      case 't':
        mode.mode |= MODE_TOPICLIMIT;
        break;
      case 'n':
        mode.mode |= MODE_NOPRIVMSGS;
        break;
      case 's':
        mode.mode |= MODE_SECRET;
        break;
      case 'm':
        mode.mode |= MODE_MODERATED;
        break;
      case 'i':
        mode.mode |= MODE_INVITEONLY;
        break;
      case 'p':
        mode.mode |= MODE_PARANOID;
        break;
      case 'k':
        strlcpy(mode.key, parv[4 + args], sizeof(mode.key));
        args++;
        if (parc < 5+args)
          return;
        break;
      case 'l':
        mode.limit = atoi(parv[4 + args]);
        args++;
        if (parc < 5+args)
          return;
        break;
    }
  }

  parabuf[0] = '\0';

  if ((chptr = hash_find_channel(parv[2])) == NULL)
  {
    isnew = 1;
    chptr = make_channel(parv[2]);
    printf("Created channel %s\n", parv[2]);
  }

  oldts   = chptr->channelts;
  oldmode = &chptr->mode;

  if (newts < 800000000)
  {

    newts = (oldts == 0) ? 0 : 800000000;
  }

  if (isnew)
    chptr->channelts = tstosend = newts;
  else if (newts == 0 || oldts == 0)
    chptr->channelts = tstosend = 0;
  else if (newts == oldts)
    tstosend = oldts;
  else if (newts < oldts)
  {
    keep_our_modes = NO;
    chptr->channelts = tstosend = newts;
  }
  else
  {
    keep_new_modes = NO;
    tstosend = oldts;
  }

  if (!keep_new_modes)
    mode = *oldmode;
  else if (keep_our_modes)
  {
    mode.mode |= oldmode->mode;
    if (oldmode->limit > mode.limit)
      mode.limit = oldmode->limit;
    if (strcmp(mode.key, oldmode->key) < 0)
      strcpy(mode.key, oldmode->key);
  }

  set_final_mode(&mode, oldmode);
  chptr->mode = mode;

  /* Lost the TS, other side wins, so remove modes on this side */
  if (!keep_our_modes)
  {
    remove_our_modes(chptr, source);
  }

  if (parv[3][0] != '0' && keep_new_modes)
  {
    channel_modes(chptr, source, modebuf, parabuf);
  }
  else
  {
    modebuf[0] = '0';
    modebuf[1] = '\0';
  }

  buflen = ircsprintf(nick_buf, ":%s SJOIN %lu %s %s %s:",
      source->name, (unsigned long)tstosend,
      chptr->chname, modebuf, parabuf);
  nick_ptr = nick_buf + buflen;

  /* check we can fit a nick on the end, as well as \r\n and a prefix "
   * @%+", and a space.
   */
  if (buflen >= (IRC_BUFSIZE - LIBIO_MAX(NICKLEN, IDLEN) - 2 - 3 - 1))
  {
    return;
  }

  mbuf = modebuf;
  sendbuf[0] = '\0';
  pargs = 0;

  *mbuf++ = '+';

  s = parv[args + 4];
  while (*s == ' ')
    s++;
  if ((p = strchr(s, ' ')) != NULL)
  {
    *p++ = '\0';
    while (*p == ' ')
      p++;
    have_many_nicks = *p;
  }

  while (*s)
  {
    int valid_mode = YES;
    fl = 0;

    do
    {
      switch (*s)
      {
        case '@':
          fl |= CHFL_CHANOP;
          s++;
          break;
#ifdef HALFOPS
        case '%':
          fl |= CHFL_HALFOP;
          s++;
          break;
#endif
        case '+':
          fl |= CHFL_VOICE;
          s++;
          break;
        default:
          valid_mode = NO;
          break;
      }
    } while (valid_mode);

    target = find_chasing(source, s, NULL);

    /*
     * if the client doesnt exist, or if its fake direction/server, skip.
     * we cannot send ERR_NOSUCHNICK here because if its a UID, we cannot
     * lookup the nick, and its better to never send the numeric than only
     * sometimes.
     */
    if (target == NULL ||
        target->from != client ||
        !IsClient(target))
    {
      goto nextnick;
    }

    len_nick = strlen(target->name);

    np = nick_prefix;

    if (keep_new_modes)
    {
      if (fl & CHFL_CHANOP)
      {
        *np++ = '@';
        len_nick++;
      }
#ifdef HALFOPS
      if (fl & CHFL_HALFOP)
      {
        *np++ = '%';
        len_nick++;
      }
#endif
      if (fl & CHFL_VOICE)
      {
        *np++ = '+';
        len_nick++;
      }
    }
    else
    {
      if (fl & (CHFL_CHANOP|CHFL_HALFOP))
        fl = CHFL_DEOPPED;
      else
        fl = 0;
    }

    *np = '\0';

    if ((nick_ptr - nick_buf + len_nick) > (IRC_BUFSIZE  - 2))
    {
      sendto_server(client, "%s", nick_buf);

      buflen = ircsprintf(nick_buf, ":%s SJOIN %lu %s %s %s:",
          source->name, (unsigned long)tstosend,
          chptr->chname, modebuf, parabuf);
      nick_ptr = nick_buf + buflen;
    }

    nick_ptr += ircsprintf(nick_ptr, "%s%s ", nick_prefix, target->name);

    if (!IsMember(target, chptr))
    {
      add_user_to_channel(chptr, target, fl, !have_many_nicks);
      printf("Added %s!%s@%s to %s\n", target->name, target->username,
          target->host, chptr->chname);
    }

    if (fl & CHFL_CHANOP)
    {
      *mbuf++ = 'o';
      para[pargs++] = target->name;

      if (pargs >= MAXMODEPARAMS)
      {
        /*
         * Ok, the code is now going to "walk" through
         * sendbuf, filling in para strings. So, I will use sptr
         * to point into the sendbuf.
         * Notice, that ircsprintf() returns the number of chars
         * successfully inserted into string.
         * - Dianora
         */

        sptr = sendbuf;
        *mbuf = '\0';
        for(lcount = 0; lcount < MAXMODEPARAMS; lcount++)
        {
          slen = ircsprintf(sptr, " %s", para[lcount]); /* see? */
          sptr += slen;         /* ready for next */
        }
        mbuf = modebuf;
        *mbuf++ = '+';

        sendbuf[0] = '\0';
        pargs = 0;
      }
    }
#ifdef HALFOPS
    if (fl & CHFL_HALFOP)
    {
      *mbuf++ = 'h';
      para[pargs++] = target->name;

      if (pargs >= MAXMODEPARAMS)
      {
        sptr = sendbuf;
        *mbuf = '\0';
        for(lcount = 0; lcount < MAXMODEPARAMS; lcount++)
        {
          slen = ircsprintf(sptr, " %s", para[lcount]);
          sptr += slen;
        }

        mbuf = modebuf;
        *mbuf++ = '+';

        sendbuf[0] = '\0';
        pargs = 0;
      }
    }
#endif
    if (fl & CHFL_VOICE)
    {
      *mbuf++ = 'v';
      para[pargs++] = target->name;

      if (pargs >= MAXMODEPARAMS)
      {
        sptr = sendbuf;
        *mbuf = '\0';
        for (lcount = 0; lcount < MAXMODEPARAMS; lcount++)
        {
          slen = ircsprintf(sptr, " %s", para[lcount]);
          sptr += slen;
        }

        mbuf = modebuf;
        *mbuf++ = '+';

        sendbuf[0] = '\0';
        pargs = 0;
      }
    }

nextnick:
    if ((s = p) == NULL)
      break;
    while (*s == ' ')
      s++;
    if ((p = strchr(s, ' ')) != NULL)
    {
      *p++ = 0;
      while (*p == ' ')
        p++;
    }
  }

  *mbuf = '\0';
  *(nick_ptr - 1) = '\0';

  /*
   * checking for lcount < MAXMODEPARAMS at this time is wrong
   * since the code has already verified above that pargs < MAXMODEPARAMS
   * checking for para[lcount] != '\0' is also wrong, since
   * there is no place where para[lcount] is set!
   * - Dianora
   */

  if (pargs != 0)
  {
    sptr = sendbuf;

    for (lcount = 0; lcount < pargs; lcount++)
    {
      slen = ircsprintf(sptr, " %s", para[lcount]);
      sptr += slen;
    }

  }

  /* If this happens, its the result of a malformed SJOIN
   * a remnant from the old persistent channel code. *sigh*
   * Or it could be the result of a client just leaving
   * and leaving us with a channel formed just as the client parts.
   * - Dianora
   */

  if ((dlink_list_length(&chptr->members) == 0) && isnew)
  {
    destroy_channel(chptr);
    return;
  }

  if (parv[4 + args][0] == '\0')
    return;
}

static void
m_nick(struct Client *source, struct Client *client, int parc, char *parv[])
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
    newclient->servptr = find_server(source->name);
    dlinkAdd(newclient, &newclient->lnode, &newclient->servptr->client_list);

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
m_part(struct Client *client, struct Client *source, int parc, char *parv[])
{
  char *p, *name;

  name = strtoken(&p, parv[1], ",");
    
  while (name)
  {
    part_one_client(client, source, name);
    name = strtoken(&p, NULL, ",");
  }
}

static void
m_quit(struct Client *client, struct Client *source, int parc, char *parv[])
{
  char *comment = (parc > 1 && parv[1]) ? parv[1] : client->name;

  if (strlen(comment) > (size_t)KICKLEN)
    comment[KICKLEN] = '\0';

  exit_client(source, source, comment);
}

static void
m_squit(struct Client *client, struct Client *source, int parc, char *parv[])
{
  struct Client *target = NULL;
  char *comment;
  const char *server;
  char def_reason[] = "No reason";

  server = parv[1];

  if ((target = find_server(server)) == NULL)
    return;

  if (!IsServer(target) || IsMe(target))
    return;

  comment = (parc > 2 && parv[2]) ? parv[2] : def_reason;

  if (strlen(comment) > (size_t)REASONLEN)
    comment[REASONLEN] = '\0';

  exit_client(target, source, comment);
}
