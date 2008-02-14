/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  dbchannel.c - channel related functions(on the database side)
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
#include "dbm.h"
#include "nickserv.h"
#include "dbchannel.h"
#include "chanserv.h"
#include "parse.h"
#include "language.h"
#include "interface.h"
#include "msg.h"

static struct RegChannel *
row_to_dbchannel(row_t *row)
{
  struct RegChannel *channel;

  channel = MyMalloc(sizeof(struct RegChannel));

  channel->id = atoi(row->cols[0]);
  strlcpy(channel->channel, row->cols[1], sizeof(channel->channel));
  DupString(channel->description, row->cols[2]);
  DupString(channel->entrymsg, row->cols[3]);
  channel->regtime = atoi(row->cols[4]);
  channel->priv = atoi(row->cols[5]);
  channel->restricted = atoi(row->cols[6]);
  channel->topic_lock = atoi(row->cols[7]);
  channel->verbose = atoi(row->cols[8]);
  channel->autolimit = atoi(row->cols[9]);
  channel->expirebans = atoi(row->cols[10]);
  channel->floodserv = atoi(row->cols[11]);
  channel->autoop = atoi(row->cols[12]);
  channel->autovoice = atoi(row->cols[13]);
  channel->leaveops = atoi(row->cols[14]);
  if(row->cols[15] != NULL)
    DupString(channel->url, row->cols[15]);
  if(row->cols[16] != NULL)
    DupString(channel->email, row->cols[16]);
  if(row->cols[17] != NULL)
    DupString(channel->topic, row->cols[17]);
  if(row->cols[18] != NULL)
    DupString(channel->mlock, row->cols[18]);
  channel->expirebans_lifetime = atoi(row->cols[19]);

  return channel;
}

struct RegChannel *
dbchannel_find(const char *name)
{
  struct RegChannel *channel;
  result_set_t *results;
  int error;

  results = db_execute(GET_FULL_CHAN, &error, "s", name);
  if(results == NULL || error)
    return NULL;

  if(results->row_count == 0)
  {
    ilog(L_DEBUG, "dbchannel_find: Channel %s not found", name);
    db_free_result(results);
    return NULL;
  }

  assert(results->row_count == 1);

  channel = row_to_dbchannel(&results->rows[0]);

  db_free_result(results);
  return channel;
}

int
dbchannel_delete(struct RegChannel *channel)
{
  int ret;

  ret = db_execute_nonquery(DELETE_CHAN, "i", channel->id);
  if(ret == -1)
    return FALSE;

  execute_callback(on_chan_drop_cb, channel->channel);

  return TRUE;
}

int
dbchannel_forbid(const char *name)
{
  int ret;
  struct RegChannel *chan;

  db_begin_transaction();

  if((chan = dbchannel_find(name)) != NULL)
  {
    if(!dbchannel_delete(chan))
      goto failure;
    free_regchan(chan);
  }

  ret = db_execute_nonquery(INSERT_CHAN_FORBID, "s", name);
  if(ret == -1)
    goto failure;

  return db_commit_transaction();

failure:
  db_rollback_transaction();
  return FALSE;
}

int
dbchannel_delete_forbid(const char *name)
{
  int ret;

  ret = db_execute_nonquery(DELETE_CHAN_FORBID, "s", name);
  if(ret == -1)
    return FALSE;

  return TRUE;
}

int
dbchannel_register(struct RegChannel *channel, struct Nick *founder)
{
  int ret;

  db_begin_transaction();

  ret = db_execute_nonquery(INSERT_CHAN, "ssii", channel->channel, 
      channel->description, CurrentTime, CurrentTime);

  if(ret == -1)
    goto failure;

  channel->id = db_insertid("channel", "id");
  if(channel->id == -1)
    goto failure;

  ret = db_execute_nonquery(INSERT_CHANACCESS, "iii", founder->id, channel->id, 
      MASTER_FLAG);

  if(ret == -1)
    goto failure;

  return db_commit_transaction();

failure:
  db_rollback_transaction();
  return FALSE;
}

int
dbchannel_is_forbid(const char *channel)
{
  int error;
  char *ret = db_execute_scalar(GET_CHAN_FORBID, &error, "s", channel);
  if(error)
  {
    ilog(L_CRIT, "dbchannel_is_forbid: Database error %d", error);
    return FALSE;
  }

  if(ret == NULL)
    return FALSE;

  MyFree(ret);
  return TRUE;
}
