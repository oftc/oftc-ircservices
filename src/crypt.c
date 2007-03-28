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
 *  Base16 en/decoding is:
 *  Copyright (c) 2001-2004, Roger Dingledine
 *  Copyright (c) 2004-2007, Roger Dingledine, Nick Mathewson
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *  Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice, 
 * this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the distribution.
 *
 * Neither the names of the copyright owners nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
  char salt[16];

  return servcrypt(password, generate_md5_salt(salt, 16));
}

char *crypt_pass(char *password, int encode)
{
  EVP_MD_CTX mdctx;
  const EVP_MD *md;
  unsigned char md_value[EVP_MAX_MD_SIZE];
  char buffer[2*DIGEST_LEN + 1];
  unsigned char *ret;
  int i, j, high, low;
  unsigned int md_len;

  md = EVP_get_digestbyname(DIGEST_FUNCTION);

  EVP_MD_CTX_init(&mdctx);
  EVP_DigestInit_ex(&mdctx, md, NULL);
  EVP_DigestUpdate(&mdctx, password, strlen(password));
  EVP_DigestFinal_ex(&mdctx, md_value, &md_len);
  EVP_MD_CTX_cleanup(&mdctx);

  if(encode)
  {
    base16_encode(buffer, sizeof(buffer), md_value, DIGEST_LEN);
    DupString(ret, buffer);
  }
  else
  {
    ret = MyMalloc(DIGEST_LEN);
    memcpy(ret, md_value, DIGEST_LEN);
  }
  return ret;
}

/** Encode the <b>srclen</b> bytes at <b>src</b> in a NUL-terminated,
 * uppercase hexadecimal string; store it in the <b>destlen</b>-byte buffer
 * <b>dest</b>.
 */
void
base16_encode(char *dest, size_t destlen, const char *src, size_t srclen)
{
  const char *end;
  char *cp;

  assert(destlen >= srclen*2+1);

  cp = dest;
  end = src+srclen;
  while (src<end) {
    *cp++ = "0123456789ABCDEF"[ (*(const uint8_t*)src) >> 4 ];
    *cp++ = "0123456789ABCDEF"[ (*(const uint8_t*)src) & 0xf ];
    ++src;
  }
  *cp = '\0';
}
