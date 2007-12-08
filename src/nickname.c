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

  results = db_execute(GET_FULL_NICK, &error, "s", nickname);
  if(error)
  {
    ilog(L_CRIT, "Database error %d trying to find nickname %s", error,
        nickname);
    return NULL;
  }

  if(results->row_count == 0)
  {
    ilog(L_DEBUG, "Nickname %s not found.", nickname);
    db_free_result(results);
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

  nick = db_execute_scalar(GET_FORBID, &error, "s", nickname);
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
    nick = db_execute_scalar(GET_NICK_FROM_ACCID, &error, "i", id);
  else
    nick = db_execute_scalar(GET_NICK_FROM_NICKID, &error, "i", id);
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
    ret = db_execute_scalar(GET_ACCID_FROM_NICK, &error, "s", nick);
  else
    ret = db_execute_scalar(GET_NICKID_FROM_NICK, &error, "s", nick);

  id = atoi(ret);
  MyFree(ret);

  return id;
}

int
nickname_register(struct Nick *nick)
{
  int accid, nickid, ret, tmp;

  db_begin_transaction();

  nickid = db_nextid("nickname", "id");
  if(nickid == -1)
    goto failure;

  ret = db_execute_nonquery(INSERT_ACCOUNT, "isssi", nickid, nick->pass, 
      nick->salt, nick->email, CurrentTime);

  if(ret == -1)
    goto failure;

  accid = db_insertid("account", "id");
  if(accid == -1)
    goto failure;

  ret = db_execute_nonquery(INSERT_NICK, "isiii", nickid, nick->nick, accid,
      CurrentTime, CurrentTime);

  if(ret == -1)
    goto failure;

  tmp = db_insertid("nickname", "id");
  if(tmp == -1)
    goto failure;

  assert(tmp == nickid);

  ret = db_execute_nonquery(SET_NICK_MASTER, "ii", nickid, accid);
  if(ret == -1)
    goto failure;

  if(!db_commit_transaction())
    goto failure;

  nick->id = accid;
  nick->nickid = nick->pri_nickid = nickid;
  nick->nick_reg_time = nick->reg_time = CurrentTime;

  return TRUE;
  
failure:
  db_rollback_transaction();
  return FALSE;
}

int
nickname_delete(struct Nick *nick)
{
  int newid, ret, error;

  db_begin_transaction();
  
  if(nick->nickid == nick->pri_nickid)
  {
    char *tmp = db_execute_scalar(GET_NEW_LINK, &error, "ii", 
        nick->id, nick->nickid);
    if(error || tmp == NULL)
    {
      ret = db_execute_nonquery(DELETE_NICK, "i", nick->nickid);
      if(ret == -1)
        goto failure;
      ret = db_execute_nonquery(DELETE_ACCOUNT_CHACCESS, "i", nick->id);
      if(ret == -1)
        goto failure;
      ret = db_execute_nonquery(DELETE_ACCOUNT, "i", nick->id);
      if(ret == -1)
        goto failure;
    }
    else
    {
      newid = atoi(tmp);
      MyFree(tmp);
      ret = db_execute_nonquery(SET_NICK_MASTER, "ii", newid, nick->id);
      if(ret == -1)
        goto failure;
    }
  }
  else
  {
    ret = db_execute_nonquery(DELETE_NICK, "i", nick->nickid);
    if(ret == -1)
      goto failure;
  }

  if(!db_commit_transaction())
    return FALSE;

  execute_callback(on_nick_drop_cb, nick->id, nick->nickid, nick->pri_nickid);
  return TRUE;
failure:
  db_rollback_transaction();
  return FALSE;
}

int
nickname_forbid(const char *nick)
{
  struct Nick *nickname;
  int ret;

  db_begin_transaction();

  if((nickname = nickname_find(nick)) != NULL)
  {
    nickname_delete(nickname);
    free_nick(nickname);
  }

  ret = db_execute_nonquery(INSERT_FORBID, "s", nick);

  if(ret == -1)
  {
    db_rollback_transaction();
    return FALSE;
  }

  return db_commit_transaction();
}
