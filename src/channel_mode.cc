/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  channel_mode.c: Channel mode functions
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

/* 10 is a magic number in hybrid 6 NFI where it comes from -db */
#define BAN_FUDGE	10

static char nuh_mask[IRCD_MAXPARA][IRC_BUFSIZE];
/* some buffers for rebuilding channel/nick lists with ,'s */
static struct ChModeChange mode_changes[IRC_BUFSIZE];
static int mode_count;
static int mode_limit;		/* number of modes set other than simple */
static int simple_modes_mask;	/* bit mask of simple modes already set */
extern BlockHeap *ban_heap;
struct Callback *channel_access_cb = NULL;


/* check_string()
 *
 * inputs       - string to check
 * output       - pointer to modified string
 * side effects - Fixes a string so that the first white space found
 *                becomes an end of string marker (`\0`).
 *                returns the 'fixed' string or "*" if the string
 *                was NULL length or a NULL pointer.
 */
static char *
check_string(char *s)
{
  char *str = s;
  static char star[] = "*";

  if (EmptyString(s))
    return star;

  for (; *s; ++s)
  {
    if (IsSpace(*s))
    {
      *s = '\0';
      break;
    }
  }

  return str;
}

/*
 * Ban functions to work with mode +b/e/d/I
 */
/* add the specified ID to the channel.. 
 *   -is 8/9/00 
 */

int
add_id(struct Client *client_p, struct Channel *chptr, char *banid, int type)
{
  dlink_list *list = NULL;
  dlink_node *ban = NULL;
  size_t len = 0;
  struct Ban *ban_p = NULL;
  char name[NICKLEN];
  char user[USERLEN + 1];
  char host[HOSTLEN + 1];
  struct split_nuh_item nuh;

  nuh.nuhmask  = check_string(banid);
  nuh.nickptr  = name;
  nuh.userptr  = user;
  nuh.hostptr  = host;

  nuh.nicksize = sizeof(name);
  nuh.usersize = sizeof(user);
  nuh.hostsize = sizeof(host);

  split_nuh(&nuh);

  /*
   * Re-assemble a new n!u@h and print it back to banid for sending
   * the mode to the channel.
   */
  len = ircsprintf(banid, "%s!%s@%s", name, user, host);

  switch (type)
  {
    case CHFL_BAN:
      list = &chptr->banlist;
      clear_ban_cache(chptr);
      break;
    case CHFL_EXCEPTION:
      list = &chptr->exceptlist;
      clear_ban_cache(chptr);
      break;
    case CHFL_INVEX:
      list = &chptr->invexlist;
      break;
    default:
      assert(0);
      return 0;
  }

  DLINK_FOREACH(ban, list->head)
  {
    ban_p = (struct Ban *)ban->data;
    if (!irccmp(ban_p->name, name) &&
	!irccmp(ban_p->username, user) &&
	!irccmp(ban_p->host, host))
    {
      return 0;
    }
  }

  ban_p = (struct Ban *)BlockHeapAlloc(ban_heap);

  DupString(ban_p->name, name);
  DupString(ban_p->username, user);
  DupString(ban_p->host, host);

  ban_p->when = CurrentTime;
  ban_p->len = len - 2; /* -2 for @ and ! */
  ban_p->type = parse_netmask(host, &ban_p->addr, &ban_p->bits);

  DupString(ban_p->who, client_p->c_nuh());

  dlinkAdd(ban_p, &ban_p->node, list);

  return 1;
}

/*
 * inputs	- pointer to channel
 *		- pointer to ban id
 *		- type of ban, i.e. ban, exception, invex
 * output	- 0 for failure, 1 for success
 * side effects	-
 */
