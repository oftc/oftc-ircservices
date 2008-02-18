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
#include "mqueue.h"
#include "regchptr.h"

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
    dbchannel_free(chan);
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

int
dbchannel_list_all(dlink_list *list)
{
  return db_string_list(GET_CHANNELS_OPER, list);
}

void
dbchannel_list_all_free(dlink_list *list)
{
  db_string_list_free(list);
}

int
dbchannel_list_regular(dlink_list *list)
{
  return db_string_list(GET_CHANNELS, list);
}

void
dbchannel_list_regular_free(dlink_list *list)
{
  db_string_list_free(list);
}

int
dbchannel_list_forbid(dlink_list *list)
{
  return db_string_list(GET_CHANNEL_FORBID_LIST, list);
}

void
dbchannel_list_forbid_free(dlink_list *list)
{
  db_string_list_free(list);
}

int
dbchannel_masters_list(unsigned int id, dlink_list *list)
{
  return db_string_list_by_id(GET_CHAN_MASTERS, list, id);
}

void
dbchannel_masters_list_free(dlink_list *list)
{
  db_string_list_free(list);
}

int
dbchannel_masters_count(unsigned int id, int *count)
{
  int error;
  *count = atoi(db_execute_scalar(GET_CHAN_MASTER_COUNT, &error, "i", id));
  if(error)
  {
    *count = -1;
    return FALSE;
  }

  return TRUE;
}

DBChannel
dbchannel_new()
{
  return MyMalloc(sizeof(struct RegChannel));
}

/* Member getters */
dlink_node
dbchannel_get_node(DBChannel this)
{
  return this->node;
}

unsigned int
dbchannel_get_id(DBChannel this)
{
  return this->id;
}

time_t
dbchannel_get_regtime(DBChannel this)
{
  return this->regtime;
}

const char *
dbchannel_get_channel(DBChannel this)
{
  return this->channel;
}

const char *
dbchannel_get_description(DBChannel this)
{
  return this->description;
}

const char *
dbchannel_get_entrymsg(DBChannel this)
{
  return this->entrymsg;
}

const char *
dbchannel_get_url(DBChannel this)
{
  return this->url;
}

const char *
dbchannel_get_email(DBChannel this)
{
  return this->email;
}

const char *
dbchannel_get_topic(DBChannel this)
{
  return this->topic;
}

const char *
dbchannel_get_mlock(DBChannel this)
{
  return this->mlock;
}

char dbchannel_get_priv(DBChannel this)
{
  return this->priv;
}

char
dbchannel_get_restricted(DBChannel this)
{
  return this->restricted;
}

char
dbchannel_get_topic_lock(DBChannel this)
{
  return this->topic_lock;
}

char
dbchannel_get_verbose(DBChannel this)
{
  return this->verbose;
}

char
dbchannel_get_autolimit(DBChannel this)
{
  return this->autolimit;
}

char
dbchannel_get_expirebans(DBChannel this)
{
  return this->expirebans;
}

char
dbchannel_get_floodserv(DBChannel this)
{
  return this->floodserv;
}

char
dbchannel_get_autoop(DBChannel this)
{
  return this->autoop;
}

char
dbchannel_get_autovoice(DBChannel this)
{
  return this->autovoice;
}

char
dbchannel_get_leaveops(DBChannel this)
{
  return this->leaveops;
}

unsigned int
dbchannel_get_expirebans_lifetime(DBChannel this)
{
  return this->expirebans_lifetime;
}

/* FloodServ */
struct MessageQueue **
dbchannel_get_flood_hash(DBChannel this)
{
  return this->flood_hash;
}

dlink_list *
dbchannel_get_flood_list(DBChannel this)
{
  return &this->flood_list;
}

struct MessageQueue *
dbchannel_get_gqueue(DBChannel this)
{
  return this->gqueue;
}

/* Memer setters */
void
dbchannel_set_node(DBChannel this, dlink_node node)
{
  this->node = node;
}

void
dbchannel_set_id(DBChannel this, unsigned int id)
{
  this->id = id;
}

void
dbchannel_set_regtime(DBChannel this, time_t regtime)
{
  this->regtime = regtime;
}

void
dbchannel_set_channel(DBChannel this, const char *name)
{
  strlcpy(this->channel, name, sizeof(this->channel));
}

