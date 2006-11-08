/*
 *  oftc-ircservices: an exstensible and flexible IRC Services package
 *  parse.c: The message parser
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

/*
 * (based on orabidoo's parser code)
 *
 * This has always just been a trie. Look at volume III of Knuth ACP
 *
 *
 * ok, you start out with an array of pointers, each one corresponds
 * to a letter at the current position in the command being examined.
 *
 * so roughly you have this for matching 'trie' or 'tie'
 *
 * 't' points -> [MessageTree *] 'r' -> [MessageTree *] -> 'i'
 *   -> [MessageTree *] -> [MessageTree *] -> 'e' and matches
 *
 *                               'i' -> [MessageTree *] -> 'e' and matches
 *
 * BUGS (Limitations!)
 * 
 * I designed this trie to parse ircd commands. Hence it currently
 * casefolds. This is trivial to fix by increasing MAXPTRLEN.
 * This trie also "folds" '{' etc. down. This means, the input to this
 * trie must be alpha tokens only. This again, is a limitation that
 * can be overcome by increasing MAXPTRLEN to include upper/lower case
 * at the expense of more memory. At the extreme end, you could make
 * MAXPTRLEN 128.
 *
 * This is also not a patricia trie. On short ircd tokens, this is
 * not likely going to matter. 
 *
 * Diane Bruce (Dianora), June 6 2003
 */

static struct MessageTree irc_msg_tree;

/*
 * NOTE: parse() should not be called recursively by other functions!
 */
static char *sender;
static char *para[IRCD_MAXPARA + 1];
static char *servpara[IRCD_MAXPARA+1];

static void handle_command(struct Message *, struct Client *, struct Client *, unsigned int, char **);
//static void recurse_report_messages(struct Client *source_p, struct MessageTree *mtree);
static void add_msg_element(struct MessageTree *mtree_p, struct Message *msg_p, const char *cmd);
static void del_msg_element(struct MessageTree *mtree_p, const char *cmd);
static void serv_add_msg_element(struct ServiceMessageTree *mtree_p, struct ServiceMessage *, const char *cmd);
static void serv_del_msg_element(struct ServiceMessageTree *mtree_p, const char *cmd);

void
init_parser()
{
  clear_tree_parse(&irc_msg_tree);
}
/* turn a string into a parc/parv pair */
static inline int
string_to_array(char *string, char *parv[])
{
  char *p;
  char *buf = string;
  int x = 1;

  parv[x] = NULL;

  while (*buf == ' ') /* skip leading spaces */
    buf++;

  if (*buf == '\0') /* ignore all-space args */
    return(x);

  do
  {
    if (*buf == ':') /* Last parameter */
    {
      buf++;
      parv[x++] = buf;
      parv[x]   = NULL;
      return(x);
    }
    else
    {
      parv[x++] = buf;
      parv[x]   = NULL;

      if ((p = strchr(buf, ' ')) != NULL)
      {
        *p++ = '\0';
        buf  = p;
      }
      else
        return(x);
    }       

    while (*buf == ' ')
      buf++;

    if (*buf == '\0')
      return(x);
  } while (x < IRCD_MAXPARA - 1);

  if (*p == ':')
    p++;

  parv[x++] = p;
  parv[x]   = NULL;
  return(x);
}

/*
 * parse a buffer.
 *
 * NOTE: parse() should not be called recusively by any other functions!
 */