int
del_id(struct Channel *chptr, char *banid, int type)
{
  dlink_list *list = NULL;
  dlink_node *ban = NULL;
  char name[NICKLEN];
  char user[USERLEN + 1];
  char host[HOSTLEN + 1];
  struct split_nuh_item nuh;

  assert(banid != NULL);

  nuh.nuhmask  = check_string(banid);
  nuh.nickptr  = name;
  nuh.userptr  = user;
  nuh.hostptr  = host;

  nuh.nicksize = sizeof(name);
  nuh.usersize = sizeof(user);
  nuh.hostsize = sizeof(host);

  split_nuh(&nuh);

  /*
   * Re-assemble a new n!u@h and print it back to banid for sending
   * the mode to the channel.
   */
  ircsprintf(banid, "%s!%s@%s", name, user, host);

  switch (type)
  {
    case CHFL_BAN:
      list = &chptr->banlist;
      clear_ban_cache(chptr);
      break;
    case CHFL_EXCEPTION:
      list = &chptr->exceptlist;
      clear_ban_cache(chptr);
      break;
    case CHFL_INVEX:
      list = &chptr->invexlist;
      break;
    default:
      assert(0);
      return 0;
  }

  DLINK_FOREACH(ban, list->head)
  {
    struct Ban *banptr = (struct Ban *)ban->data;

    if (!irccmp(name, banptr->name) &&
	!irccmp(user, banptr->username) &&
	!irccmp(host, banptr->host))
    {
      remove_ban(banptr, list);
      return 1;
    }
  }

  return 0;
}

static const struct mode_letter
{
  const unsigned int mode;
  const unsigned char letter;
} flags[] = {
  { MODE_INVITEONLY, 'i' },
  { MODE_MODERATED,  'm' },
  { MODE_NOPRIVMSGS, 'n' },
  { MODE_PARANOID,   'p' },
  { MODE_SECRET,     's' },
  { MODE_TOPICLIMIT, 't' },
  { 0, '\0' }
};

/* channel_modes()
 *
 * inputs       - pointer to channel
 *              - pointer to client
 *              - pointer to mode buf
 *              - pointer to parameter buf
 * output       - NONE
 * side effects - write the "simple" list of channel modes for channel
 * chptr onto buffer mbuf with the parameters in pbuf.
 */
void
channel_modes(struct Channel *chptr, struct Client *client_p,
              char *mbuf, char *pbuf)
{
  int i;

  *mbuf++ = '+';
  *pbuf = '\0';

  for (i = 0; flags[i].mode; ++i)
    if (chptr->mode.mode & flags[i].mode)
      *mbuf++ = flags[i].letter;

  if (chptr->mode.limit)
  {
    *mbuf++ = 'l';

    if (IsMember(client_p, chptr) /*|| IsServer(client_p)*/)
      pbuf += ircsprintf(pbuf, "%d ", chptr->mode.limit);
  }

  if (chptr->mode.key[0])
  {
    *mbuf++ = 'k';

    if (*pbuf || IsMember(client_p, chptr) /*|| IsServer(client_p)*/)
      ircsprintf(pbuf, "%s ", chptr->mode.key);
  }

  *mbuf = '\0';
}

#if 0

XXX This isnt used yet
/* fix_key()
 * 
 * inputs       - pointer to key to clean up
 * output       - pointer to cleaned up key
 * side effects - input string is modified
 *
 * stolen from Undernet's ircd  -orabidoo
 */
static char *
fix_key(char *arg)
{
  unsigned char *s, *t, c;

  for (s = t = (unsigned char *)arg; (c = *s); s++)
  {
    c &= 0x7f;
    if (c != ':' && c > ' ' && c != ',')
      *t++ = c;
  }

  *t = '\0';
  return arg;
}
#endif

/* fix_key_old()
 * 
 * inputs       - pointer to key to clean up
 * output       - pointer to cleaned up key
 * side effects - input string is modifed 
 *
 * Here we attempt to be compatible with older non-hybrid servers.
 * We can't back down from the ':' issue however.  --Rodder
 */
static char *
fix_key_old(char *arg)
{
  unsigned char *s, *t, c;

  for (s = t = (unsigned char *)arg; (c = *s); s++)
  {
    c &= 0x7f;
    if ((c != 0x0a) && (c != ':') && (c != 0x0d) && (c != ','))
      *t++ = c;
  }

  *t = '\0';
  return arg;
}

/* bitmasks for various error returns that set_channel_mode should only return
 * once per call  -orabidoo
 */

