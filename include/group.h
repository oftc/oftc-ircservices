/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  group.h group related header file
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
 *  $Id: msg.h 957 2007-05-07 16:52:26Z stu $
 */

#ifndef INCLUDED_group_h
#define INCLUDED_group_h

typedef struct
{
  dlink_node node;

  unsigned int id;
  char name[GROUPLEN+1];
  char *desc;
  char *email;
  char *url;
  unsigned char priv;
  time_t reg_time;
} Group;

Group* group_find(const char *);
int group_register(Group *);
int group_delete(Group *);

char *group_name_from_id(int, int);
int group_id_from_name(const char *);

int group_save(Group *);

int group_link_list(unsigned int, dlink_list *);
void group_link_list_free(dlink_list *);

int group_chan_list(unsigned int, dlink_list *);
void group_chan_list_free(dlink_list *);

int group_list_all(dlink_list *);
void group_list_all_free(dlink_list *);

int group_list_regular(dlink_list *);
void group_list_regular_free(dlink_list *);

Group *group_new();
inline void group_free(Group *);

/* Group getters */

unsigned int group_get_id(Group *);
inline const char *group_get_name(Group *);
const char *group_get_email(Group *);
const char *group_get_url(Group *);
const char *group_get_desc(Group *);
unsigned char group_get_priv(Group *);
time_t group_get_reg_time(Group *);

/* Group setters */
inline int group_set_id(Group *, unsigned int);
inline int group_set_name(Group *, const char *);
inline int group_set_desc(Group *, const char *);
inline int group_set_url(Group *, const char *);
inline int group_set_email(Group *, const char *);
inline int group_set_priv(Group *, unsigned char);
inline int group_set_reg_time(Group *, time_t);
#endif
