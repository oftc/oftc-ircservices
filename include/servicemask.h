/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
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
 */

#ifndef INCLUDED_servicemask_h
#define INCLUDED_servicemask_h

#include "stdinc.h"

enum ServiceMaskType
{
  AKICK_MASK = 0,
  AKILL_MASK,
  INVEX_MASK,
  EXCPT_MASK,
  QUIET_MASK,
};

struct ServiceMask
{
  unsigned int id;
  unsigned int type;
  unsigned int channel;
  unsigned int target;
  unsigned int setter;
  char *mask;
  char *reason;
  time_t time_set;
  time_t duration;
};

void free_servicemask(struct ServiceMask *);

int servicemask_add_akick_target(unsigned int, unsigned int, unsigned int, unsigned int,
  unsigned int, const char *);
int servicemask_add_akick(const char *, unsigned int, unsigned int, unsigned int,
  unsigned int, const char *);
int servicemask_add_invex(const char *, unsigned int, unsigned int, unsigned int,
  unsigned int, const char *);
int servicemask_add_quiet(const char *, unsigned int, unsigned int, unsigned int,
  unsigned int, const char *);
int servicemask_add_excpt(const char *, unsigned int, unsigned int, unsigned int,
  unsigned int, const char *);

int servicemask_remove_akick_target(unsigned int, const char *);
int servicemask_remove_akick(unsigned int, const char *);
int servicemask_remove_invex(unsigned int, const char *);
int servicemask_remove_quiet(unsigned int, const char *);
int servicemask_remove_excpt(unsigned int, const char *);

void servicemask_list_free(dlink_list *);
int servicemask_list_akick(unsigned int, dlink_list *);
int servicemask_list_invex(unsigned int, dlink_list *);
int servicemask_list_excpt(unsigned int, dlink_list *);
int servicemask_list_quiet(unsigned int, dlink_list *);

#endif