#define SM_ERR_NOTS         0x00000001 /* No TS on channel  */
#define SM_ERR_NOOPS        0x00000002 /* No chan ops       */
#define SM_ERR_UNKNOWN      0x00000004
#define SM_ERR_RPL_B        0x00000008
#define SM_ERR_RPL_E        0x00000010
#define SM_ERR_NOTONCHANNEL 0x00000020 /* Not on channel    */
#define SM_ERR_RPL_I        0x00000040

/* Now lets do some stuff to keep track of what combinations of
 * servers exist...
 * Note that the number of combinations doubles each time you add
 * something to this list. Each one is only quick if no servers use that
 * combination, but if the numbers get too high here MODE will get too
 * slow. I suggest if you get more than 7 here, you consider getting rid
 * of some and merging or something. If it wasn't for irc+cs we would
 * probably not even need to bother about most of these, but unfortunately
 * we do. -A1kmm
 */

/* Mode functions handle mode changes for a particular mode... */
static void
chm_nosuch(struct Client *client_p, struct Client *source_p,
           struct Channel *chptr, int parc, int *parn,
           char **parv, int *errors, int alev, int dir, char c, void *d,
           const char *chname)
{
  if (*errors & SM_ERR_UNKNOWN)
    return;

  *errors |= SM_ERR_UNKNOWN;
  ilog(L_DEBUG, "Unknown mode %s %s %c", me->c_nuh(), source_p->c_nuh(), c);
}

static void
chm_simple(struct Client *client_p, struct Client *source_p, struct Channel *chptr,
           int parc, int *parn, char **parv, int *errors, int alev, int dir,
           char c, void *d, const char *chname)
{
  long mode_type;

  mode_type = (long)d;

  /* If have already dealt with this simple mode, ignore it */
  if (simple_modes_mask & mode_type)
    return;

  simple_modes_mask |= mode_type;

  /* setting + */
  /* Apparently, (though no one has ever told the hybrid group directly) 
   * admins don't like redundant mode checking. ok. It would have been nice 
   * if you had have told us directly. I've left the original code snippets 
   * in place. 
   * 
   * -Dianora 
   */ 
  if ((dir == MODE_ADD)) /* && !(chptr->mode.mode & mode_type)) */
  {
    chptr->mode.mode |= mode_type;

    mode_changes[mode_count].letter = c;
    mode_changes[mode_count].dir = MODE_ADD;
    mode_changes[mode_count].caps = 0;
    mode_changes[mode_count].nocaps = 0;
    mode_changes[mode_count].id = NULL;
    mode_changes[mode_count].mems = ALL_MEMBERS;
    mode_changes[mode_count++].arg = NULL;
  }
  else if ((dir == MODE_DEL)) /* && (chptr->mode.mode & mode_type)) */
  {
    /* setting - */

    chptr->mode.mode &= ~mode_type;

    mode_changes[mode_count].letter = c;
    mode_changes[mode_count].dir = MODE_DEL;
    mode_changes[mode_count].caps = 0;
    mode_changes[mode_count].nocaps = 0;
    mode_changes[mode_count].mems = ALL_MEMBERS;
    mode_changes[mode_count].id = NULL;
    mode_changes[mode_count++].arg = NULL;
  }
}

static void
chm_ban(struct Client *client_p, struct Client *source_p,
        struct Channel *chptr, int parc, int *parn,
        char **parv, int *errors, int alev, int dir, char c, void *d,
        const char *chname)
{
  char *mask = NULL;

  mask = nuh_mask[*parn];
  memcpy(mask, parv[*parn], sizeof(nuh_mask[*parn]));
  ++*parn;

  if (strchr(mask, ' '))
    return;

  switch (dir)
  {
    case MODE_ADD:
      if (!add_id(source_p, chptr, mask, CHFL_BAN))
        return;
      break;
    case MODE_DEL:
/* XXX grrrrrrr */
#ifdef NO_BAN_COOKIE
      if (!del_id(chptr, mask, CHFL_BAN))
        return;
#else
     /* XXX this hack allows /mode * +o-b nick ban.cookie
      * I'd like to see this hack go away in the future.
      */
      del_id(chptr, mask, CHFL_BAN);
#endif
      break;
    default:
      assert(0);
  }

  mode_changes[mode_count].letter = c;
  mode_changes[mode_count].dir = dir;
  mode_changes[mode_count].caps = 0;
  mode_changes[mode_count].nocaps = 0;
  mode_changes[mode_count].mems = ALL_MEMBERS;
  mode_changes[mode_count].id = NULL;
  mode_changes[mode_count++].arg = mask;
}