void
parse(struct Client *client, char *pbuffer, char *bufend)
{
  struct Client *from = client;
  char *ch;
  char *s;
  char *numeric = 0;
  unsigned int i = 0;
  int paramcount;
  int mpara = 0;
  struct Message *mptr = NULL;

  if (IsDefunct(client->server))
    return;

  assert(client->server->fd.flags.open);
  assert((bufend - pbuffer) < 512);

  for (ch = pbuffer; *ch == ' '; ch++) /* skip spaces */
    /* null statement */ ;

  para[0] = from->name;

  if (*ch == ':')
  {
    ch++;

    /* Copy the prefix to 'sender' assuming it terminates
     * with SPACE (or NULL, which is an error, though).
     */
    sender = ch;

    if ((s = strchr(ch, ' ')) != NULL)
    {
      *s = '\0';
      s++;
      ch = s;
    }

    if (*sender)
    {
      /*
       * XXX it could be useful to know which of these occurs most frequently.
       * the ID check should always come first, though, since it is so easy.
       */
      if ((from = find_person(client, sender)) == NULL)
      {
        from = find_server(sender);
      }
      
      /*
       * Hmm! If the client corresponding to the
       * prefix is not found--what is the correct
       * action??? Now, I will ignore the message
       * (old IRC just let it through as if the
       * prefix just wasn't there...) --msa
       */
      if (from == NULL)
      {
        printf("from null, sender:%s\n", sender);
        return;
      }

      para[0] = from->name;

      if (from->from != client)
      {
        printf("from from is not client %s %s %s %s %s\n", 
            from->from->name, client->name, sender, ch, s);
        return;
      }
    }

    while (*ch == ' ')
      ++ch;
  }

  if (*ch == '\0')
  {
    return;
  }

  /* Extract the command code from the packet.  Point s to the end
   * of the command code and calculate the length using pointer
   * arithmetic.  Note: only need length for numerics and *all*
   * numerics must have parameters and thus a space after the command
   * code. -avalon
   */

  /* EOB is 3 chars long but is not a numeric */
  if (*(ch + 3) == ' ' && /* ok, lets see if its a possible numeric.. */
      IsDigit(*ch) && IsDigit(*(ch + 1)) && IsDigit(*(ch + 2)))
  {
    mptr = NULL;
    numeric = ch;
    paramcount = IRCD_MAXPARA;
    s = ch + 3;  /* I know this is ' ' from above if            */
    *s++ = '\0'; /* blow away the ' ', and point s to next part */
  }
  else
  { 
    int ii = 0;

    if ((s = strchr(ch, ' ')) != NULL)
      *s++ = '\0';

    if ((mptr = find_command(ch, &irc_msg_tree)) == NULL)
    {
      printf("Unknown Message: %s %s\n", ch, s);
      return;
    }

    assert(mptr->cmd != NULL);

    paramcount = mptr->parameters;
    mpara      = mptr->maxpara;

    ii = bufend - ((s) ? s : ch);
    mptr->bytes += ii;
  }

  if (s != NULL)
    i = string_to_array(s, para);
  else
  {
    i = 0;
    para[1] = NULL;
  }

  if (mptr != NULL)
    handle_command(mptr, client, from, i, para);
}
/* handle_command()
 *
 * inputs	- pointer to message block
 *		- pointer to client
 *		- pointer to client message is from
 *		- count of number of args
 *		- pointer to argv[] array
 * output	- -1 if error from server
 * side effects	-
 */
static void
handle_command(struct Message *mptr, struct Client *client,
               struct Client *from, unsigned int i, char *hpara[])
{
  MessageHandler handler = 0;

  if (IsServer(client))
    mptr->rcount++;

  mptr->count++;

  handler = mptr->handlers[client->handler];

  /* check right amount of params is passed... --is */
  if (i < mptr->parameters)
  {
    printf("Dropping server %s due to (invalid) command '%s' "
        "with only %d arguments (expecting %d).",
        client->name, mptr->cmd, i, mptr->parameters);
    exit_client(client, client, "Not enough arguments to server command.");
  }
  else
    (*handler)(client, from, i, hpara);
}

static void
handle_services_command(struct ServiceMessage *mptr, struct Service *service,
     struct Client *from, unsigned int i, char *hpara[])
{
  ServiceMessageHandler handler = 0;
  
  mptr->count++;
  handler = mptr->handlers[from->service_handler];

