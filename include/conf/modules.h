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

using std::string;
using std::vector;

typedef Service* create_t();
typedef void destroy_t(Service *);

class Module;

extern vector<Module *>loaded_modules;
extern vector<string> mod_paths;

class Module
{
public:
  // Constants
  static const int SERVICE_MODULE = 0;

  // Constructors
  Module() : handle(0), address(0) {};
  Module(string const& n) : name(n) { Module(); };

  // Members
  bool load(string const&, string const&, int=SERVICE_MODULE);

  // Property Accessors
  const string& get_name() const { return name; };

  // Static Members
  static Module *find(string const&);
protected:
  string name;
  string version;
  create_t *create_service;
  destroy_t *destroy_service;
  void *handle;
  void *address;
};

#define INIT_MODULE(NAME, REV) \
  static void _modinit(void); \
  static void _moddeinit(void); \
  struct Module NAME ## _module = {#NAME, REV, _modinit, _moddeinit}; \
  static void _modinit(void)

#define CLEANUP_MODULE \
  static void _moddeinit(void)

#ifdef IN_CONF_C
void init_modules(void);
#endif

EXTERN int load_module(const char *);
EXTERN void unload_module(struct Module *);
EXTERN void boot_modules(char);
EXTERN void cleanup_modules(void);
