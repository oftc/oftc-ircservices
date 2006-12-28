/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  connection.c: Connection functions
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

static CNCB serv_connect_callback;
struct Callback *connected_cb;

static void
serv_connect_callback(fde_t *fd, int status, void *data)
{
  struct Client *client = (struct Client*)data;
  struct Server *server = NULL;

  assert(client != NULL);

  server = client->server;

  assert(server != NULL);
  assert(&server->fd == fd);

  if(status != COMM_OK)
  {
    ilog(L_DEBUG, "serv_connect_callback: Connect failed :(");
    exit(1);
  }

  ilog(L_DEBUG, "serv_connect_callback: Connect succeeded!");
  comm_setselect(fd, COMM_SELECT_READ, read_packet, client, 0);

  dlinkAdd(client, &client->node, &global_server_list);
  
  execute_callback(connected_cb, client);
}

void 
connect_server()
{
  struct Client *client = make_client(NULL);
  struct Server *server = make_server(client);
  struct Module *protomod;

  protomod = find_module(Connect.protocol, NO);
  if(protomod == NULL)
  {
    ilog(L_CRIT, "Unable to connect to uplink, protocol module %s not found.",
        Connect.protocol);
    services_die("Connect error", NO);
  }

  ServerModeList = (struct ModeList *)modsym(protomod->handle, "ModeList");
  ilog(L_DEBUG, "Loaded server mode list %p %c %d", ServerModeList, 
      ServerModeList[0].letter, ServerModeList[0].mode);

  strlcpy(server->pass, Connect.password, sizeof(server->pass));
  strlcpy(client->name, Connect.name, sizeof(client->name));
  strlcpy(client->host, Connect.host, sizeof(client->host));

  SetConnecting(client);
    
  dlinkAdd(client, &client->node, &global_client_list);

  if(comm_open(&server->fd, AF_INET, SOCK_STREAM, 0, NULL) < 0)
  {
    ilog(L_DEBUG, "connect_server: Could not open socket");
    exit(1);
  }

  comm_connect_tcp(&server->fd, Connect.host, Connect.port,
      NULL, 0, serv_connect_callback, client, AF_INET, CONNECTTIMEOUT);
  
}

void *
server_connected(va_list args)
{
  return NULL;
}
