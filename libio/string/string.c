/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  irc_string.c: IRC string functions.
 *
 *  Copyright (C) 2002 by the past and present ircd coders, and others.
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

#ifndef INADDRSZ 
#define INADDRSZ 4
#endif

#ifndef IN6ADDRSZ
#define IN6ADDRSZ 16
#endif

#ifndef INT16SZ
#define INT16SZ 2
#endif

/*
 * myctime - This is like standard ctime()-function, but it zaps away
 *   the newline from the end of that string. Also, it takes
 *   the time value as parameter, instead of pointer to it.
 *   Note that it is necessary to copy the string to alternate
 *   buffer (who knows how ctime() implements it, maybe it statically
 *   has newline there and never 'refreshes' it -- zapping that
 *   might break things in other places...)
 *
 *
 * Thu Nov 24 18:22:48 1986 
 */
const char *
myctime(time_t value)
{
  static char buf[32];
  char *p;

  strcpy(buf, ctime(&value));

  if ((p = strchr(buf, '\n')) != NULL)
    *p = '\0';
  return buf;
}

/*
 * clean_string - clean up a string possibly containing garbage
 *
 * *sigh* Before the kiddies find this new and exciting way of 
 * annoying opers, lets clean up what is sent to local opers
 * -Dianora
 */
char *
clean_string(char* dest, const unsigned char* src, size_t len)
{
  char* d    = dest; 
  assert(0 != dest);
  assert(0 != src);

  if(dest == NULL || src == NULL)
    return NULL;
    
  len -= 3;  /* allow for worst case, '^A\0' */

  while (*src && (len > 0))
  {
    if (*src & 0x80)             /* if high bit is set */
    {
      *d++ = '.';
      --len;
    }
    else if (!IsPrint(*src))       /* if NOT printable */
    {
      *d++ = '^';
      --len;
      *d++ = 0x40 + *src;   /* turn it into a printable */
    }
    else
      *d++ = *src;
    ++src, --len;
  }
  *d = '\0';
  return dest;
}

/*
 * strip_tabs(dst, src, length)
 *
 *   Copies src to dst, while converting all \t (tabs) into spaces.
 */
void
strip_tabs(char *dest, const char *src, size_t len)
{
  char *d = dest;

  /* Sanity check; we don't want anything nasty... */
  assert(dest != NULL);
  assert(src  != NULL);
  assert(len > 0);

  for (; --len && *src; ++src)
    *d++ = *src == '\t' ? ' ' : *src;

  *d = '\0'; /* NUL terminate, thanks and goodbye */
}

/*
 * strtoken - walk through a string of tokens, using a set of separators
 *   argv 9/90
 *
 */
#ifndef HAVE_STRTOK_R

char *
strtoken(char** save, char* str, const char* fs)
{
  char* pos = *save;  /* keep last position across calls */
  char* tmp;

  if (str)
    pos = str;    /* new string scan */

  while (pos && *pos && strchr(fs, *pos) != NULL)
    ++pos;        /* skip leading separators */

  if (!pos || !*pos)
    return (pos = *save = NULL);   /* string contains only sep's */

  tmp = pos;       /* now, keep position of the token */

  while (*pos && strchr(fs, *pos) == NULL)
    ++pos;       /* skip content of the token */

  if (*pos)
    *pos++ = '\0';    /* remove first sep after the token */
  else
    pos = NULL;    /* end of string */

  *save = pos;
  return tmp;
}

#endif /* !HAVE_STRTOK_R */

/* libio_basename()
 *
 * input	- i.e. "/usr/local/ircd/modules/m_whois.so"
 * output	- i.e. "m_whois.so"
 * side effects - this will be overwritten on subsequent calls
 */
const char *
libio_basename(const char *path)
{
  const char *s = strrchr(path, '/');
#ifdef _WIN32
  const char *s2 = strrchr(path, '\\');

  s = IRCD_MAX(s, s2);
#endif

  if (s == NULL)
    s = path;
  else
    s++;

  return s;
}

