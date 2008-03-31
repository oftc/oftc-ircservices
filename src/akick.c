/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  akick.c - akick related functions
 *
 *  Copyright (C) 2006 Stuart Walsh and the OFTC Coding department
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
 *  $Id: dbm.c 1260 2007-12-07 08:53:17Z swalsh $
 */

#include "stdinc.h"
#include "language.h"
#include "parse.h"
#include "dbm.h"
#include "nickname.h"
#include "dbchannel.h"
#include "channel_mode.h"
#include "channel.h"
#include "interface.h"
#include "client.h"
#include "hostmask.h"
#include "akick.h"

static struct ServiceBan *
row_to_akick(row_t *row)
{
  struct ServiceBan *sban;

  sban = MyMalloc(sizeof(struct ServiceBan));
  sban->id = atoi(row->cols[0]);
  sban->channel = atoi(row->cols[1]);
  if(row->cols[2] != NULL)
    sban->target = atoi(row->cols[2]);
  else
    sban->target = 0;
  sban->setter = atoi(row->cols[3]);
  DupString(sban->mask, row->cols[4]);
  DupString(sban->reason, row->cols[5]);
  sban->time_set = atoi(row->cols[6]);
  sban->duration = atoi(row->cols[7]);
  sban->type = AKICK_BAN;

  return sban;
}

static int
akick_check_mask(struct Client *client, const char *mask)
{
  struct irc_ssaddr addr;
  struct split_nuh_item nuh;
  char name[NICKLEN];
  char user[USERLEN + 1];
  char host[HOSTLEN + 1];
  int type, bits, found = 0;

  DupString(nuh.nuhmask, mask);
  nuh.nickptr  = name;
  nuh.userptr  = user;
  nuh.hostptr  = host;

  nuh.nicksize = sizeof(name);
  nuh.usersize = sizeof(user);
  nuh.hostsize = sizeof(host);

  split_nuh(&nuh);

  type = parse_netmask(host, &addr, &bits);

  if(match(name, client->name) && match(user, client->username))
  {
    switch(type)
    {
      case HM_HOST:
        if(match(host, client->host))
          found = 1;
        break;
      case HM_IPV4:
        if (client->aftype == AF_INET)
          if (match_ipv4(&client->ip, &addr, bits))
            found = 1;
        break;
#ifdef IPV6
      case HM_IPV6:
        if (client->aftype == AF_INET6)
          if (match_ipv6(&client->ip, &addr, bits))
            found = 1;
        break;
#endif
    }
  }
  MyFree(nuh.nuhmask);
  return found;
}

void
akick_list_free(dlink_list *list)
{
  dlink_node *ptr, *next;
  struct ServiceBan *sban;

  ilog(L_DEBUG, "Freeing akick list %p of length %lu", list, 
      dlink_list_length(list));

  DLINK_FOREACH_SAFE(ptr, next, list->head)
  {
    sban = (struct ServiceBan *)ptr->data;
    free_serviceban(sban);
    dlinkDelete(ptr, list);
    free_dlink_node(ptr);
  }
}

int
akick_list(unsigned int channel, dlink_list *list)
{
  result_set_t *results;
  int error;
  int i;

  results = db_execute(GET_AKICKS, &error, "i", &channel);
  if(results == NULL && error != 0)
  {
    ilog(L_CRIT, "akick_list: database error %d", error);
    return FALSE;
  }
  else if(results == NULL)
    return FALSE;

  if(results->row_count == 0)
    return FALSE;

  for(i = 0; i < results->row_count; i++)
  {
    row_t *row = &results->rows[i];
    struct ServiceBan *sban = row_to_akick(row);

    dlinkAdd(sban, make_dlink_node(), list);
  }

  db_free_result(results);

  return dlink_list_length(list);
}

static int
akick_enforce_one(struct Service *service, struct Channel *chptr,
  struct Client *client, struct ServiceBan *sban)
{
  char host[HOSTLEN+1];

  if(sban->mask == NULL)
  {
    char *nick = nickname_nick_from_id(sban->target, TRUE);
    if(ircncmp(nick, client->name, NICKLEN) == 0)
    {
      snprintf(host, HOSTLEN, "%s!*@*", nick);
      ban_mask(service, chptr, host);
      kick_user(service, chptr, client->name, sban->reason);
      MyFree(nick);
      return TRUE;
    }
    MyFree(nick);
    return FALSE;
  }
  else if(akick_check_mask(client, sban->mask))
  {
    char banmask[IRC_BUFSIZE+1];
    ircsprintf(banmask, "%s", sban->mask);
    ban_mask(service, chptr, banmask);
    kick_user(service, chptr, client->name, sban->reason);

    return TRUE;
  }

  return FALSE;
}

int
akick_check_client(struct Service *service, struct Channel *chptr, struct Client *client)
{
  dlink_list list = { 0 };
  dlink_node *ptr;

  if(chptr->regchan == NULL)
    return FALSE;

  akick_list(dbchannel_get_id(chptr->regchan), &list);

  DLINK_FOREACH(ptr, list.head)
  {
    struct ServiceBan *sban = (struct ServiceBan *)ptr->data;
    if(akick_enforce_one(service, chptr, client, sban))
    {
      akick_list_free(&list);
      return TRUE;
    }
  }

  akick_list_free(&list);
  return FALSE;
}

int
akick_add(struct ServiceBan *akick)
{
  int ret;

  akick->type = AKICK_BAN;

  if(akick->target != 0)
    ret = db_execute_nonquery(INSERT_AKICK_ACCOUNT, "iiisii", &akick->channel,
        &akick->target, &akick->setter, akick->reason, &akick->time_set,
        &akick->duration);
  else if(akick->mask != NULL)
    ret = db_execute_nonquery(INSERT_AKICK_MASK, "iissii", &akick->channel,
        &akick->setter, akick->reason, akick->mask, &akick->time_set, &akick->duration);
  else
    assert(1 == 0);

  if(ret == -1)
    return FALSE;

  return TRUE;
}

int
akick_remove_index(unsigned int channel, unsigned int index)
{
  int ret = db_execute_nonquery(DELETE_AKICK_IDX, "ii", &index, &channel);
  if(ret == -1)
    return FALSE;

  return TRUE;
}

int
akick_remove_mask(unsigned int channel, const char *mask)
{
  int ret = db_execute_nonquery(DELETE_AKICK_MASK, "is", &channel, mask);
  if(ret == -1)
    return FALSE;

  return TRUE;
}

int
akick_remove_account(unsigned int channel, const char *account)
{
  int ret = db_execute_nonquery(DELETE_AKICK_ACCOUNT, "is", &channel, account);
  return ret == -1 ? FALSE : TRUE;
}

int
akick_enforce(struct Service *service, struct Channel *chptr,
  struct ServiceBan *akick)
{
  dlink_node *ptr;
  dlink_node *next_ptr;
  int numkicks = 0;

  DLINK_FOREACH_SAFE(ptr, next_ptr, chptr->members.head)
  {
    struct Membership *ms = ptr->data;
    struct Client *client = ms->client_p;

    numkicks += akick_enforce_one(service, chptr, client, akick);
  }
  return numkicks;
}
