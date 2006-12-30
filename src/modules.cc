/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  modules.c: A module loader.
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

#include <string>

using std::string;

class ErrorMessage : public Message
{
public:
  ErrorMessage(string const& n) : Message(n) {};
  ~ErrorMessage()
  {
  };
  void handler(Client *uplink, Client *source, vector<string> args)
  {
    string arg;

    if(args.size() == 0)
      arg = "<>";
    else
      arg = args[0];

    if(uplink == source)
      ilog(L_DEBUG, "Error :from %s -- %s", source->c_name(), arg.c_str());
    else
      ilog(L_DEBUG, "Error :from %s via %s -- %s", source->c_name(), 
          uplink->c_name(), arg.c_str());
  };
};

void
modules_init(Parser *parser)
{
  ErrorMessage *error = new ErrorMessage("ERROR");

  parser->add_message(error);
}
