/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  modules.c: Defines the modules{} block of ircd.conf.
 *
 *  Copyright (C) 2005 by the Hybrid Development Team.
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
#include "conf/conf.h"
#include "lua_module.h"
#include "ruby_module.h"
#include <sys/types.h>
#include <dirent.h>
#include <dlfcn.h>

#include <vector>
#include <string>
#include <sstream>

using std::string;
using std::stringstream;
using std::vector;

vector<Module *> loaded_modules;
vector<string> mod_paths;
vector<string> mod_extra;

static dlink_node *hreset, *hpass;

Module *
Module::find(string const& filename)
{
  const string name = libio_basename(filename.c_str());
  vector<Module *>::const_iterator i;

  for(i = loaded_modules.begin(); i != loaded_modules.end(); i++)
  {
    Module *m = *i;

    if(m->get_name() == name)
      return m;
  }
  return NULL;
}

#if 0
/*
 * init_module()
 *
 * Deals with initializing an already loaded module.
 *
 * inputs:
 *   - pointer to struct Module
 *   - name that will be passed to load_module on reloading
 *     (contains suffix for shared modules)
 * output: none, except success report
 */
static void
init_module(struct Module *mod, const char *fullname)
{
  mod->modinit();
}
#endif

bool
Module::load(string const& dir, string const& fname)
{
  string path = dir + "/" + fname;
  stringstream message;

  if(!(handle = modload(path.c_str(), &address)))
  {
    ilog(L_DEBUG, "Failed to load %s: %s", path.c_str(), dlerror());
    return false;
  }

  message << "Shared module " << name << " loaded at " << address;

  ilog(L_NOTICE, "%s", message.str().c_str());
  ilog(L_DEBUG, "%s", message.str().c_str());

  loaded_modules.push_back(this);
 // init_module(mod, fname);
  return true;
}

#if 0
/*
 * load_module()
 *
 * [API] A module is loaded from a file or taken from the static pool.
 *
 * inputs: module name (without path, with suffix if needed)
 * output:
 *   -1 if the module was already loaded (no error message),
 *    0 if loading failed (errors reported),
 *    1 if ok (success reported)
 */
int
load_module(const char *filename)
{
  char name[PATH_MAX], *p = NULL;

  if (find_module(filename, NO) != NULL)
    return -1;

  if (strpbrk(filename, "\\/") == NULL)
  {
    strlcpy(name, libio_basename(filename), sizeof(name));

    if ((p = strchr(name, '.')) != NULL)
      *p = '\0';

#ifdef SHARED_MODULES
    {
      dlink_node *ptr;

      DLINK_FOREACH(ptr, mod_paths.head)
        if (load_shared_module(name, (const char *)ptr->data, filename))
          return 1;
    }
#endif

#ifdef BUILTIN_MODULES
    if (mod == NULL)
    {
      struct Module **mptr;

      for (mptr = builtin_mods; *mptr; mptr++)
      {
        if (!_COMPARE((*mptr)->name, filename))
        {
          init_module(*mptr, mod->name);
          return 1;
        }
      }
    }
#endif
  }

  ilog(L_CRIT, "Cannot locate module %s", filename);
  ilog(L_DEBUG, "Cannot locate module %s", filename);
  return 0;
}

/*
 * unload_module()
 *
 * [API] A module is unloaded. This is actually MUCH simplier.
 *
 * inputs: pointer to struct Module
 * output: none
 */
void
unload_module(struct Module *mod)
{
  mod->modremove();

  MyFree(mod->fullname);
  mod->fullname = NULL;

  dlinkDelete(&mod->node, &loaded_modules);

#ifdef SHARED_MODULES
  if (mod->handle != NULL)
    modunload(mod->handle);
#endif
}
#endif
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

  if(cold)
  {
    {
      char buf[PATH_MAX], *pp;
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
      }
    }
  }

  for(i = mod_extra.begin(); i != mod_extra.end(); i++)
  {
    if(!Module::find(*i))
    {
      Module *m = new Module(*i);
      if(!m->load(AUTOMODPATH, *i))
        delete m;
    }
  }
}

/*
 * h_switch_conf_pass()
 *
 * Hook function called after switching to conf_pass.
 * (After pass 1 we boot all modules.)
 *
 * inputs: none
 * output: none
 */
static void *
h_switch_conf_pass(va_list args)
{
  if (conf_pass == 2)
    boot_modules(conf_cold);

  return pass_callback(hpass);
}

/*
 * h_reset_conf()
 *
 * Hook function called before parsing the conf file.
 * Clears out old paths/extra modules.
 *
 * inputs: none
 * output: none
 */
static void *
h_reset_conf(va_list args)
{
  mod_paths.clear();
  mod_extra.clear();

  return pass_callback(hreset);
}

/*
 * mod_add_path()
 *
 * Deals with path="..." conf entry.
 *
 * inputs: value text
 * output: none
 */
static void
mod_add_path(void *value, void *unused)
{
  if (!chdir((char *) value))
  {
    mod_paths.push_back((char*)value);
    chdir(DPATH);
  }
  else
    parse_error("directory not found");
}

/*
 * mod_add_module()
 *
 * Deals with module="..." conf entry.
 *
 * inputs: value text
 * output: none
 */
static void
mod_add_module(void *value, void *unused)
{
  mod_extra.push_back((char*)value);
}

/*
 * init_modules()
 *
 * Defines the modules{} conf section.
 *
 * inputs: none
 * output: none
 */
void
init_modules(void)
{
  struct ConfSection *s = add_conf_section("modules", 1);

  hreset = install_hook(reset_conf, h_reset_conf);
  hpass = install_hook(switch_conf_pass, h_switch_conf_pass);

  add_conf_field(s, "path", CT_STRING, mod_add_path, NULL);
  add_conf_field(s, "module", CT_STRING, mod_add_module, NULL);
}

void
cleanup_modules(void)
{
}
