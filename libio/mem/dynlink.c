/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  dynlink.c: Low level interface for module support.
 *
 *  Copyright (C) 2002-2006 by the past and present ircd coders, and others.
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

#include "libioinc.h"

#ifdef HAVE_LINK_H
#include <link.h>
#endif

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#include <ltdl.h>
#ifdef USE_SHARED_MODULES

void *
modload(const char *name, void **base)
{
  void *handle = lt_dlopenext(name);

  if (handle)
  {
#ifdef HAVE_DLINFO
    struct link_map *map;

    if (!dlinfo(handle, RTLD_DI_LINKMAP, &map))
      *base = (void*)map->l_addr;
    else
#endif
      *base = NULL;
  }

  return handle;
}

void *
modsym(void *handle, const char *name)
{
  return lt_dlsym(handle, name);
}

void
modunload(void *handle)
{
  lt_dlclose(handle);
}

const char *
moderror(void)
{
  return lt_dlerror();
}

#endif /* USE_SHARED_MODULES */