static void
chm_except(struct Client *client_p, struct Client *source_p,
           struct Channel *chptr, int parc, int *parn,
           char **parv, int *errors, int alev, int dir, char c, void *d,
           const char *chname)
{
  char *mask = NULL;

  mask = nuh_mask[*parn];
  memcpy(mask, parv[*parn], sizeof(nuh_mask[*parn]));
  ++*parn;

  if (strchr(mask, ' '))
    return;

  switch (dir)
  {
    case MODE_ADD:
      if (!add_id(source_p, chptr, mask, CHFL_EXCEPTION))
        return;
      break;
    case MODE_DEL:
      if (!del_id(chptr, mask, CHFL_EXCEPTION))
        return;
      break;
    default:
      assert(0);
  }

  mode_changes[mode_count].letter = c;
  mode_changes[mode_count].dir = dir;
  mode_changes[mode_count].nocaps = 0;

  mode_changes[mode_count].mems = ONLY_CHANOPS;

  mode_changes[mode_count].id = NULL;
  mode_changes[mode_count++].arg = mask;
}

static void
chm_invex(struct Client *client_p, struct Client *source_p,
          struct Channel *chptr, int parc, int *parn,
          char **parv, int *errors, int alev, int dir, char c, void *d,
          const char *chname)
{
  char *mask = NULL;

  if (alev < CHACCESS_HALFOP)
  {
    if (!(*errors & SM_ERR_NOOPS))
      ilog(L_DEBUG, "Invex from non chop");
    *errors |= SM_ERR_NOOPS;
    return;
  }

  mask = nuh_mask[*parn];
  memcpy(mask, parv[*parn], sizeof(nuh_mask[*parn]));
  ++*parn;

  if (strchr(mask, ' '))
    return;

  switch (dir)
  {
    case MODE_ADD:
      if (!add_id(source_p, chptr, mask, CHFL_INVEX))
        return;
      break;
    case MODE_DEL:
      if (!del_id(chptr, mask, CHFL_INVEX))
        return;
      break;
    default:
      assert(0);
  }

  mode_changes[mode_count].letter = c;
  mode_changes[mode_count].dir = dir;

  mode_changes[mode_count].mems = ONLY_CHANOPS;

  mode_changes[mode_count].id = NULL;
  mode_changes[mode_count++].arg = mask;
}

/*
 * inputs	- pointer to channel
 * output	- none
 * side effects	- clear ban cache
 */
void
clear_ban_cache(struct Channel *chptr)
{
  dlink_node *ptr = NULL;

  DLINK_FOREACH(ptr, chptr->members.head)
  {
    struct Membership *ms = (struct Membership *)ptr->data;
    ms->flags &= ~(CHFL_BAN_SILENCED|CHFL_BAN_CHECKED);
  }
}

