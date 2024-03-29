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
#include "conf/service.h"
#include "ruby_module.h"
#include "python_module.h"
#include "mem/dynlink.h"
#include <sys/types.h>
#include <dirent.h>
#include <ltdl.h>

dlink_list loaded_modules = {NULL, NULL, 0};

const char *core_modules[] =
{
  "irc",
  "oftc",
  NULL
};

static dlink_list mod_paths = {NULL, NULL, 0};
static dlink_list mod_extra = {NULL, NULL, 0};
static dlink_node *hreset, *hpass;

//
// Windows ignores case in file names, so using M_PART for m_part
// is perfectly legal.
//

#ifdef _WIN32
# define _COMPARE   strcasecmp
# define _NCOMPARE  strncasecmp
#else
# define _COMPARE   strcmp
# define _NCOMPARE  strncmp
#endif

/*
 * find_module()
 *
 * [API] Checks whether a module is loaded.
 *
 * inputs:
 *   - module name (with or without path/suffix)
 *   - 1 if we should match exact fullname, 0 if only the canonical name
 * output: pointer to struct Module or NULL
 */
struct Module *
find_module(const char *filename, int exact)
{
  dlink_node *ptr;
  const char *name = libio_basename(filename), *p;
  int cnt = ((p = strchr(name, '.')) != NULL ? p - name : strlen(name));

  DLINK_FOREACH(ptr, loaded_modules.head)
  {
    struct Module *mod = ptr->data;

    if (!_NCOMPARE(mod->name, name, cnt) && !mod->name[cnt])
    {
      if (exact && _COMPARE(mod->fullname, filename) != 0)
        continue;
      return mod;
    }
  }

  return NULL;
}

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
static void *
init_module(struct Module *mod, const char *fullname)
{
  char message[IRC_BUFSIZE];

  if (mod->address != NULL)
    snprintf(message, sizeof(message), "Shared module %s loaded at %p",
      fullname, mod->address);
  else
    snprintf(message, sizeof(message), "Loaded %s module %s",
      mod->handle ? "shared" : "built-in", fullname);

  ilog(L_NOTICE, "%s", message);
  ilog(L_DEBUG, "%s", message);

  DupString(mod->fullname, fullname);
  dlinkAdd(mod, &mod->node, &loaded_modules);
  return mod->modinit();
}

/*
 * load_shared_module()
 *
 * Loads a shared module from given directory.
 * WARNING: We don't check for already loaded modules!
 *
 * inputs:
 *   - canonical module name (without suffix)
 *   - directory to load from
 *   - file name
 * output: 1 if ok, 0 otherwise
 */
#ifdef USE_SHARED_MODULES
static void *
load_shared_module(const char *name, const char *dir, const char *fname)
{
  char path[PATH_MAX];
  char sym[PATH_MAX];
  char tmpext[PATH_MAX];
  char tmp;
  char *ext;
  void *handle, *base;
  struct Module *mod;

  snprintf(path, sizeof(path), "%s/%s", dir, fname);

  /* Check what type of module this is and pass it off to the appropriate
   * loader.
   */
  ext = strchr(fname, '.');
  if(ext != NULL)
  {
    tmp = *ext;
    *ext = '\0';
    ext++;
    strlcpy(tmpext, ext, sizeof(tmpext));
    ext--;
    *ext = tmp;
    int result = -1;

    mod = MyMalloc(sizeof(struct Module));
    DupString(mod->name, name);
    DupString(mod->fullname, path);

    if(strcmp(tmpext, "rb") == 0)
    {
#ifdef HAVE_RUBY
      result = load_ruby_module(name, dir, fname);
      mod->type = MODTYPE_RUBY;
#else
      ilog(L_NOTICE, "Trying to load ruby module %s, but ruby is disabled", fname);
      MyFree(mod->name);
      MyFree(mod->fullname);
      MyFree(mod);
      return NULL;
#endif
    }

    if(strcmp(tmpext, "py") == 0)
    {
#ifdef HAVE_PYTHON
      result = load_python_module(name, dir, fname);
      mod->type = MODTYPE_PYTHON;
#else
      ilog(L_NOTICE, "Trying to load python module %s, but python is disabled", fname);
      MyFree(mod->name);
      MyFree(mod->fullname);
      MyFree(mod);
      return NULL;
#endif
    }

    if(result > -1)
    {
      if(result == 1)
      {
        dlinkAdd(mod, &mod->node, &loaded_modules);
        return mod;
      }
      else
      {
        MyFree(mod->name);
        MyFree(mod->fullname);
        MyFree(mod);
        return mod;
      }
    }

    MyFree(mod->name);
    MyFree(mod->fullname);
    MyFree(mod);
  }

  if (!(handle = modload(path, &base)))
  {
    ilog(L_DEBUG, "Failed to load %s: %s", path, lt_dlerror());
    return 0;
  }

  snprintf(sym, sizeof(sym), "%s_module", name);
  if (!(mod = modsym(handle, sym)))
  {
    char error[IRC_BUFSIZE];

    modunload(handle);
    snprintf(error, sizeof(error), "%s contains no %s export!", fname, sym);

    ilog(L_WARN, "%s", error);
    ilog(L_DEBUG, "%s", error);
    return 0;
  }

  mod->handle = handle;
  mod->address = base;
  mod->type = MODTYPE_SO;
  return init_module(mod, fname);
  
}
#endif

