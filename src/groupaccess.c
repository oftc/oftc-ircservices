/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  groupaccess.c - Functions for managing group access lists(member lists)
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
 *  MERgroupTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
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
#include "group.h"
#include "channel_mode.h"
#include "channel.h"
#include "interface.h"
#include "client.h"
#include "hostmask.h"
#include "nickname.h"
#include "akick.h"

static struct GroupAccess *
row_to_groupaccess(row_t *row)
{
  struct GroupAccess *access;
  access = MyMalloc(sizeof(struct GroupAccess));
  access->id = atoi(row->cols[0]);
  access->group = atoi(row->cols[1]);
  access->account = atoi(row->cols[2]);
  access->level = atoi(row->cols[3]);
  return access;
}

void
groupaccess_list_free(dlink_list *list)
{
  dlink_node *ptr, *next;
  struct GroupAccess *access;

  ilog(L_DEBUG, "Freeing group access list %p of length %lu", list, 
      dlink_list_length(list));

  DLINK_FOREACH_SAFE(ptr, next, list->head)
  {
    access = (struct GroupAccess*)ptr->data;
    MyFree(access);
    dlinkDelete(ptr, list);
    free_dlink_node(ptr);
  }
}

int
groupaccess_list(unsigned int group, dlink_list *list)
{
  result_set_t *results;
  int error;
  int i;

  results = db_execute(GET_GROUP_ACCESSES, &error, "i", &group);
  if(results == NULL && error != 0)
  {
    ilog(L_CRIT, "groupaccess_list: database error %d", error);
    return FALSE;
  }
  else if(results == NULL)
    return FALSE;

  if(results->row_count == 0)
    return FALSE;

  for(i = 0; i < results->row_count; i++)
  {
    row_t *row = &results->rows[i];
    struct GroupAccess *access = row_to_groupaccess(row);

    dlinkAdd(access, make_dlink_node(), list);
  }

  db_free_result(results);

  return dlink_list_length(list);
}

int
groupaccess_add(struct GroupAccess *access)
{
  int ret;

  ret = db_execute_nonquery(INSERT_GROUPACCESS, "iii", &access->account, 
      &access->group, &access->level);

  if(ret == -1)
    return FALSE;

  return TRUE;
}

int
groupaccess_remove(struct GroupAccess *access)
{
  int ret;

  ret = db_execute_nonquery(DELETE_GROUPACCESS, "ii", &access->group, 
      &access->account);

  if(ret == -1)
    return FALSE;

  return TRUE;
}

struct GroupAccess *
groupaccess_find(unsigned int group, unsigned int account)
{
  int error;
  result_set_t *results;
  struct GroupAccess *access;

  results = db_execute(GET_GROUP_ACCESS, &error, "ii", &group, &account);
  if(results == NULL && error != 0)
  {
    ilog(L_CRIT, "groupaccess_find: database error %d", error);
    return FALSE;
  }
  else if(results == NULL)
    return FALSE;

  if(results->row_count == 0)
    return FALSE;

  access = row_to_groupaccess(&results->rows[0]);

  db_free_result(results);

  return access;
}

int
groupaccess_count(unsigned int group)
{
  int error = 0;
  int count = atoi(db_execute_scalar(COUNT_GROUP_ACCESS_LIST, &error, "i", 
        &group));
  
  if(error)
  {
    ilog(L_CRIT, "groupaccess_count: database error %d", error);
    return -1;
  }

  return count;
}