void
dbchannel_set_description(DBChannel this, const char *description)
{
  MyFree(this->description);
  DupString(this->description, description);
  db_execute_nonquery(SET_CHAN_DESC, "s", this->description);
}

void
dbchannel_set_entrymsg(DBChannel this, const char *entrymsg)
{
  MyFree(this->entrymsg);
  DupString(this->entrymsg, entrymsg);
  db_execute_nonquery(SET_CHAN_ENTRYMSG, "s", this->entrymsg);
}

void
dbchannel_set_url(DBChannel this, const char *url)
{
  MyFree(this->url);
  DupString(this->url, url);
  db_execute_nonquery(SET_CHAN_URL, "s", this->url);
}

void
dbchannel_set_email(DBChannel this, const char *email)
{
  MyFree(this->email);
  DupString(this->email, email);
  db_execute_nonquery(SET_CHAN_EMAIL, "s", this->email);
}

void dbchannel_set_topic(DBChannel this, const char *topic)
{
  MyFree(this->topic);
  DupString(this->topic, topic);
  db_execute_nonquery(SET_CHAN_TOPIC, "s", this->topic);
}

void
dbchannel_set_mlock(DBChannel this, const char *mlock)
{
  MyFree(this->mlock);
  DupString(this->mlock, mlock);
  db_execute_nonquery(SET_CHAN_MLOCK, "s", this->mlock);
}

void
dbchannel_set_priv(DBChannel this, char priv)
{
  this->priv = priv;
  db_execute_nonquery(SET_CHAN_PRIVATE, "b", priv);
}

void
dbchannel_set_restricted(DBChannel this, char restricted)
{
  this->restricted = restricted;
  db_execute_nonquery(SET_CHAN_RESTRICTED, "b", restricted);
}

void
dbchannel_set_topic_lock(DBChannel this, char topic_lock)
{
  this->topic_lock = topic_lock;
  db_execute_nonquery(SET_CHAN_TOPICLOCK, "b", topic_lock);
}

void
dbchannel_set_verbose(DBChannel this, char verbose)
{
  this->verbose = verbose;
  db_execute_nonquery(SET_CHAN_VERBOSE, "b", verbose);
}

void
dbchannel_set_autolimit(DBChannel this, char autolimit)
{
  this->autolimit = autolimit;
  db_execute_nonquery(SET_CHAN_AUTOLIMIT, "b", autolimit);
}

void
dbchannel_set_expirebans(DBChannel this, char expirebans)
{
  this->expirebans = expirebans;
  db_execute_nonquery(SET_CHAN_EXPIREBANS, "b", expirebans);
}

void
dbchannel_set_floodserv(DBChannel this, char floodserv)
{
  this->floodserv = floodserv;
  db_execute_nonquery(SET_CHAN_FLOODSERV, "b", floodserv);
}

void
dbchannel_set_autoop(DBChannel this, char autoop)
{
  this->autoop = autoop;
  db_execute_nonquery(SET_CHAN_AUTOOP, "b", autoop);
}

void
dbchannel_set_autovoice(DBChannel this, char autovoice)
{
  this->autovoice = autovoice;
  db_execute_nonquery(SET_CHAN_AUTOVOICE, "b", autovoice);
}

void
dbchannel_set_leaveops(DBChannel this, char leaveops)
{
  this->leaveops = leaveops;
  db_execute_nonquery(SET_CHAN_LEAVEOPS, "b", leaveops);
}

void
dbchannel_set_expirebans_lifetime(DBChannel this, unsigned int time)
{
  this->expirebans_lifetime = time;
  db_execute_nonquery(SET_EXPIREBANS_LIFETIME, "i", time);
}

/* FloodServ */
void
dbchannel_set_flood_hash(DBChannel this, struct MessageQueue ** queue)
{
  this->flood_hash = queue;
}

void
dbchannel_set_gqueue(DBChannel this, struct MessageQueue *queue)
{
  this->gqueue = queue;
}

void
dbchannel_free(DBChannel this)
{
  MyFree(this->description);
  MyFree(this->entrymsg);
  MyFree(this->url);
  MyFree(this->email);
  MyFree(this->topic);
  MyFree(this->mlock);
  mqueue_hash_free(this->flood_hash, &this->flood_list);
  this->flood_hash = NULL;
  mqueue_free(this->gqueue);
  this->gqueue = NULL;
  MyFree(this);
}

