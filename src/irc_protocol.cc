/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  interface.c: Functions for interfacing with service modules
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
 *  $Id: /local/oftc-ircservices/branches/ootest/src/interface.cc 1653 2006-12-31T23:12:16.092182Z stu  $
 */

#include <string>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "stdinc.h"
#include "language.h"
#include "interface.h"
#include "parse.h"
#include "connection.h"
#include "client.h"

using std::string;
using std::stringstream;
using std::runtime_error;

class IgnoreMessage : public Message
{
public:
  IgnoreMessage(string const& n) : Message(n) {};
  IgnoreMessage()
  {
  };
  void handler(Server *uplink, BaseClient *source, vector<string> args)
  {
  }
};

class ErrorMessage : public Message
{
public:
  ErrorMessage() : Message("ERROR") {};
  ~ErrorMessage()
  {
  };
  void handler(Server *uplink, BaseClient *source, vector<string> args)
  {
    stringstream ss;

    ss << "Error :from " << source->name();

    if(uplink != source)
      ss << " via " << uplink->name();

    if(args.size() == 0)
      ss << "<>";
    else
      ss << args[0];

    ilog(L_DEBUG, "%s", ss.str().c_str());
  };
};

class PingMessage : public Message
{
public:
  PingMessage() : Message("PING") {};
  ~PingMessage()
  {
  };
  void handler(Server *uplink, BaseClient *source, vector<string> args)
  {
    stringstream ss;

    ss << ":" << me->name() << " PONG " << me->name() << " :" << 
      source->name();
    uplink->send(ss.str());
  };
};

class ServerMessage : public Message
{
public:
  ServerMessage() : Message("SERVER") {};
  ~ServerMessage()
  {
  };
  void handler(Server *uplink, BaseClient *source, vector<string> args)
  {
    stringstream ss;
    Server *newserver;

    newserver = dynamic_cast<Server *>(Server::find(args[0]));
    if(newserver == uplink)
    {
      ilog(L_DEBUG, "Completed connection to server %s", uplink->name().c_str());
      ss << "SVINFO 5 5 0: " << CurrentTime;
      uplink->send(ss.str());
    }
    else
    {
      newserver = new Server(args[0], args[2]);
      newserver->init();
      ilog(L_DEBUG, "New server %s from hub %s", args[0].c_str(), 
          source->name().c_str());
    }
  };
};

class NickMessage : public Message
{
public:
  NickMessage() : Message("NICK") {};
  ~NickMessage()
  {
  };
  void handler(Server *uplink, BaseClient *source, vector<string> args)
  {
    stringstream ss;
    BaseClient *target;

    target = Client::find(args[0]);
    if(args.size() == 8)
    {
      Server *fromserv = dynamic_cast<Server *>(Server::find(args[6]));

      if(fromserv == NULL)
      {
        ilog(L_ERROR, "Got NICK %s from server %s via %s which is an unknown server",
            args[0].c_str(), args[6].c_str(), source->name().c_str());
        return;
      }
      target = Client::find(args[0]);
      if(target == NULL)
      {
        target = new Client(args[0], args[4], args[5], args[7]);
        target->init();
        return;
      }
    }
    if(target == source)
    {
      if(target->name() != args[0])
        target->set_name(args[0]);
    }
  };
};

Protocol::Protocol() : name("IRC"), parser(0), connection(0)
{
}

void
Protocol::init(Parser *p, Connection *c)
{
  PingMessage   *ping   = new PingMessage();
  ErrorMessage  *error  = new ErrorMessage();
  ServerMessage *server = new ServerMessage();
  NickMessage   *nick   = new NickMessage();
  IgnoreMessage *ignore = new IgnoreMessage("EOB"); 

  parser = p;
  connection = c;

  parser->add_message(ping);
  parser->add_message(error);
  parser->add_message(server);
  parser->add_message(nick);
  parser->add_message(ignore);
}

void Protocol::connected()
{
  stringstream ss;

  ss << "PASS " << connection->password() << " TS 5";
  connection->send(ss.str());
  connection->send("CAPAB :KLN PARA EOB QS UNKLN GLN ENCAP TBURST CHW IE EX");
  introduce_server(me);

  connection->process_send_queue();
}

void Protocol::introduce_server(Client *client)
{
  stringstream ss;

  ss << "SERVER " << client->name() << " 1 :" << client->gecos();
  connection->send(ss.str());
}
