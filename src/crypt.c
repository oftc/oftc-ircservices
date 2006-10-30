/*
 *  oftc-ircservices: an exstensible and flexible IRC Services package
 *  crypt.c: Functions for encrypting things
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


#include "stdinc.h"

static const char saltChars[] = 
  "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  /* 0 .. 63, ascii - 64 */

char *
generate_md5_salt(char *salt, int length)
{
  int i;
  srandom(time(NULL));
  for(i = 0; i < length; i++)
  {
    salt[i] = saltChars[random() % 64];
  }
  return(salt);
}

char *
crypt_pass(char *password)
{
  char salt[16];

  return servcrypt(password, generate_md5_salt(salt, 16));
}