  if (i < mptr->parameters)
  {
    reply_user(service, from, "Insufficient Parameters");
    printf("%s sent services a command %s with too few paramters\n",
        from->name, mptr->cmd);
  }
  else
  {
    service->last_command = (char *)mptr->cmd;
    (*handler)(service, from, i, hpara);
  }
}

/* clear_tree_parse()
 *
 * inputs       - msg_tree - the tree to clear
 * output       - NONE
 * side effects - MUST MUST be called at startup ONCE before
 *                any other keyword routine is used.
 */
void
clear_tree_parse(struct MessageTree *msg_tree)
{
  memset(msg_tree, 0, sizeof(*msg_tree));
}

void
clear_serv_tree_parse(struct ServiceMessageTree *msg_tree)
{
  memset(msg_tree, 0, sizeof(*msg_tree));
}

/* add_msg_element()
 *
 * inputs	- pointer to MessageTree
 *		- pointer to Message to add for given command
 *		- pointer to current portion of command being added
 * output	- NONE
 * side effects	- recursively build the Message Tree ;-)
 */
/*
 * How this works.
 *
 * The code first checks to see if its reached the end of the command
 * If so, that struct MessageTree has a msg pointer updated and the links
 * count incremented, since a msg pointer is a reference.
 * Then the code descends recursively, building the trie.
 * If a pointer index inside the struct MessageTree is NULL a new
 * child struct MessageTree has to be allocated.
 * The links (reference count) is incremented as they are created
 * in the parent.
 */
static void
add_msg_element(struct MessageTree *mtree_p, 
		struct Message *msg_p, const char *cmd)
{
  struct MessageTree *ntree_p;

  if (*cmd == '\0')
  {
    mtree_p->msg = msg_p;
    mtree_p->links++;    /* Have msg pointer, so up ref count */
  }
  else
  {
    /* *cmd & (MAXPTRLEN-1) 
     * convert the char pointed to at *cmd from ASCII to an integer
     * between 0 and MAXPTRLEN.
     * Thus 'A' -> 0x1 'B' -> 0x2 'c' -> 0x3 etc.
     */

    if ((ntree_p = mtree_p->pointers[*cmd & (MAXPTRLEN-1)]) == NULL)
    {
      ntree_p = (struct MessageTree *)MyMalloc(sizeof(struct MessageTree));
      mtree_p->pointers[*cmd & (MAXPTRLEN-1)] = ntree_p;

      mtree_p->links++;    /* Have new pointer, so up ref count */
    }

    add_msg_element(ntree_p, msg_p, cmd+1);
  }
}

static void
serv_add_msg_element(struct ServiceMessageTree *mtree_p,
    struct ServiceMessage *msg_p, const char *cmd)
{
  struct ServiceMessageTree *ntree_p;

  if (*cmd == '\0')
  {
    mtree_p->msg = msg_p;
    mtree_p->links++;    /* Have msg pointer, so up ref count */
  }
  else
  {
    /* *cmd & (MAXPTRLEN-1)
     * convert the char pointed to at *cmd from ASCII to an integer
     * between 0 and MAXPTRLEN.
     * Thus 'A' -> 0x1 'B' -> 0x2 'c' -> 0x3 etc.
     */

    if ((ntree_p = mtree_p->pointers[*cmd & (MAXPTRLEN-1)]) == NULL)
    {
      ntree_p = (struct ServiceMessageTree *)
        MyMalloc(sizeof(struct ServiceMessageTree));
      mtree_p->pointers[*cmd & (MAXPTRLEN-1)] = ntree_p;

      mtree_p->links++;    /* Have new pointer, so up ref count */
    }

    serv_add_msg_element(ntree_p, msg_p, cmd+1);
  }
}


/* del_msg_element()
 *
 * inputs	- Pointer to MessageTree to delete from
 *		- pointer to command name to delete
 * output	- NONE
 * side effects	- recursively deletes a token from the Message Tree ;-)
 */
