/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  kill.c - functions for killing clients
 *
 *  Copyright (C) 2012 Stuart Walsh and the OFTC Coding department
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
 */

#include "stdinc.h"
#include "interface.h"
#include "kill.h"

static dlink_list kill_list;

static void
check_kills(void *param)
{
  dlink_node *ptr, *nptr;

  DLINK_FOREACH_SAFE(ptr, nptr, kill_list.head)
  {
    struct KillRequest *request = (struct KillRequest *)ptr->data;

    send_kill(request->service, request->client, request->reason);

    dlinkDelete(ptr, &kill_list);
    free_dlink_node(ptr);

    MyFree(request);
  }
}

void
init_kill()
{
  eventAdd("Check kills", check_kills, NULL, 1);
}

void
kill_user(struct Service *service, struct Client *client, const char *reason)
{
  struct KillRequest *request = MyMalloc(sizeof(struct KillRequest));

  request->service = service;
  request->client = client;
  request->reason = reason;

  dlinkAdd(request, make_dlink_node(), &kill_list);  
}

void
kill_remove_service(struct Service *service)
{
  dlink_node *ptr, *nptr;

  DLINK_FOREACH_SAFE(ptr, nptr, kill_list.head)
  {
    struct KillRequest *request = (struct KillRequest *)ptr->data;

    if(request->service == service)
    {
      dlinkDelete(ptr, &kill_list);
      free_dlink_node(ptr);
    }
  }
}

void
kill_remove_client(struct Client *client)
{
  dlink_node *ptr, *nptr;

  DLINK_FOREACH_SAFE(ptr, nptr, kill_list.head)
  {
    struct KillRequest *request = (struct KillRequest *)ptr->data;

    if(request->client == client)
    {
      dlinkDelete(ptr, &kill_list);
      free_dlink_node(ptr);
    }
  }
}