static void
chm_op(struct Client *client_p, struct Client *source_p,
       struct Channel *chptr, int parc, int *parn,
       char **parv, int *errors, int alev, int dir, char c, void *d,
       const char *chname)
{
  char *opnick;
  struct Client *targ_p;
  struct Membership *member;
  int caps = 0;

  if ((dir == MODE_QUERY) || (parc <= *parn))
    return;

  opnick = parv[(*parn)++];

  if ((targ_p = find_chasing(source_p, opnick, NULL)) == NULL)
    return;

  if ((member = find_channel_link(targ_p, chptr)) == NULL)
  {
    *errors |= SM_ERR_NOTONCHANNEL;
    return;
  }

  /* no redundant mode changes */
  if (dir == MODE_ADD &&  has_member_flags(member, CHFL_CHANOP))
    return;
  if (dir == MODE_DEL && !has_member_flags(member, CHFL_CHANOP))
  {
#ifdef HALFOPS
    if (has_member_flags(member, CHFL_HALFOP))
      chm_hop(client_p, source_p, chptr, parc, parn, parv, errors, alev,
              dir, c, d, chname);
#endif
    return;
  }

#ifdef HALFOPS
  if (dir == MODE_ADD && has_member_flags(member, CHFL_HALFOP))
  {
    /* promoting from % to @ is visible only to CAP_HOPS servers */
    mode_changes[mode_count].letter = 'h';
    mode_changes[mode_count].dir = MODE_DEL;
    mode_changes[mode_count].caps = caps = CAP_HOPS;
    mode_changes[mode_count].nocaps = 0;
    mode_changes[mode_count].mems = ALL_MEMBERS;
    mode_changes[mode_count].id = NULL;
    mode_changes[mode_count].arg = targ_p->name;
    mode_changes[mode_count++].client = targ_p;
  }
#endif

  mode_changes[mode_count].letter = 'o';
  mode_changes[mode_count].dir = dir;
  mode_changes[mode_count].caps = caps;
  mode_changes[mode_count].nocaps = 0;
  mode_changes[mode_count].mems = ALL_MEMBERS;
  mode_changes[mode_count].id = targ_p->c_id();
  mode_changes[mode_count].arg = targ_p->c_name();
  mode_changes[mode_count++].client = targ_p;

  if (dir == MODE_ADD)
  {
    AddMemberFlag(member, CHFL_CHANOP);
    DelMemberFlag(member, CHFL_DEOPPED | CHFL_HALFOP);
  }
  else
    DelMemberFlag(member, CHFL_CHANOP);
}

#ifdef HALFOPS
static void
chm_hop(struct Client *client_p, struct Client *source_p,
       struct Channel *chptr, int parc, int *parn,
       char **parv, int *errors, int alev, int dir, char c, void *d,
       const char *chname)
{
  char *opnick;
  struct Client *targ_p;
  struct Membership *member;

  /* *sigh* - dont allow halfops to set +/-h, they could fully control a
   * channel if there were no ops - it doesnt solve anything.. MODE_PARANOID
   * when used with MODE_SECRET is paranoid - cant use +p
   *
   * it needs to be optional per channel - but not via +p, that or remove
   * paranoid.. -- fl_
   *
   * +p means paranoid, it is useless for anything else on modern IRC, as
   * list isn't really usable. If you want to have a private channel these
   * days, you set it +s. Halfops can no longer remove simple modes when
   * +p is set (although they can set +p) so it is safe to use this to
   * control whether they can (de)halfop...
   */
  if (alev <
      ((chptr->mode.mode & MODE_PARANOID) ? CHACCESS_CHANOP : CHACCESS_HALFOP))
  {
    if (!(*errors & SM_ERR_NOOPS))
    *errors |= SM_ERR_NOOPS;
    return;
  }

  if ((dir == MODE_QUERY) || (parc <= *parn))
    return;

  opnick = parv[(*parn)++];

  if ((targ_p = find_chasing(source_p, opnick, NULL)) == NULL)
    return;
  assert(IsClient(targ_p));

  if ((member = find_channel_link(targ_p, chptr)) == NULL)
  {
    *errors |= SM_ERR_NOTONCHANNEL;
    return;
  }

  /* no redundant mode changes */
  if (dir == MODE_ADD &&  has_member_flags(member, CHFL_HALFOP | CHFL_CHANOP))
    return;
  if (dir == MODE_DEL && !has_member_flags(member, CHFL_HALFOP))
    return;

  mode_changes[mode_count].letter = 'h';
  mode_changes[mode_count].dir = dir;
  mode_changes[mode_count].caps = CAP_HOPS;
  mode_changes[mode_count].nocaps = 0;
  mode_changes[mode_count].mems = ALL_MEMBERS;
  mode_changes[mode_count].id = targ_p->id;
  mode_changes[mode_count].arg = targ_p->name;
  mode_changes[mode_count++].client = targ_p;

  mode_changes[mode_count].letter = 'o';
  mode_changes[mode_count].dir = dir;
  mode_changes[mode_count].caps = 0;
  mode_changes[mode_count].nocaps = CAP_HOPS;
  mode_changes[mode_count].mems = ONLY_SERVERS;
  mode_changes[mode_count].id = targ_p->id;
  mode_changes[mode_count].arg = targ_p->name;
  mode_changes[mode_count++].client = targ_p;

  if (dir == MODE_ADD)
  {
    AddMemberFlag(member, CHFL_HALFOP);
    DelMemberFlag(member, CHFL_DEOPPED);
  }
  else
    DelMemberFlag(member, CHFL_HALFOP);
}
#endif