/*
 * How this works.
 *
 * Well, first off, the code recursively descends into the trie
 * until it finds the terminating letter of the command being removed.
 * Once it has done that, it marks the msg pointer as NULL then
 * reduces the reference count on that allocated struct MessageTree
 * since a command counts as a reference.
 *
 * Then it pops up the recurse stack. As it comes back up the recurse
 * The code checks to see if the child now has no pointers or msg
 * i.e. the links count has gone to zero. If its no longer used, the
 * child struct MessageTree can be deleted. The parent reference
 * to this child is then removed and the parents link count goes down.
 * Thus, we continue to go back up removing all unused MessageTree(s)
 */
static void
del_msg_element(struct MessageTree *mtree_p, const char *cmd)
{
  struct MessageTree *ntree_p;

  /*
   * In case this is called for a nonexistent command
   * check that there is a msg pointer here, else links-- goes -ve
   * -db
   */
  if ((*cmd == '\0') && (mtree_p->msg != NULL))
  {
    mtree_p->msg = NULL;
    mtree_p->links--;
  }
  else
  {
    if ((ntree_p = mtree_p->pointers[*cmd & (MAXPTRLEN-1)]) != NULL)
    {
      del_msg_element(ntree_p, cmd+1);

      if (ntree_p->links == 0)
      {
        mtree_p->pointers[*cmd & (MAXPTRLEN-1)] = NULL;
        mtree_p->links--;
        MyFree(ntree_p);
      }
    }
  }
}

static void
serv_del_msg_element(struct ServiceMessageTree *mtree_p, const char *cmd)
{
  struct ServiceMessageTree *ntree_p;

  /*
   * In case this is called for a nonexistent command
   * check that there is a msg pointer here, else links-- goes -ve
   * -db
   */
  if ((*cmd == '\0') && (mtree_p->msg != NULL))
  {
    mtree_p->msg = NULL;
    mtree_p->links--;
  }
  else
  {
    if ((ntree_p = mtree_p->pointers[*cmd & (MAXPTRLEN-1)]) != NULL)
    {
      serv_del_msg_element(ntree_p, cmd+1);

      if (ntree_p->links == 0)
      {
        mtree_p->pointers[*cmd & (MAXPTRLEN-1)] = NULL;
        mtree_p->links--;
        MyFree(ntree_p);
      }
    }
  }
}

/* msg_tree_parse()
 *
 * inputs	- Pointer to command to find
 *		- Pointer to MessageTree root
 * output	- Find given command returning Message * if found NULL if not
 * side effects	- none
 */
static struct Message *
msg_tree_parse(const char *cmd, struct MessageTree *root)
{
  struct MessageTree *mtree = root;
  assert(cmd && *cmd);

  while (IsAlpha(*cmd) && (mtree = mtree->pointers[*cmd & (MAXPTRLEN - 1)]))
    if (*++cmd == '\0')
      return mtree->msg;

  return NULL;
}

static struct ServiceMessage *
serv_msg_tree_parse(const char *cmd, struct ServiceMessageTree *root)
{
  struct ServiceMessageTree *mtree = root;
  assert(cmd && *cmd);

  while (IsAlpha(*cmd) && (mtree = mtree->pointers[*cmd & (MAXPTRLEN - 1)]))
    if (*++cmd == '\0')
      return mtree->msg;

  return NULL;
}


/* mod_add_cmd()
 *
 * inputs	- pointer to struct Message
 * output	- none
 * side effects - load this one command name
 *		  msg->count msg->bytes is modified in place, in
 *		  modules address space. Might not want to do that...
 */
void
mod_add_cmd(struct Message *msg)
{
  struct Message *found_msg = NULL;

  if (msg == NULL)
    return;

  /* someone loaded a module with a bad messagetab */
  assert(msg->cmd != NULL);

  /* command already added? */
  if ((found_msg = msg_tree_parse(msg->cmd, &irc_msg_tree)) != NULL)
    return;

  add_msg_element(&irc_msg_tree, msg, msg->cmd);
  msg->count = msg->rcount = msg->bytes = 0;
}

