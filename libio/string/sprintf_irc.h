/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  sprintf_irc.h: The irc sprintf header.
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
 *  $Id: sprintf_irc.h 153 2005-10-17 21:20:34Z adx $
 */

/*=============================================================================
 * Prototypes
 */

LIBIO_EXTERN int vsprintf_irc(char *str, const char *format, va_list);

/* XXX NOT USED AND NOT DEFINED */
LIBIO_EXTERN int vsnprintf_irc(char *, int, const char*, va_list);

/* old */
/* LIBIO_EXTERN int ircsprintf(char *str, char *format, ...); */
/* */

/*
 * ircsprintf - optimized sprintf
 */
#ifdef __GNUC__
LIBIO_EXTERN int ircsprintf(char*, const char*, ...)
               __attribute__ ((format(printf, 2, 3)));
#else
LIBIO_EXTERN int ircsprintf(char *str, const char *format, ...);
#endif
