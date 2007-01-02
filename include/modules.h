/*
 *  modules.h: A header for the modules functions.
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

#ifndef INCLUDED_modules_h
#define INCLUDED_modules_h

#ifdef USE_SHARED_MODULES
#include <ltdl.h>

using std::string;
using std::vector;

typedef Service* servcreate_t(string const&);
typedef void servdestroy_t(Service *);

typedef Protocol* protocreate_t(string const&);
typedef void protodestroy_t(Protocol *);

class Module;

EXTERN vector<Module *>loaded_modules;
EXTERN vector<string> mod_paths;

class Module
{
public:
  // Constructors
  Module() : _name(""), _version(""), handle(0), address(0) {};
  Module(string const& n) : _name(n), _version(""), handle(0), address(0) { };
  virtual ~Module() {};

  // Members
  virtual bool load(string const&, string const&);

  // Property Accessors
  const string& name() const { return _name; };

  // Static Members
  static Module *find(string const&);
protected:
  string _name;
  string _version;
  lt_dlhandle handle;
  void *address;
};

class ServiceModule : public Module
{
public:
  ServiceModule() : Module(), create_service(0), destroy_service(0) {};
  ServiceModule(string const& name) : Module(name), create_service(0), destroy_service(0) {};
  ServiceModule(ServiceConf *c) : 
    Module(c->module), service_name(c->name), create_service(0), destroy_service(0) {};
  
  bool load(string const&, string const&);
private:
  string service_name;
  servcreate_t *create_service;
  servdestroy_t *destroy_service;
};

class ProtocolModule : public Module
{
public:
  ProtocolModule() : Module(), create_protocol(0), destroy_protocol(0) {};
  ProtocolModule(string const& name) : Module(name), create_protocol(0), destroy_protocol(0) {};
  ProtocolModule(string const& name, string const& proto) : Module(name),
    _proto(proto), create_protocol(0), destroy_protocol(0) {};

  bool load(string const&, string const&);
private:
  string _proto;
  protocreate_t *create_protocol;
  protodestroy_t *destroy_protocol;
};


EXTERN void init_module();
EXTERN void cleanup_module();

#endif /* USE_SHARED_MODULES */
#endif /* INCLUDED_modules_h */