/* mod_add_servcmd()
 *
 * inputs - pointer to Message Tree
          - pointer to struct Message
 * output - none
 * side effects - load this one command name
 *      msg->count msg->bytes is modified in place, in
 *      modules address space. Might not want to do that...
 */
void
mod_add_servcmd(struct ServiceMessageTree *msg_tree, struct ServiceMessage *msg)
{
  struct ServiceMessage *found_msg = NULL;

  if (msg == NULL)
    return;

  /* someone loaded a module with a bad messagetab */
  assert(msg->cmd != NULL);

  /* command already added? */
  if ((found_msg = serv_msg_tree_parse(msg->cmd, msg_tree)) != NULL)
    return;

  serv_add_msg_element(msg_tree, msg, msg->cmd);
}


/* mod_del_cmd()
 *
 * inputs	- pointer to struct Message
 * output	- none
 * side effects - unload this one command name
 */
void
mod_del_cmd(struct Message *msg)
{
  assert(msg != NULL);

  if (msg == NULL)
    return;

  del_msg_element(&irc_msg_tree, msg->cmd);
}

/* mod_del_servcmd()
 *
 * inputs - pointer to Message Tree
          - pointer to struct Message
 * output - none
 * side effects - unload this one command name
 */
void
mod_del_servcmd(struct ServiceMessageTree *msg_tree, struct ServiceMessage *msg)
{
  assert(msg != NULL);

  if (msg == NULL)
    return;

  serv_del_msg_element(msg_tree, msg->cmd);
}


/* find_command()
 *
 * inputs	- command name
 * output	- pointer to struct Message
 * side effects - none
 */
struct Message *
find_command(const char *cmd, struct MessageTree *msg_tree)
{
  return msg_tree_parse(cmd, msg_tree);
}

struct ServiceMessage *
find_services_command(const char *cmd, struct ServiceMessageTree *msg_tree)
{
  return serv_msg_tree_parse(cmd, msg_tree);
}

static void
recurse_help_messages(struct Service *service, struct Client *client,
    struct ServiceMessageTree *mtree)
{
  int i;

  if (mtree->msg != NULL)
  {
    reply_user(service, client, "\002%s\002: %s",
        mtree->msg->cmd, _L(service, client, mtree->msg->help_short));
  }
  for (i = 0; i < MAXPTRLEN; i++)
  {
    if (mtree->pointers[i] != NULL)
      recurse_help_messages(service, client, mtree->pointers[i]);
  }
}

void
do_serv_help_messages(struct Service *service, struct Client *client)
{
  struct ServiceMessageTree *mtree = &service->msg_tree;
  int i;

  for (i = 0; i < MAXPTRLEN; i++)
  {
    if (mtree->pointers[i] != NULL)
      recurse_help_messages(service, client, mtree->pointers[i]);
  }
}

#if 0
/* report_messages()
 *
 * inputs	- pointer to client to report to
 * output	- NONE
 * side effects	- client is shown list of commands
 */
void
report_messages(struct Client *source_p)
{
  struct MessageTree *mtree = &msg_tree;
  int i;

  for (i = 0; i < MAXPTRLEN; i++)
  {
    if (mtree->pointers[i] != NULL)
      recurse_report_messages(source_p, mtree->pointers[i]);
  }
}

static void
recurse_report_messages(struct Client *source_p, struct MessageTree *mtree)
{
  int i;

  if (mtree->msg != NULL)
  {
    sendto_one(source_p, form_str(RPL_STATSCOMMANDS),
               me.name, source_p->name, mtree->msg->cmd,
               mtree->msg->count, mtree->msg->bytes,
               mtree->msg->rcount);
  }

  for (i = 0; i < MAXPTRLEN; i++)
  {
    if (mtree->pointers[i] != NULL)
      recurse_report_messages(source_p, mtree->pointers[i]);
  }
}
#endif