static void
chm_voice(struct Client *client_p, struct Client *source_p,
          struct Channel *chptr, int parc, int *parn,
          char **parv, int *errors, int alev, int dir, char c, void *d,
          const char *chname)
{
  char *opnick;
  struct Client *targ_p;
  struct Membership *member;

  if (alev < CHACCESS_HALFOP)
  {
    *errors |= SM_ERR_NOOPS;
    return;
  }

  if ((dir == MODE_QUERY) || parc <= *parn)
    return;

  opnick = parv[(*parn)++];

  if ((targ_p = find_chasing(source_p, opnick, NULL)) == NULL)
    return;

  if ((member = find_channel_link(targ_p, chptr)) == NULL)
  {
    *errors |= SM_ERR_NOTONCHANNEL;
    return;
  }

  /* no redundant mode changes */
  if (dir == MODE_ADD &&  has_member_flags(member, CHFL_VOICE))
    return;
  if (dir == MODE_DEL && !has_member_flags(member, CHFL_VOICE))
    return;

  mode_changes[mode_count].letter = 'v';
  mode_changes[mode_count].dir = dir;
  mode_changes[mode_count].caps = 0;
  mode_changes[mode_count].nocaps = 0;
  mode_changes[mode_count].mems = ALL_MEMBERS;
  mode_changes[mode_count].id = targ_p->c_id();
  mode_changes[mode_count].arg = targ_p->c_name();
  mode_changes[mode_count++].client = targ_p;

  if (dir == MODE_ADD)
    AddMemberFlag(member, CHFL_VOICE);
  else
    DelMemberFlag(member, CHFL_VOICE);
}

static void
chm_limit(struct Client *client_p, struct Client *source_p,
          struct Channel *chptr, int parc, int *parn,
          char **parv, int *errors, int alev, int dir, char c, void *d,
          const char *chname)
{
  int i, limit;
  char *lstr;

  if (alev < CHACCESS_HALFOP)
  {
    *errors |= SM_ERR_NOOPS;
    return;
  }

  if (dir == MODE_QUERY)
    return;

  if ((dir == MODE_ADD) && parc > *parn)
  {
    lstr = parv[(*parn)++];

    if ((limit = atoi(lstr)) <= 0)
      return;

    ircsprintf(lstr, "%d", limit);

    /* if somebody sets MODE #channel +ll 1 2, accept latter --fl */
    for (i = 0; i < mode_count; i++)
    {
      if (mode_changes[i].letter == c && mode_changes[i].dir == MODE_ADD)
        mode_changes[i].letter = 0;
    }

    mode_changes[mode_count].letter = c;
    mode_changes[mode_count].dir = MODE_ADD;
    mode_changes[mode_count].caps = 0;
    mode_changes[mode_count].nocaps = 0;
    mode_changes[mode_count].mems = ALL_MEMBERS;
    mode_changes[mode_count].id = NULL;
    mode_changes[mode_count++].arg = lstr;

    chptr->mode.limit = limit;
  }
  else if (dir == MODE_DEL)
  {
    if (!chptr->mode.limit)
      return;

    chptr->mode.limit = 0;

    mode_changes[mode_count].letter = c;
    mode_changes[mode_count].dir = MODE_DEL;
    mode_changes[mode_count].caps = 0;
    mode_changes[mode_count].nocaps = 0;
    mode_changes[mode_count].mems = ALL_MEMBERS;
    mode_changes[mode_count].id = NULL;
    mode_changes[mode_count++].arg = NULL;
  }
}

