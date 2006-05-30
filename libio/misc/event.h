/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  event.h: The ircd event header.
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
 *  $Id: event.h 153 2005-10-17 21:20:34Z adx $
 */

/*
 * How many event entries we need to allocate at a time in the block
 * allocator. 16 should be plenty at a time.
 */
#define	MAX_EVENTS	50

typedef void EVH(void *);

/* The list of event processes */
struct ev_entry
{
  EVH *func;
  void *arg;
  const char *name;
  time_t frequency;
  time_t when;
  int active;
};

LIBIO_EXTERN const char *last_event_ran;
LIBIO_EXTERN struct ev_entry event_table[];

LIBIO_EXTERN void eventAdd(const char *, EVH *, void *, time_t);
LIBIO_EXTERN void eventAddIsh(const char *, EVH *, void *, time_t);
LIBIO_EXTERN void eventRun(void);
LIBIO_EXTERN time_t eventNextTime(void);
#ifdef IN_MISC_C
extern void eventInit(void);
#endif
LIBIO_EXTERN void eventDelete(EVH *, void *);
LIBIO_EXTERN void set_back_events(time_t);