void
m_ignore(struct Client *client, struct Client *source, int parc, char *parv[])
{
  return;
}

void
m_servignore(struct Service *service, struct Client *source, int parc, 
    char *parv[])
{
}

void
m_alreadyreg(struct Service *service, struct Client *source, int parc, 
    char *parv[])
{
  reply_user(service, source, "%s is already registered dumbass.", 
      source->name);
}

void
m_unreg(struct Service *service, struct Client *source,
    int parc, char *parv[])
{
  reply_user(service, source, "You are not identified.",
        source->name);
}

void
process_privmsg(struct Client *client, struct Client *source, 
    int parc, char *parv[])
{
  struct Service *service;
  struct ServiceMessage *mptr;
  char  *s, *ch, *ch2;
  int i;

  service = find_service(parv[1]);
  if(service == NULL)
  {
    printf("Message for %s from %s, who we know nothing about!", parv[1], 
        source->name);
    return;
  }

  for (ch = parv[2]; *ch == ' '; ch++) /* skip spaces */
    /* null statement */ ;

  if ((s = strchr(ch, ' ')) != NULL)
    *s++ = '\0';

  if ((mptr = find_services_command(ch, &service->msg_tree)) == NULL)
  {
    printf("Unknown Message: %s %s for service %s from %s\n", ch, s, 
        parv[1], source->name);
    reply_user(service, source, "Unknown command");
    return;
  }

  assert(mptr->cmd != NULL);
  parv[3] = s;

  if(mptr->sub != NULL && parv[3] != NULL)
  {
    /* Process sub commands */
    struct SubMessage *sub;

    sub = mptr->sub;

    for (ch2 = parv[3]; *ch2 == ' '; ch2++) /* skip spaces */
      ;
    
    i = string_to_array(ch2, servpara);

    if(*servpara[1] == '#' && i > 2)
    {
      printf("Got %s which is possibly a channeli\n", servpara[1]);
      servpara[0] = servpara[1];
      servpara[1] = servpara[2];
      servpara[2] = servpara[0];
    }

    while(sub->cmd != NULL)
    {
      if(irccmp(sub->cmd, servpara[1]) == 0)
      {
        servpara[0] = source->name;

        if(i > 2)
        {
          int i2;
          
          for(i2 = 2; i2 < i; i2++)
          {
            servpara[i2-1] = servpara[i2];
          }
          servpara[i2-1] = NULL;
          i--;
        }
        else
        {
          i = 0;
          servpara[1] = NULL;
        }

        sub->count++;
        if (i < sub->parameters)
        {
          reply_user(service, source, "Insufficient Parameters");
          printf("%s sent services a sub command %s with too few paramters\n",
              client->name, sub->cmd);
          return;
        }
        else
        {
          if(mptr->handlers[source->service_handler] == m_unreg)
          {
            m_unreg(service, source, (i == 0) ? i : i-1, servpara);
            return;
          }
          (*sub->handler)(service, source, (i == 0) ? i : i-1, servpara);
          return;
        }
      }
      sub++;
    }
  }

  servpara[0] = source->name;

  if(s != NULL)
    i = string_to_array(s, servpara);
  else
  {
    i = 0;
    servpara[1] = NULL;
  }

  handle_services_command(mptr, service, source, (i == 0) ? i : i-1, servpara);
}

size_t
join_params(char *target, int parc, char *parv[])
{
  size_t length = 0;
  int i;

  if(parv[0] != NULL)
    length += strlcpy(target, parv[0], IRC_BUFSIZE);
  for(i = 1; i < parc; i++)
  {
    length += strlcat(target, " ", IRC_BUFSIZE);
    length += strlcat(target, parv[i], IRC_BUFSIZE);
  }

  return length;
}