/*
 * load_module()
 *
 * [API] A module is loaded from a file or taken from the static pool.
 *
 * inputs: module name (without path, with suffix if needed)
 * output:
 *   NULL if module not found or already loaded
 *   pointer returned by the module's init function otherwise
 *   
 *  
 */
void *
load_module(const char *filename)
{
  char name[PATH_MAX], *p = NULL;
  void *modptr;

  if (find_module(filename, NO) != NULL)
    return NULL;

  if (strpbrk(filename, "\\/") == NULL)
  {
    strlcpy(name, libio_basename(filename), sizeof(name));

    if ((p = strchr(name, '.')) != NULL)
      *p = '\0';

#ifdef USE_SHARED_MODULES
    {
      dlink_node *ptr;

      DLINK_FOREACH(ptr, mod_paths.head)
        if ((modptr = load_shared_module(name, ptr->data, filename)) != NULL)
          return modptr;
    }
#endif
  }

  ilog(L_CRIT, "Cannot locate module %s", filename);
  ilog(L_DEBUG, "Cannot locate module %s", filename);
  return NULL;
}

/*
 * unload_module()
 *
 * [API] A module is unloaded. This is actually MUCH simpler.
 *
 * inputs: pointer to struct Module
 * output: none
 */
void
unload_module(struct Module *mod)
{
  if(mod->type == MODTYPE_SO)
    mod->modremove();

  MyFree(mod->fullname);
  mod->fullname = NULL;

  dlinkDelete(&mod->node, &loaded_modules);

#ifdef USE_SHARED_MODULES

#ifdef HAVE_RUBY
  if (mod->type == MODTYPE_RUBY)
  {
    unload_ruby_module(mod->name);
    MyFree(mod->name);
    MyFree(mod);
    mod = NULL;
  }
#endif
#ifdef HAVE_PYTHON
  if(mod->type == MODTYPE_PYTHON)
  {
    unload_python_module(mod->name);
    MyFree(mod->name);
    MyFree(mod);
    mod = NULL;
  }
#endif

  if (mod != NULL && mod->handle != NULL && !ServicesState.keepmodules)
    modunload(mod->handle);
#endif
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
  dlink_node *ptr;
  const char **p;

  if (cold)
  {
#ifdef USE_SHARED_MODULES
    {
      char buf[PATH_MAX], *pp;
      const char **cp;
      struct dirent *ldirent;
      DIR *moddir;

      for (cp = core_modules; *cp; cp++)
      {
        snprintf(buf, sizeof(buf), "%s%s", *cp, SHARED_SUFFIX);
        load_shared_module(*cp, MODPATH, buf);
      }

      if ((moddir = opendir(AUTOMODPATH)) == NULL)
        ilog(L_WARN, "Could not load modules from %s: %s", AUTOMODPATH,
          strerror(errno));
      else
      {
        while ((ldirent = readdir(moddir)) != NULL)
        {
          strlcpy(buf, ldirent->d_name, sizeof(buf));
          if ((pp = strchr(buf, '.')) != NULL)
            *pp = 0;
          if (!find_module(buf, NO))
            load_shared_module(buf, AUTOMODPATH, ldirent->d_name);
        }
        closedir(moddir);
      }
    }
#endif
  }

  DLINK_FOREACH(ptr, mod_extra.head)
    if (!find_module(ptr->data, NO))
      load_module(ptr->data);

  for (p = core_modules; *p; p++)
  {
    if (!find_module(*p, NO))
    {
      services_die("No core modules", 0);
    }
  }

  DLINK_FOREACH(ptr, service_confs.head)
  {
    struct ServiceConf *sc = ptr->data;

    if(!find_module(sc->module, NO))
      load_shared_module(sc->name, MODPATH, sc->module);
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
  /* XXX Don't load modules on a config switch pass, wait
   * for the rest of the system to be up
  if (conf_pass == 2)
    boot_modules(conf_cold);
  */

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
  dlink_node *ptr, *ptr_next;

  DLINK_FOREACH_SAFE(ptr, ptr_next, mod_paths.head)
  {
    MyFree(ptr->data);
    dlinkDelete(ptr, &mod_paths);
    free_dlink_node(ptr);
  }

  DLINK_FOREACH_SAFE(ptr, ptr_next, mod_extra.head)
  {
    MyFree(ptr->data);
    dlinkDelete(ptr, &mod_extra);
    free_dlink_node(ptr);
  }

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
    char *path;

    chdir(DPATH);
    DupString(path, (char *) value);

    dlinkAddTail(path, make_dlink_node(), &mod_paths);
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
  char *name;

  DupString(name, (char *) value);
  dlinkAddTail(name, make_dlink_node(), &mod_extra);
}

dlink_list *
get_modpaths()
{
  return &mod_paths;
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

  lt_dlinit();
}

void
cleanup_modules(void)
{
  dlink_node *ptr, *nptr;
  struct ConfSection *s = find_conf_section("modules");

  DLINK_FOREACH_SAFE(ptr, nptr, loaded_modules.head)
  {
    struct Module *mod = ptr->data;

    unload_module(mod);
  }

  DLINK_FOREACH_SAFE(ptr, nptr, mod_paths.head)
  {
    MyFree(ptr->data);
    dlinkDelete(ptr, &mod_paths);
    free_dlink_node(ptr);
  }

  delete_conf_section(s);
  MyFree(s);

  lt_dlexit();
}
