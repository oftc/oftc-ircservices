/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  irc_string.h: A header for the ircd string functions.
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

#ifndef INCLUDED_libio_string_irc_string_h
#define INCLUDED_libio_string_irc_string_h

LIBIO_EXTERN int ircd_pcre_exec(const pcre *, const char *);
LIBIO_EXTERN pcre *ircd_pcre_compile(const char *, const char **);

/*
 * match - compare name with mask, mask may contain * and ? as wildcards
 * match - returns 1 on successful match, 0 otherwise
 *
 * match_esc - compare with support for escaping chars
 * match_chan - like match_esc with first character auto-escaped
 * match_cidr - compares u!h@addr with u!h@addr/cidr
 */
LIBIO_EXTERN int match(const char *, const char *);
LIBIO_EXTERN int match_esc(const char *, const char *);
LIBIO_EXTERN int match_chan(const char *, const char *);
LIBIO_EXTERN int match_cidr(const char *, const char *);

LIBIO_EXTERN int has_wildcards(const char *);

/*
 * collapse - collapse a string in place, converts multiple adjacent *'s 
 * into a single *.
 * collapse - modifies the contents of pattern 
 *
 * collapse_esc() - collapse with support for escaping chars
 */
LIBIO_EXTERN char *collapse(char *);
LIBIO_EXTERN char *collapse_esc(char *);

/*
 * NOTE: The following functions are NOT the same as strcasecmp
 * and strncasecmp! These functions use the Finnish (RFC1459)
 * character set. Do not replace!
 * 
 * irccmp - case insensitive comparison of s1 and s2
 */
LIBIO_EXTERN int irccmp(const char *, const char *);

/*
 * ircncmp - counted case insensitive comparison of s1 and s2
 */
LIBIO_EXTERN int ircncmp(const char *, const char *, size_t);

/* XXX
 * inetntop() 
 * portable interface for inet_ntop(), kludge; please use inet_ntop if possible
 * since inet_misc has a more conformant one
 */
LIBIO_EXTERN const char *inetntop(int, const void *, char *, unsigned int);
   
#ifndef HAVE_STRLCPY
LIBIO_EXTERN size_t strlcpy(char *, const char *, size_t);
#endif

#ifndef HAVE_STRLCAT
LIBIO_EXTERN size_t strlcat(char *, const char *, size_t);
#endif

#ifndef HAVE_SNPRINTF
LIBIO_EXTERN int snprintf(char *, size_t, const char *,...);
#endif

#ifndef HAVE_VSNPRINTF
LIBIO_EXTERN int vsnprintf(char *, size_t, const char *, va_list);
#endif

LIBIO_EXTERN const char *libio_basename(const char *);

/*
 * clean_string - cleanup control and high ascii characters
 * -Dianora
 */
LIBIO_EXTERN char *clean_string(char *, const unsigned char *, size_t);

LIBIO_EXTERN char *stripws(char *);

/*
 * strip_tabs - convert tabs to spaces
 * - jdc
 */
LIBIO_EXTERN void strip_tabs(char *, const char *, size_t);

LIBIO_EXTERN const char *myctime(time_t);

#define EmptyString(x) (!(x) || (*(x) == '\0'))

#ifndef HAVE_STRTOK_R
LIBIO_EXTERN char *strtoken(char **, char *, const char *);
#endif

/*
 * character macros
 */
LIBIO_EXTERN const unsigned char ToLowerTab[];
#define ToLower(c) (ToLowerTab[(unsigned char)(c)])

LIBIO_EXTERN const unsigned char ToUpperTab[];
#define ToUpper(c) (ToUpperTab[(unsigned char)(c)])

LIBIO_EXTERN const unsigned int CharAttrs[];

#define PRINT_C   0x001
#define CNTRL_C   0x002
#define ALPHA_C   0x004
#define PUNCT_C   0x008
#define DIGIT_C   0x010
#define SPACE_C   0x020
#define NICK_C    0x040
#define CHAN_C    0x080
#define KWILD_C   0x100
#define CHANPFX_C 0x200
#define USER_C    0x400
#define HOST_C    0x800
#define NONEOS_C 0x1000
#define SERV_C   0x2000
#define EOL_C    0x4000
#define MWILD_C  0x8000
#define VCHAN_C   0x10000

#define IsVisibleChanChar(c)   (CharAttrs[(unsigned char)(c)] & VCHAN_C)
#define IsHostChar(c)   (CharAttrs[(unsigned char)(c)] & HOST_C)
#define IsUserChar(c)   (CharAttrs[(unsigned char)(c)] & USER_C)
#define IsChanPrefix(c) (CharAttrs[(unsigned char)(c)] & CHANPFX_C)
#define IsChanChar(c)   (CharAttrs[(unsigned char)(c)] & CHAN_C)
#define IsKWildChar(c)  (CharAttrs[(unsigned char)(c)] & KWILD_C)
#define IsMWildChar(c)  (CharAttrs[(unsigned char)(c)] & MWILD_C)
#define IsNickChar(c)   (CharAttrs[(unsigned char)(c)] & NICK_C)
#define IsServChar(c)   (CharAttrs[(unsigned char)(c)] & (NICK_C | SERV_C))
#define IsCntrl(c)      (CharAttrs[(unsigned char)(c)] & CNTRL_C)
#define IsAlpha(c)      (CharAttrs[(unsigned char)(c)] & ALPHA_C)
#define IsSpace(c)      (CharAttrs[(unsigned char)(c)] & SPACE_C)
#define IsLower(c)      (IsAlpha((c)) && ((unsigned char)(c) > 0x5f))
#define IsUpper(c)      (IsAlpha((c)) && ((unsigned char)(c) < 0x60))
#define IsDigit(c)      (CharAttrs[(unsigned char)(c)] & DIGIT_C)
#define IsXDigit(c) (IsDigit(c) || ('a' <= (c) && (c) <= 'f') || \
        ('A' <= (c) && (c) <= 'F'))
#define IsAlNum(c) (CharAttrs[(unsigned char)(c)] & (DIGIT_C | ALPHA_C))
#define IsPrint(c) (CharAttrs[(unsigned char)(c)] & PRINT_C)
#define IsAscii(c) ((unsigned char)(c) < 0x80)
#define IsGraph(c) (IsPrint((c)) && ((unsigned char)(c) != 0x32))
#define IsPunct(c) (!(CharAttrs[(unsigned char)(c)] & \
                                           (CNTRL_C | ALPHA_C | DIGIT_C)))

#define IsNonEOS(c) (CharAttrs[(unsigned char)(c)] & NONEOS_C)
#define IsEol(c) (CharAttrs[(unsigned char)(c)] & EOL_C)

#endif /* INCLUDED_libio_string_irc_string_h */
