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
  if(row->cols[6] != NULL)
    DupString(nick->url, row->cols[6]);
  DupString(nick->email, row->cols[7]);
  if(row->cols[8] != NULL)
    strlcpy(nick->cloak, row->cols[8], sizeof(nick->cloak));
  nick->enforce = atoi(row->cols[9]);
  nick->secure = atoi(row->cols[10]);
  nick->verified = atoi(row->cols[11]);
  nick->cloak_on = atoi(row->cols[12]);
  nick->admin = atoi(row->cols[13]);
  nick->email_verified = atoi(row->cols[14]);
  nick->priv = atoi(row->cols[15]);
  nick->language = atoi(row->cols[16]);
  if(row->cols[17] != NULL)
    DupString(nick->last_host, row->cols[17]);
  if(row->cols[18] != NULL)
    DupString(nick->last_realname, row->cols[18]);
  if(row->cols[19] != NULL)
    DupString(nick->last_quit, row->cols[19]);
  if(row->cols[20] != NULL)
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
    if(error)
      goto failure;
    if(tmp == NULL)
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

int 
nickname_delete_forbid(const char *nick)
{
  int ret;

  ret = db_execute_nonquery(DELETE_FORBID, "s", nick);
  if(ret == -1)
    return FALSE;

  return TRUE;
}

int
nickname_set_master(struct Nick *nick, const char *master)
{
  int newid, ret;

  newid = nickname_id_from_nick(master, FALSE);

  if(newid == -1)
    return FALSE;

  ret = db_execute_nonquery(SET_NICK_MASTER, "ii", newid, nick->id);

  return (ret != -1);
}

int
nickname_link(struct Nick *master, struct Nick *child)
{
  int ret;

  db_begin_transaction();

  ret = db_execute_nonquery(SET_NICK_LINK, "ii", master->id, child->id);
  if(ret == -1)
    goto failure;

  ret = db_execute_nonquery(DELETE_DUPLICATE_CHACCESS, "iiii", child->id,
      master->id, master->id, child->id);
  if(ret == -1)
    goto failure;

  ret = db_execute_nonquery(MERGE_CHACCESS, "ii", master->id, child->id);
  if(ret == -1)
    goto failure;

  ret = db_execute_nonquery(DELETE_ACCOUNT, "i", child->id);
  if(ret == -1)
    goto failure;

  return db_commit_transaction();

failure:
  db_rollback_transaction();
  return FALSE;
}

int
nickname_unlink(struct Nick *nick)
{
  int ret, newid, new_nickid, error;
  char *tmp;

  db_begin_transaction();

  ret = db_execute_nonquery(INSERT_NICK_CLONE, "i", nick->id);
  if(ret == -1)
    goto failure;

  newid = db_insertid("account", "id");
  if(newid == -1)
    goto failure;

  if(nick->pri_nickid != nick->nickid)
  {
    ret = db_execute_nonquery(SET_NICK_LINK_EXCLUDE, "iii", newid,
        nick->id, nick->nickid);
    if(ret == -1)
      goto failure;
  }

  tmp = db_execute_scalar(GET_NEW_LINK, &error, "ii", 
      nick->id, nick->pri_nickid);
  if(error)
    goto failure;
  new_nickid = atoi(tmp);
  MyFree(tmp);

  if(nick->nickid == nick->pri_nickid)
  {
    ret = db_execute_nonquery(SET_NICK_MASTER, "ii", nick->pri_nickid,
        nick->id);
    if(ret == -1)
      goto failure;
    
    ret = db_execute_nonquery(SET_NICK_MASTER, "ii", new_nickid, newid);
    if(ret == -1)
      goto failure;

    ret = db_execute_nonquery(SET_NICK_LINK_EXCLUDE, "iii", newid,
        nick->id, new_nickid);
    if(ret == -1)
      goto failure;
  }
  else
  {
    ret = db_execute_nonquery(SET_NICK_MASTER, "ii", nick->nickid, newid);
    if(ret == -1)
      goto failure;
  }

  if(!db_commit_transaction())
    return -1;

  return newid;
failure:
  db_rollback_transaction();
  return -1;
}

int
nickname_save(struct Nick *nick)
{
  int ret;

  db_begin_transaction();

  ret = db_execute_nonquery(SET_NICK_LAST_SEEN, "ii", nick->nickid, 
      nick->last_seen);
  if(ret == -1)
  {
    db_rollback_transaction();
    return FALSE;
  }

  ret = db_execute_nonquery(SAVE_NICK, "sssbbbbbbbisssii", nick->url,
      nick->email, nick->cloak, nick->enforce, nick->secure,
      nick->verified, nick->cloak_on, nick->admin, nick->email_verified,
      nick->priv, nick->language, nick->last_host, nick->last_realname,
      nick->last_quit, nick->last_quit_time, nick->id);
  if(ret == -1)
  {
    db_rollback_transaction();
    return FALSE;
  }

  return db_commit_transaction();
}