/*
 * Copyright (c) 1996-1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#define SPRINTF(x) ((size_t)ircsprintf x)

/*
 * WARNING: Don't even consider trying to compile this on a system where
 * sizeof(int) < 4.  sizeof(int) > 4 is fine; all the world's not a VAX.
 */

static const char *inet_ntop4(const unsigned char *src, char *dst, unsigned int size);
#ifdef IPV6
static const char *inet_ntop6(const unsigned char *src, char *dst, unsigned int size);
#endif

static const char *
inetntoa(const char *src)
{
  static char buf[16];
  int oct[4];

  oct[0] = *src++;
  oct[1] = *src++;
  oct[2] = *src++;
  oct[3] = *src++;

  snprintf(buf, sizeof(buf), "%d.%d.%d.%d",
           oct[0], oct[1], oct[2], oct[3]);
  return buf;
}

/* const char *
 * inet_ntop4(src, dst, size)
 *	format an IPv4 address
 * return:
 *	`dst' (as a const)
 * notes:
 *	(1) uses no statics
 *	(2) takes a unsigned char* not an in_addr as input
 * author:
 *	Paul Vixie, 1996.
 */
static const char *
inet_ntop4(const unsigned char *src, char *dst, unsigned int size)
{
  if (size < 16)
    return NULL;

  return strcpy(dst, inetntoa((const char *)src));
}

/* const char *
 * inet_ntop6(src, dst, size)
 *	convert IPv6 binary address into presentation (printable) format
 * author:
 *	Paul Vixie, 1996.
 */
#ifdef IPV6
static const char *
inet_ntop6(const unsigned char *src, char *dst, unsigned int size)
{
	/*
	 * Note that int32_t and int16_t need only be "at least" large enough
	 * to contain a value of the specified size.  On some systems, like
	 * Crays, there is no such thing as an integer variable with 16 bits.
	 * Keep this in mind if you think this function should have been coded
	 * to use pointer overlays.  All the world's not a VAX.
	 */
	char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
	struct { int base, len; } best = { 0, 0 }, cur = { 0, 0 };
	unsigned int words[IN6ADDRSZ / INT16SZ];
	int i;

	/*
	 * Preprocess:
	 *	Copy the input (bytewise) array into a wordwise array.
	 *	Find the longest run of 0x00's in src[] for :: shorthanding.
	 */
	memset(words, '\0', sizeof words);
	for (i = 0; i < IN6ADDRSZ; i += 2)
		words[i / 2] = (src[i] << 8) | src[i + 1];
	best.base = -1;
	cur.base = -1;
	for (i = 0; i < (IN6ADDRSZ / INT16SZ); i++) {
		if (words[i] == 0) {
			if (cur.base == -1)
				cur.base = i, cur.len = 1;
			else
				cur.len++;
		} else {
			if (cur.base != -1) {
				if (best.base == -1 || cur.len > best.len)
					best = cur;
				cur.base = -1;
			}
		}
	}
	if (cur.base != -1) {
		if (best.base == -1 || cur.len > best.len)
			best = cur;
	}
	if (best.base != -1 && best.len < 2)
		best.base = -1;

	/*
	 * Format the result.
	 */
	tp = tmp;
	for (i = 0; i < (IN6ADDRSZ / INT16SZ); i++) {
		/* Are we inside the best run of 0x00's? */
		if (best.base != -1 && i >= best.base &&
		    i < (best.base + best.len)) {
			if (i == best.base)
				*tp++ = ':';
			continue;
		}
		/* Are we following an initial run of 0x00s or any real hex? */
		if (i != 0)
			*tp++ = ':';
		/* Is this address an encapsulated IPv4? */
		if (i == 6 && best.base == 0 &&
		    (best.len == 6 || (best.len == 5 && words[5] == 0xffff))) {
			if (!inet_ntop4(src+12, tp, sizeof tmp - (tp - tmp)))
				return (NULL);
			tp += strlen(tp);
			break;
		}
		tp += SPRINTF((tp, "%x", words[i]));
	}
	/* Was it a trailing run of 0x00's? */
	if (best.base != -1 && (best.base + best.len) == 
	    (IN6ADDRSZ / INT16SZ))
		*tp++ = ':';
	*tp++ = '\0';

	/*
	 * Check for overflow, copy, and we're done.
	 */
	
	assert (tp - tmp >= 0);
	
	if ((unsigned int)(tp - tmp) > size) {
		return (NULL);
	}
	return strcpy(dst, tmp);
}
#endif

