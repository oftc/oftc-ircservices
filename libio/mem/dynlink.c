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
 *  $Id: dynlink.c 466 2006-02-13 16:33:18Z adx $
 */

#include "libioinc.h"
/*
 * jmallett's dl* interface
 */

#ifdef HAVE_DLOPEN

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#ifdef HAVE_LINK_H
#include <link.h>
#endif

#ifndef RTLD_NOW
#define RTLD_NOW    RTLD_LAZY /* openbsd deficiency */
#endif

void *
modload(const char *name, void **base)
{
  void *handle = dlopen(name, RTLD_NOW);

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
  return dlsym(handle, name);
}

void
modunload(void *handle)
{
  dlclose(handle);
}

const char *
moderror(void)
{
  return dlerror();
}

#else

/*
 * Win32 DLL interface
 */

#ifdef _WIN32

void *
modload(const char *name, void **base)
{
  return (*base = LoadLibrary(filename));
}

void *
modsym(void *handle, const char *name)
{
  return GetProcAddress((HMODULE) handle, name);
}

void
modunload(void *handle)
{
  FreeLibrary((HMODULE) module);
}

const char *
moderror(void)
{
  static char errbuf[IRCD_BUFSIZE];
  char *p;

  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errbuf, sizeof(errbuf), NULL);

  if ((p = strpbrk(errbuf, "\r\n")) != NULL)
    *p = 0;

  return (const char *) errbuf;
}

#else

#ifdef HAVE_MACH_O_DYLD_H

/*
 * NSModule(3) interface
 */

#include <mach-o/dyld.h>

static int myDlError;
static const char *myErrorTable[] =
{
  "Loading file as object failed\n",
  "Loading file as object succeeded\n",
  "Not a valid shared object\n",
  "Architecture of object invalid on this architecture\n",
  "Invalid or corrupt image\n",
  "Could not access object\n",
  "NSCreateObjectFileImageFromFile failed\n",
  NULL
};

void
undefinedErrorHandler(const char *symbolName)
{
  return;
}

NSModule
multipleErrorHandler(NSSymbol s, NSModule old, NSModule new)
{
  /*
   * XXX This results in substantial leaking of memory... Should free
   * one module, maybe?
   */
  return new;
}

void
linkEditErrorHandler(NSLinkEditErrors errorClass, int errnum,
             const char *fileName, const char *errorString)
{
  return;
}

void *
modload(const char *name, void **base)
{
  NSObjectFileImage myImage;
  NSModule myModule;
  static char initialized = NO;

  if (!initialized)
  {
    NSLinkEditErrorHandlers linkEditErrorHandlers;

    linkEditErrorHandlers.undefined = undefinedErrorHandler;
    linkEditErrorHandlers.multiple  = multipleErrorHandler;
    linkEditErrorHandlers.linkEdit  = linkEditErrorHandler;
    NSInstallLinkEditErrorHandlers(&linkEditErrorHandlers);

    initialized = YES;
  }

  myDlError = NSCreateObjectFileImageFromFile(name, &myImage);
  if (myDlError != NSObjectFileImageSuccess)
    return NULL;

  *base = NULL;

  return (void *) NSLinkModule(myImage, name,
    NSLINKMODULE_OPTION_PRIVATE);
}

void *
modsym(void *handle, const char *name)
{
  NSSymbol mySymbol;

  mySymbol = NSLookupSymbolInModule((NSModule) myModule, mySymbolName);
  return NSAddressOfSymbol(mySymbol);
}

void
modunload(void *handle)
{
  NSUnLinkModule(handle, FALSE);
}

const char *
moderror(void)
{
  return (const char *) (myDlError == NSObjectFileImageSuccess ? NULL :
    myErrorTable[myDlError % 7]);
}

#else

#error No applicable dynamic loading interface found!

#endif
#endif
#endif