static void
chm_key(struct Client *client_p, struct Client *source_p,
        struct Channel *chptr, int parc, int *parn,
        char **parv, int *errors, int alev, int dir, char c, void *d,
        const char *chname)
{
  int i;
  char *key;

  if (alev < CHACCESS_HALFOP)
  {
    *errors |= SM_ERR_NOOPS;
    return;
  }

  if (dir == MODE_QUERY)
    return;

  if ((dir == MODE_ADD) && parc > *parn)
  {
    key = parv[(*parn)++];

    fix_key_old(key);

    if (*key == '\0')
      return;

    assert(key[0] != ' ');
    strlcpy(chptr->mode.key, key, sizeof(chptr->mode.key));

    /* if somebody does MODE #channel +kk a b, accept latter --fl */
    for (i = 0; i < mode_count; i++)
    {
      if (mode_changes[i].letter == c && mode_changes[i].dir == MODE_ADD)
        mode_changes[i].letter = 0;
    }

    mode_changes[mode_count].letter = c;
    mode_changes[mode_count].dir = MODE_ADD;
    mode_changes[mode_count].caps = 0;
    mode_changes[mode_count].nocaps = 0;
    mode_changes[mode_count].mems = ALL_MEMBERS;
    mode_changes[mode_count].id = NULL;
    mode_changes[mode_count++].arg = chptr->mode.key;
  }
  else if (dir == MODE_DEL)
  {
    if (parc > *parn)
      (*parn)++;

    if (chptr->mode.key[0] == '\0')
      return;

    chptr->mode.key[0] = '\0';

    mode_changes[mode_count].letter = c;
    mode_changes[mode_count].dir = MODE_DEL;
    mode_changes[mode_count].caps = 0;
    mode_changes[mode_count].nocaps = 0;
    mode_changes[mode_count].mems = ALL_MEMBERS;
    mode_changes[mode_count].id = NULL;
    mode_changes[mode_count++].arg = "*";
  }
}

struct ChannelMode
{
  void (*func) (struct Client *client_p, struct Client *source_p,
                struct Channel *chptr, int parc, int *parn, char **parv,
                int *errors, int alev, int dir, char c, void *d,
                const char *chname);
  void *d;
};

static struct ChannelMode ModeTable[255] =
{
  {chm_nosuch, NULL},
  {chm_nosuch, NULL},                             /* A */
  {chm_nosuch, NULL},                             /* B */
  {chm_nosuch, NULL},                             /* C */
  {chm_nosuch, NULL},                             /* D */
  {chm_nosuch, NULL},                             /* E */
  {chm_nosuch, NULL},                             /* F */
  {chm_nosuch, NULL},                             /* G */
  {chm_nosuch, NULL},                             /* H */
  {chm_invex, NULL},                              /* I */
  {chm_nosuch, NULL},                             /* J */
  {chm_nosuch, NULL},                             /* K */
  {chm_nosuch, NULL},                             /* L */
  {chm_nosuch, NULL},                             /* M */
  {chm_nosuch, NULL},                             /* N */
  {chm_nosuch, NULL},                             /* O */
  {chm_nosuch, NULL},                             /* P */
  {chm_nosuch, NULL},                             /* Q */
  {chm_nosuch, NULL},                             /* R */
  {chm_nosuch, NULL},                             /* S */
  {chm_nosuch, NULL},                             /* T */
  {chm_nosuch, NULL},                             /* U */
  {chm_nosuch, NULL},                             /* V */
  {chm_nosuch, NULL},                             /* W */
  {chm_nosuch, NULL},                             /* X */
  {chm_nosuch, NULL},                             /* Y */
  {chm_nosuch, NULL},                             /* Z */
  {chm_nosuch, NULL},
  {chm_nosuch, NULL},
  {chm_nosuch, NULL},
  {chm_nosuch, NULL},
  {chm_nosuch, NULL},
  {chm_nosuch, NULL},
  {chm_nosuch, NULL},				  /* a */
  {chm_ban, NULL},                                /* b */
  {chm_nosuch, NULL},                             /* c */
  {chm_nosuch, NULL},                             /* d */
  {chm_except, NULL},                             /* e */
  {chm_nosuch, NULL},                             /* f */
  {chm_nosuch, NULL},                             /* g */
#ifdef HALFOPS
  {chm_hop, NULL},                                /* h */
#else
  {chm_nosuch, NULL},				  /* h */
#endif
  {chm_simple, (void *) MODE_INVITEONLY},         /* i */
  {chm_nosuch, NULL},                             /* j */
  {chm_key, NULL},                                /* k */
  {chm_limit, NULL},                              /* l */
  {chm_simple, (void *) MODE_MODERATED},          /* m */
  {chm_simple, (void *) MODE_NOPRIVMSGS},         /* n */
  {chm_op, NULL},                                 /* o */
  {chm_simple, (void *) MODE_PARANOID},            /* p */
  {chm_nosuch, NULL},                             /* q */
  {chm_nosuch, NULL},                             /* r */
  {chm_simple, (void *) MODE_SECRET},             /* s */
  {chm_simple, (void *) MODE_TOPICLIMIT},         /* t */
  {chm_nosuch, NULL},                             /* u */
  {chm_voice, NULL},                              /* v */
  {chm_nosuch, NULL},                             /* w */
  {chm_nosuch, NULL},                             /* x */
  {chm_nosuch, NULL},                             /* y */
  {chm_nosuch, NULL},                             /* z */
};

