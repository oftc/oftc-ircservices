/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  modules.h: Defines modules{} conf section.
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

#ifndef INCLUDED_conf_modules_h
#define INCLUDED_conf_modules_h

enum ModType
{
  MODTYPE_RUBY,
  MODTYPE_PYTHON,
  MODTYPE_SO
};

struct Module
{
  char *name;
  const char *version;
  void *(* modinit) (void);
  void (* modremove) (void);
  char *fullname;
  void *handle;
  void *address;
  dlink_node node;
  enum ModType type;
};

#define INIT_MODULE(NAME, REV) \
  static void *_modinit(void); \
  static void _moddeinit(void); \
  struct Module NAME ## _module = {#NAME, REV, _modinit, _moddeinit}; \
  static void *_modinit(void)

#define CLEANUP_MODULE \
  static void _moddeinit(void)

#ifdef IN_CONF_C
void init_modules(void);
#endif

EXTERN dlink_list loaded_modules;
EXTERN const char *core_modules[];

EXTERN struct Module *find_module(const char *, int);
EXTERN void * load_module(const char *);
EXTERN void unload_module(struct Module *);
EXTERN void boot_modules(char);
EXTERN void cleanup_modules();
EXTERN dlink_list* get_modpaths();

#endif /* INCLUDED_conf_modules_h */
