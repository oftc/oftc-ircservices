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
#include <sys/types.h>
#include <dirent.h>

dlink_list loaded_modules = {NULL, NULL, 0};

const char *core_modules[] =
{
  "irc",
  NULL
};

#ifdef BUILTIN_MODULES
static struct Module builtin_mods[] = {BUILTIN_MODULES, NULL};
#endif
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
static void
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
  printf("%s", message);

  DupString(mod->fullname, fullname);
  dlinkAdd(mod, &mod->node, &loaded_modules);
  mod->modinit();
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
#ifdef SHARED_MODULES
static int
load_shared_module(const char *name, const char *dir, const char *fname)
{
  char path[PATH_MAX];
  char sym[PATH_MAX];
  void *handle, *base;
  struct Module *mod;

  snprintf(path, sizeof(path), "%s/%s", dir, fname);
  if (!(handle = modload(path, &base)))
  {
    printf("Failed to load %s: %s\n", path, dlerror());
    return 0;
  }

  snprintf(sym, sizeof(sym), "%s_module", name);
  if (!(mod = modsym(handle, sym)))
  {
    char error[IRC_BUFSIZE];

    modunload(handle);
    snprintf(error, sizeof(error), "%s contains no %s export!", fname, sym);

    ilog(L_WARN, "%s", error);
    printf("%s", error);
    return 0;
  }

  mod->handle = handle;
  mod->address = base;
  init_module(mod, fname);
  return 1;
}
#endif

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
        if (load_shared_module(name, ptr->data, filename))
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
  printf("Cannot locate module %s",
                       filename);
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
#ifdef SHARED_MODULES
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

#ifdef BUILTIN_MODULES
    {
      struct Module *mptr;

      for (mptr = builtin_mods; *mptr; mptr++)
        if (!find_module(mptr->name, NO))
          init_module(mptr);
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
