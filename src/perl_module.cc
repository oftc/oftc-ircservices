/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  perl_module.c: An interface to run perl scripts
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
 *  $Id$
 */

typedef        struct crypt_data {     /* straight from /usr/include/crypt.h */
  /* From OSF, Not needed in AIX
   *        char C[28], D[28];
   *            */
  char E[48];
  char KS[16][48];
  char block[66];
  char iobuf[16];
} CRYPTD;

#include <EXTERN.h>
#include <perl.h>
#undef load_module
#undef my_perl
#undef opendir
#undef readdir
#undef strerror

PerlInterpreter *P;

void
init_perl()
{
  P = perl_alloc();
  perl_construct(P);
}

void destroy_perl() {
  perl_destruct(P);
  perl_free(P);
}

int
load_perl_module(const char *name, const char *dir, const char *fname)
{
  int status;
  char path[PATH_MAX];
  char *embedding[] = { "",  path };

  snprintf(path, sizeof(path), "%s/%s", dir, fname);

  ilog(L_DEBUG, "Loading perl module: %s\n", path);
  status = perl_parse(P, NULL, 2, embedding, NULL);
  if(status != 0)
    return 0;
  status = perl_run(P);
  if(status != 0)
    return 0;

  return 1;
}
