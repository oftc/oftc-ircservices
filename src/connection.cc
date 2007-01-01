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

#include <string>
#include <sstream>
#include <queue>
#include <iostream>
#include <tr1/unordered_map>

#include "stdinc.h"
#include "language.h"
#include "interface.h"
#include "connection.h"
#include "client.h"
#include "parse.h"
#include "conf/connect.h"
#include "services.h"

using std::string;
using std::stringstream;
using std::cout;
using std::endl;

void
Connection::connect()
{
//  struct Module *protomod;

/*  protomod = find_module(Connect.protocol, NO);
  if(protomod == NULL)
  {
    ilog(L_CRIT, "Unable to connect to uplink, protocol module %s not found.",
        Connect.protocol);
    services_die("Connect error", NO);
  }

  ServerModeList = (struct ModeList *)modsym(protomod->handle, "ModeList");
  ilog(L_DEBUG, "Loaded server mode list %p %c %d", ServerModeList, 
      ServerModeList[0].letter, ServerModeList[0].mode);*/

  protocol = new Protocol();
  protocol->init(parser, this);

  if(comm_open(&fd, AF_INET, SOCK_STREAM, 0, NULL) < 0)
  {
    ilog(L_DEBUG, "connect_server: Could not open socket");
    exit(1);
  }

  comm_connect_tcp(&fd, Connect.host, Connect.port,
      NULL, 0, connect_callback, this, AF_INET, CONNECTTIMEOUT);
}

void
Connection::connect_callback(fde_t *fd, int status, void *data)
{
  Connection *connection = static_cast<Connection*>(data);
  Server *server;

  if(status != COMM_OK)
  {
    ilog(L_DEBUG, "serv_connect_callback: Connect failed :(");
    exit(1);
  }

  ilog(L_DEBUG, "serv_connect_callback: Connect succeeded!");

  server = new Server(Connect.name, Connect.name);
  server->init();
  server->set_connection(connection);

  connection->set_server(server);
  connection->set_password(Connect.password);
  connection->connected();
  connection->setup_read();
  
//  execute_callback(connected_cb, client);
}

void
Connection::read()
{
  int length = 0;
  char readBuf[READBUF_SIZE];

  memset(readBuf, 0, READBUF_SIZE);

  do
  {
    length = recv(fd.fd, readBuf, READBUF_SIZE, 0);
    if(length <= 0)
    {
      if(length < 0 && ignoreErrno(errno))
        break;

      services_die("connection went dead on read", NO);
    }
    read_queue.push_back(readBuf);
    process_read_queue();
  } while(length == sizeof(readBuf));
  this->setup_read();
}

void
Connection::process_read_queue()
{
  char c;
  int line_bytes, empty_bytes, phase;
  stringstream ss;
  string line;

  line_bytes = empty_bytes = phase = 0; 

  while(!read_queue.empty())
  {
    string s = read_queue.front();
    string::const_iterator i;

    for(i = s.begin(); i != s.end(); i++)
    {
      c = *i;
      if(IsEol(c) || (c == ' ' && phase != 1))
      {
        empty_bytes++;
        if(phase == 1)
          phase = 2;
      }
      else switch(phase)
      {
        case 0: 
          phase = 1;
        case 1: 
          if(line_bytes++ < IRC_BUFSIZE - 2)
            ss << c;
          break;
        case 2: 
          line = ss.str();
          
          parser->parse_line(this, line);
          
          line_bytes = empty_bytes = phase = 0;
          ss.clear();
          ss.flush();
          ss.str("");
          i--;
          break;
      }
    }
    if(phase != 2)
    {
      if(read_queue.size() == 1)
        return; // This is the only element in the queue, so we need more.
      read_queue.pop_front();
    }
    else
    {
      line = ss.str();

      parser->parse_line(this, line);

      line_bytes = empty_bytes = phase = 0;
      ss.clear();
      ss.flush();
      ss.str("");
      read_queue.pop_front();
    }
  }
}

void
Connection::send(string const &message)
{
  string msg;
  cout << "Queued: " << message << endl;
  
  msg = message.substr(0, 509);
  msg.append("\r\n");
  send_queue.push_back(msg);
}

void
Connection::process_send_queue()
{
  while(!send_queue.empty())
  {
    string message = send_queue.front();
    int len;

    len = ::send(fd.fd, message.c_str(), message.length(), 0);
    if(len <= 0)
      services_die("Connection went dead on write", 0);

    send_queue.pop_front();
  }
}