/* get_channel_access()
 *
 * inputs       - pointer to Client struct
 *              - pointer to Membership struct
 * output       - CHACCESS_CHANOP if we should let them have
 *                chanop level access, 0 for peon level access.
 * side effects - NONE
 */
static void *
get_channel_access(va_list args)
{
  struct Client *source_p = va_arg(args, struct Client *);
  struct Membership *member = va_arg(args, struct Membership *);
  int *level = va_arg(args, int *);

  /* Use these to avoid warnings */
  if(source_p == NULL)
    return NULL;

  if(member == NULL)
    return NULL;

  /* Let hacked servers in for now... */
  *level = CHACCESS_CHANOP;
  return NULL;
}

void
init_channel_modes(void)
{
  channel_access_cb = register_callback("get_channel_access",
                                        get_channel_access);
}

/* void set_channel_mode(struct Client *client_p, struct Client *source_p,
 *               struct Channel *chptr, int parc, char **parv,
 *               char *chname)
 * Input: The client we received this from, the client this originated
 *        from, the channel, the parameter count starting at the modes,
 *        the parameters, the channel name.
 * Output: None.
 * Side-effects: Changes the channel membership and modes appropriately,
 *               sends the appropriate MODE messages to the appropriate
 *               clients.
 */
void
set_channel_mode(struct Client *client_p, struct Client *source_p, struct Channel *chptr,
                 struct Membership *member, int parc, char *parv[], char *chname)
{
  int dir = MODE_ADD;
  int parn = 1;
  int alevel, errors = 0;
  char *ml = parv[0], c;
  int table_position;

  mode_count = 0;
  mode_limit = 0;
  simple_modes_mask = 0;

  execute_callback(channel_access_cb, source_p, member, &alevel);

  for (; (c = *ml) != '\0'; ml++) 
  {
    switch (c)
    {
      case '+':
        dir = MODE_ADD;
        break;
      case '-':
        dir = MODE_DEL;
        break;
      case '=':
        dir = MODE_QUERY;
        break;
      default:
        if (c < 'A' || c > 'z')
          table_position = 0;
        else
          table_position = c - 'A' + 1;
       ModeTable[table_position].func(client_p, source_p, chptr,
                                       parc, &parn,
                                       parv, &errors, alevel, dir, c,
                                       ModeTable[table_position].d,
                                       chname);
       if(dir == MODE_ADD || dir == MODE_DEL)
         execute_callback(on_cmode_change_cb, source_p, chptr, dir, c, parv[parn]);
       break;
    }
  }
}
