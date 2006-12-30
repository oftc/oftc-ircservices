/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
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
#include <openssl/sha.h>
//#include "libio/mem/memory.h"

static const char saltChars[] = 
  "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  /* 0 .. 63, ascii - 64 */

static char bin2hex[] = 
{
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c',
  'd', 'e', 'f'
};


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
crypt_pass_old(char *password)
{
  //char salt[16];
  return NULL;

//  return servcrypt(password, generate_md5_salt(salt, 16));
}

char *crypt_pass(char *password)
{
  EVP_MD_CTX mdctx;
  const EVP_MD *md;
  unsigned char md_value[EVP_MAX_MD_SIZE];
  char buffer[41];
  char *ret;
  int i, j, high, low;
  unsigned int md_len;

  md = EVP_get_digestbyname("SHA1");

  EVP_MD_CTX_init(&mdctx);
  EVP_DigestInit_ex(&mdctx, md, NULL);
  EVP_DigestUpdate(&mdctx, password, strlen(password));
  EVP_DigestFinal_ex(&mdctx, md_value, &md_len);
  EVP_MD_CTX_cleanup(&mdctx);

  for (i = 0, j = 0; i < 20; i++, j += 2) 
  {
    high = md_value[i] >> 4;
    low = md_value[i] - (high << 4);
    buffer[j] = bin2hex[high];
    buffer[j + 1] = bin2hex[low];
  }
  buffer[40] = '\0';

  DupString(ret, buffer);
  return ret;
}
