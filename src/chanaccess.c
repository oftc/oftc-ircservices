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
#include "channel_mode.h"
#include "channel.h"
#include "interface.h"
#include "client.h"
#include "hostmask.h"
#include "nickname.h"
#include "akick.h"

static struct ChanAccess *
row_to_chanaccess(row_t *row)
{
  struct ChanAccess *access;
  access = MyMalloc(sizeof(struct ChanAccess));
  access->id = atoi(row->cols[0]);
  access->channel = atoi(row->cols[1]);

  if(row->cols[2] != NULL)
    access->account = atoi(row->cols[2]);
  else
    access->account = 0;

  if(row->cols[3] != NULL)
    access->group = atoi(row->cols[3]);
  else
    access->group = 0;

  access->level = atoi(row->cols[4]);
  return access;
}

void
chanaccess_list_free(dlink_list *list)
{
  dlink_node *ptr, *next;
  struct ChanAccess *access;

  ilog(L_DEBUG, "Freeing ChanAccess list %p of length %lu", list, 
      dlink_list_length(list));

  DLINK_FOREACH_SAFE(ptr, next, list->head)
  {
    access = (struct ChanAccess*)ptr->data;
    MyFree(access);
    dlinkDelete(ptr, list);
    free_dlink_node(ptr);
  }
}

int
chanaccess_list(unsigned int channel, dlink_list *list)
{
  result_set_t *results;
  int error;
  int i;
  int res = 0;

  results = db_execute(GET_CHAN_ACCESSES, &error, "i", &channel);
  if(results == NULL && error != 0)
  {
    ilog(L_CRIT, "chanaccess_list: database error %d", error);
    return FALSE;
  }
  else if(results == NULL)
    res = 0;

  if(results->row_count == 0)
    res = 0;

  if(results != NULL)
  {
    for(i = 0; i < results->row_count; i++)
    {
      row_t *row = &results->rows[i];
      struct ChanAccess *access = row_to_chanaccess(row);

      dlinkAdd(access, make_dlink_node(), list);
    }

    db_free_result(results);
  }

  results = db_execute(GET_CHAN_ACCESSES_GROUP, &error, "i", &channel);
  if(results == NULL && error != 0)
  {
    ilog(L_CRIT, "chanaccess_list: database error %d", error);
    return res;
  }
  else if(results == NULL)
    return res;

  if(results->row_count == 0)
    return dlink_list_length(list);

  for(i = 0; i < results->row_count; i++)
  {
    row_t *row = &results->rows[i];
    struct ChanAccess *access = row_to_chanaccess(row);

    dlinkAdd(access, make_dlink_node(), list);
  }

  db_free_result(results);

  return dlink_list_length(list);
}

int
chanaccess_add(struct ChanAccess *access)
{
  int ret;

  if(access->account > 0)
    ret = db_execute_nonquery(INSERT_CHANACCESS, "iii", &access->account, 
        &access->channel, &access->level);
  else
    ret = db_execute_nonquery(INSERT_CHANACCESS_GROUP, "iii", &access->group,
        &access->channel, &access->level);

  if(ret == -1)
    return FALSE;

  return TRUE;
}

int
chanaccess_remove(struct ChanAccess *access)
{
  int ret;

  ret = db_execute_nonquery(DELETE_CHAN_ACCESS, "ii", &access->channel, &access->account);

  if(ret == -1)
    return FALSE;

  return TRUE;
}

struct ChanAccess *
chanaccess_find(unsigned int channel, unsigned int account)
{
  int error;
  result_set_t *results;
  struct ChanAccess *access;

  results = db_execute(GET_CHAN_ACCESS, &error, "ii", &channel, &account);

  if(results == NULL && error != 0)
  {
    ilog(L_CRIT, "chanaccess_find: database error %d", error);
    return FALSE;
  }
  else if(results == NULL)
    return FALSE;

  if(results->row_count == 0)
    return FALSE;

  access = row_to_chanaccess(&results->rows[0]);

  db_free_result(results);

  return access;
}

struct ChanAccess *
chanaccess_find_exact(unsigned int channel, unsigned int account, unsigned int group)
{
  int error;
  result_set_t *results;
  struct ChanAccess *access;

  if(account > 0)
    results = db_execute(GET_CHAN_ACCESS_EXACT, &error, "ii", &channel, &account);
  else
    results = db_execute(GET_CHAN_ACCESS_GROUP, &error, "ii", &channel, &group);

  if(results == NULL && error != 0)
  {
    ilog(L_CRIT, "chanaccess_find: database error %d", error);
    return FALSE;
  }
  else if(results == NULL)
    return FALSE;

  if(results->row_count == 0)
    return FALSE;

  access = row_to_chanaccess(&results->rows[0]);

  db_free_result(results);

  return access;
}

int
chanaccess_count(unsigned int channel)
{
  int error = 0;
  int count = atoi(db_execute_scalar(COUNT_CHANNEL_ACCESS_LIST, &error, "i", &channel));
  if(error)
  {
    ilog(L_CRIT, "chanaccess_count: database error %d", error);
    return -1;
  }

  return count;
}
