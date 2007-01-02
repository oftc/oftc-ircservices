/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  module.cc: Module class to deal with loadable modules
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
 *  $Id: /local/oftc-ircservices/branches/ootest/src/conf/modules.cc 1662 2007-01-01T22:02:00.161060Z stu  $
 */

#include "stdinc.h"
#include "language.h"
#include "interface.h"
#include "connection.h"
#include "client.h"
#include "parse.h"

#include "conf/service.h"
#include "module.h"
#include "lua_module.h"
#include "ruby_module.h"
#include <ltdl.h>

#include <string>
#include <vector>
#include <sstream>

using std::string;
using std::stringstream;
using std::vector;

vector<Module *> loaded_modules;
vector<Protocol *>protocol_list;
extern vector<string> mod_extra;

Module *
Module::find(string const& filename)
{
  const string name = libio_basename(filename.c_str());
  vector<Module *>::const_iterator i;

  for(i = loaded_modules.begin(); i != loaded_modules.end(); i++)
  {
    Module *m = *i;

    if(m->name() == name)
      return m;
  }
  return NULL;
}

bool
Module::load(string const& dir, string const& fname)
{
  string path = dir + "/" + fname;
  stringstream message;

  if((handle = lt_dlopenext(path.c_str())) == NULL)
  {
    ilog(L_DEBUG, "Failed to load %s: %s", path.c_str(), lt_dlerror());
    return false;
  }

  message << "Shared module " << _name << " loaded.";

  ilog(L_NOTICE, "%s", message.str().c_str());

  loaded_modules.push_back(this);
  return true;
}

bool
ServiceModule::load(string const& dir, string const& fname)
{
  Service *service;

  if(!Module::load(dir, fname))
    return false;

  create_service = (servcreate_t *)lt_dlsym(handle, "create");
  destroy_service = (servdestroy_t *)lt_dlsym(handle, "destroy");

  service = create_service(service_name);
  service->init();

  return true;
}

bool ProtocolModule::load(string const& dir, string const& fname)
{
  Protocol *protocol;

  if(!Module::load(dir, fname))
    return false;

  create_protocol = (protocreate_t *)lt_dlsym(handle, "create");
  destroy_protocol = (protodestroy_t *)lt_dlsym(handle, "destroy");

  protocol = create_protocol(_proto);
  protocol_list.push_back(protocol);

  return true;
}

/*
 * boot_modules()
 *
 * [API] Initializes core, autoload and conf (extra) modules.
 *
 * inputs: 0 if we should load only conf modules, 1 otherwise
 * output: none
 */
void
boot_modules(char cold)
{
  vector<string>::const_iterator i;
  vector<ServiceConf *>::const_iterator si;

  if(cold)
  {
    /*  char buf[PATH_MAX], *pp;
      struct dirent *ldirent;
      DIR *moddir;

      if((moddir = opendir(AUTOMODPATH)) == NULL)
        ilog(L_WARN, "Could not load modules from %s: %s", AUTOMODPATH,
          strerror(errno));
      else
      {
        while((ldirent = readdir(moddir)) != NULL)
        {
          strlcpy(buf, ldirent->d_name, sizeof(buf));
          if((pp = strchr(buf, '.')) != NULL)
            *pp = 0;
          if(!Module::find(buf))
          {
            Module *m = new Module(buf);
            if(!m->load(AUTOMODPATH, ldirent->d_name))
              delete m;
          }
        }
        closedir(moddir);
      }*/
  }

  for(i = mod_extra.begin(); i != mod_extra.end(); i++)
  {
    if(!Module::find(*i))
    {
      Module *m = new Module(*i);
      if(!m->load(MODPATH, *i))
        delete m;
    }
  }

  for(si = ServiceConfs.begin(); si != ServiceConfs.end(); si++)
  {
    ServiceConf *sc = *si;
    Module *m = new ServiceModule(sc);
    if(!m->load(MODPATH, sc->module))
      delete m;
  }

  Module *m = new ProtocolModule("oftc.so", "oftc");
  if(!m->load(MODPATH, "oftc.so"))
    delete m;
}

void
init_module()
{
  lt_dlmalloc = (lt_ptr (*) LT_PARAMS((size_t))) MyMalloc;
  lt_dlfree   = (void (*) LT_PARAMS((lt_ptr))) MyFree;
  lt_dlinit();
}

void
cleanup_module()
{
  lt_dlexit();
}