/* char *
 * inetntop(af, src, dst, size)
 *	convert a network format address to presentation format.
 * return:
 *	pointer to presentation format address (`dst'), or NULL (see errno).
 * author:
 *	Paul Vixie, 1996.
 */
const char *
inetntop(int af, const void *src, char *dst, unsigned int size)
{
  switch (af)
  {
    case AF_INET:
      return inet_ntop4(src, dst, size);
#ifdef IPV6
    case AF_INET6:
      if (IN6_IS_ADDR_V4MAPPED((const struct in6_addr *)src) ||
          IN6_IS_ADDR_V4COMPAT((const struct in6_addr *)src))
        return inet_ntop4((unsigned char *)&((const struct in6_addr *)src)->s6_addr[12], dst, size);
      else 
        return inet_ntop6(src, dst, size);
#endif
    default:
      return NULL;
  }
  /* NOTREACHED */
}

/*
 * strlcat and strlcpy were ripped from openssh 2.5.1p2
 * They had the following Copyright info: 
 *
 *
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright    
 *    notice, this list of conditions and the following disclaimer in the  
 *    documentation and/or other materials provided with the distribution. 
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED `AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
#ifndef HAVE_STRLCAT
size_t
strlcat(char *dst, const char *src, size_t siz)
{
  char *d = dst;
  const char *s = src;
  size_t n = siz, dlen;

  while (n-- != 0 && *d != '\0')
    d++;

  dlen = d - dst;
  n    = siz - dlen;

  if (n == 0)
    return(dlen + strlen(s));

  while (*s != '\0')
  {
    if (n != 1)
    {
      *d++ = *s;
      n--;
    }

    s++;
  }

  *d = '\0';
  return dlen + (s - src); /* count does not include NUL */
}
#endif

#ifndef HAVE_STRLCPY
size_t
strlcpy(char *dst, const char *src, size_t siz)
{
  char *d = dst;
  const char *s = src;
  size_t n = siz;

  /* Copy as many bytes as will fit */
  if (n != 0 && --n != 0)
  {
    do
    {
      if ((*d++ = *s++) == 0)
        break;
    } while (--n != 0);
  }

  /* Not enough room in dst, add NUL and traverse rest of src */
  if (n == 0)
  {
    if (siz != 0)
      *d = '\0'; /* NUL-terminate dst */
    while (*s++)
      ;
  }

  return s - src - 1; /* count does not include NUL */
}
#endif

pcre *
ircd_pcre_compile(const char *pattern, const char **errptr)
{
  int erroroffset = 0;
  int options = PCRE_EXTRA;

  assert(pattern);

  return pcre_compile(pattern, options, errptr, &erroroffset, NULL);
}

int
ircd_pcre_exec(const pcre *code, const char *subject)
{
  assert(code && subject);

  return pcre_exec(code, NULL, subject, strlen(subject), 0, 0, NULL, 0) < 0;
}

char *
stripws(char *txt)
{
  char *tmp;

  while (IsSpace(*txt))
    txt++;
  for (tmp = txt + strlen(txt) - 1; tmp >= txt && IsSpace(*tmp); tmp--)
    ;
  *(tmp + 1) = 0;
  return txt;
}
