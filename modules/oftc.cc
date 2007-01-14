/*
 *  oftc-ircservices: an exstensible and flexible IRC Services package
 *  oftc.c: A protocol handler for the OFTC IRC Network
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
 *  $Id$
 */

#include <string>
#include <sstream>
#include <vector>
#include <tr1/unordered_map>

#include "stdinc.h"
#include "language.h"
#include "interface.h"
#include "connection.h"
#include "client.h"
#include "parse.h"
#include "oftc.h"
#include "conf/connect.h"

extern "C" Protocol *create(string const& name)
{
  return new OFTCProtocol(name);
}

extern "C" void destroy(Protocol *p)
{
  delete p;
}

void SIDMessage::handler(Server *uplink, BaseClient *source, 
    vector<string> args)
{
  Server *newserver = new Server(args[0], args[3]);

  newserver->set_id(args[2]);
  newserver->init();
}

void UIDMessage::handler(Server *uplink, BaseClient *source, 
    vector<string> args)
{
  Client *newclient = new Client(args[0], args[4], args[5], args[8]);
  newclient->set_id(args[7]);
  newclient->init();
}

void PassMessage::handler(Server *uplink, BaseClient *source, 
    vector<string> args)
{
  source->set_id(args[3]);
  // Special case - add to id hash manually
  GlobalIDHash[args[3]] = source;
}

void
OFTCProtocol::init(Parser *p, Connection *c)
{
  Protocol::init(p, c);
  
  parser->add_message(new ServerMessage(6));
  parser->add_message(new SIDMessage());
  parser->add_message(new UIDMessage());
  parser->add_message(new PassMessage());
  parser->add_message(new IgnoreMessage("GNOTICE"));
  parser->add_message(new IgnoreMessage("REALHOST"));
}

void
OFTCProtocol::connected(bool handshake)
{
  std::ostringstream ss;

  ss << "PASS " << connection->password() << " TS 6 :" << me->id();
  connection->send(ss.str());
  connection->send("CAPAB :KLN PARA EOB QS UNKLN GLN ENCAP TBURST CHW IE EX");
  Protocol::introduce_client(me);

  Protocol::connected(false);
}

void
OFTCProtocol::introduce_client(Server *server)
{

}

void
OFTCProtocol::introduce_client(Client *client)
{
  std::ostringstream ss;

  ss << ":" << me->id() << " UID " << client->name() << " 1 1 +o " << 
    client->username() << " " << client->host() << " 255.255.255.255 " <<
    client->id() << " :" << client->gecos();

  connection->send(ss.str());
}
