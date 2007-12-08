/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  nickname.c - nickname related functions
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
 *  $Id: dbm.c 1260 2007-12-07 08:53:17Z swalsh $
 */

#include "stdinc.h"

static struct Nick *
row_to_nickname(row_t *row)
{
  struct Nick *nick = MyMalloc(sizeof(struct Nick));

  nick->id = atoi(row->cols[0]);
  nick->pri_nickid = atoi(row->cols[1]);
  nick->nickid = atoi(row->cols[2]);
  strlcpy(nick->nick, row->cols[3], sizeof(nick->nick));
  strlcpy(nick->pass, row->cols[4], sizeof(nick->pass));
  strlcpy(nick->salt, row->cols[5], sizeof(nick->salt));
  DupString(nick->url, row->cols[6]);
  DupString(nick->email, row->cols[7]);
  strlcpy(nick->cloak, row->cols[8], sizeof(nick->cloak));
  nick->enforce = atoi(row->cols[9]);
  nick->secure = atoi(row->cols[10]);
  nick->verified = atoi(row->cols[11]);
  nick->cloak_on = atoi(row->cols[12]);
  nick->admin = atoi(row->cols[13]);
  nick->email_verified = atoi(row->cols[14]);
  nick->priv = atoi(row->cols[15]);
  nick->language = atoi(row->cols[16]);
  DupString(nick->last_host, row->cols[17]);
  DupString(nick->last_realname, row->cols[18]);
  DupString(nick->last_quit, row->cols[19]);
  nick->last_quit_time = atoi(row->cols[20]);
  nick->reg_time = atoi(row->cols[21]);
  nick->nick_reg_time = atoi(row->cols[22]);
  nick->last_seen = atoi(row->cols[23]);
 
  return nick;
}

struct Nick *
nickname_find(const char *nickname)
{
  result_set_t *results;
  struct Nick *nick;
  int error;

  results = db_execute(GET_FULL_NICK, 1, &error, nickname);
  if(error)
  {
    ilog(L_CRIT, "Database error %d trying to find nickname %s", error,
        nickname);
    return NULL;
  }

  if(results == NULL)
  {
    ilog(L_DEBUG, "Nickname %s not found.", nickname);
    return NULL;
  }

  nick = row_to_nickname(&results->rows[0]);
 
  return nick;
}

int 
nickname_is_forbid(const char *nickname)
{
  char *nick;
  int error;

  nick = db_execute_scalar(GET_FORBID, 1, &error, nickname);
  if(error)
  {
    ilog(L_CRIT, "Database error %d trying to test forbidden nickname %s", 
        error, nickname);
    return 0;
  }

  if(nick == NULL)
  {
    MyFree(nick);
    return 0;
  }

  MyFree(nick);
  return 1;
}

char *
nickname_nick_from_id(int id, int is_accid)
{
  char *nick;
  int error;

  if(is_accid)
    nick = db_execute_scalar(GET_NICK_FROM_ACCID, 1, &error, id);
  else
    nick = db_execute_scalar(GET_NICK_FROM_NICKID, 1, &error, id);
  if(error || nick == NULL)
    return NULL;

  return nick;
}

int
nickname_id_from_nick(const char *nick, int is_accid)
{
  int id, error;
  char *ret;

  if(is_accid)
    ret = db_execute_scalar(GET_ACCID_FROM_NICK, 1, &error, nick);
  else
    ret = db_execute_scalar(GET_NICKID_FROM_NICK, 1, &error, nick);

  id = atoi(ret);
  MyFree(ret);

  return id;
}
