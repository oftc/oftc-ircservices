/*
 *  oftc-ircservices: an exstensible and flexible IRC Services package
 *  dbm.c: The database manager
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
 *  $Id$
 */

#include "stdinc.h"
#include <dbi/dbi.h>
#include "conf/conf.h"

static dbi_result db_query(const char *, ...);
static unsigned int db_get_id_from_nick(const char *);

void
init_db()
{
  int num_drivers = dbi_initialize(NULL);

  memset(&Database, 0, sizeof(Database));

  if(num_drivers >= 0)
    printf("db: loaded: %d drivers available.\n", num_drivers);
  else
  {
    printf("db: Error loading db.\n");
    exit(-1);
  }

  printf("db: version: %s\n", dbi_version());
}

void
db_load_driver()
{
  Database.conn = dbi_conn_new(Database.driver);
  if(Database.conn == NULL)
  {
    printf("db: Error loading database driver %s\n", Database.driver);
    exit(-1);
  }

  printf("db: Driver %s loaded\n", Database.driver);

  dbi_conn_set_option(Database.conn, "username", Database.username);
  dbi_conn_set_option(Database.conn, "password", Database.password);
  dbi_conn_set_option(Database.conn, "dbname", Database.dbname);

  if(dbi_conn_connect(Database.conn) < 0)
  {
    const char *error;
    dbi_conn_error(Database.conn, &error);
    printf("db: Failed to connect to database %s\n", error);
  }
  else
    printf("db: Database connection succeeded.\n");

  Database.driv = dbi_conn_get_driver(Database.conn);
}

dbi_result
db_query(const char *pattern, ...)
{
  va_list args;
  char *buffer;
  dbi_result result;
  int len;

  va_start(args, pattern);
  len = vasprintf(&buffer, pattern, args);
  va_end(args);
  if(len == -1)
    return NULL;

  printf("db_query: %s\n", buffer);
  result = dbi_conn_query(Database.conn, buffer);
  if(result == NULL)
  {
    const char *error;
    dbi_conn_error(Database.conn, &error);
    printf("db_query: Failed: %s\n", error);
  }
  MyFree(buffer);
  return result;
}

struct Nick *
db_find_nick(const char *nick)
{
  dbi_result result, link_result;
  char *escnick = NULL;
  struct Nick *nick_p;
  char *retnick, *retpass, *retcloak, *retsalt;
  unsigned int id;
  
  assert(nick != NULL);

  if(dbi_driver_quote_string_copy(Database.driv, nick, &escnick) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    return NULL;
  }
  
  result = db_query("SELECT id, nick, password, salt, email, cloak, last_quit, "
      "last_quit_time, reg_time, last_seen, last_used, flags, language "
      "FROM %s WHERE nick=%s", "nickname", escnick);
  MyFree(escnick);

  if(result == NULL)
    return NULL;

  if(dbi_result_get_numrows(result) == 0)
  {
    printf("db: Nick %s not found\n", nick);
    return NULL;
  }

  dbi_result_first_row(result);
  dbi_result_get_fields(result, "id.%ui", &id);

  link_result = db_query("SELECT nick_id FROM nickname_links WHERE link_id=%d", 
      id);
  if(link_result != NULL && dbi_result_get_numrows(link_result) != 0)
  {
    dbi_result_free(result);
    dbi_result_first_row(link_result);
    dbi_result_get_fields(link_result, "nick_id.%ui", &id);
    dbi_result_free(link_result);
    result = db_query("SELECT id, nick, password, salt, email, cloak, last_quit, "
      "last_quit_time, reg_time, last_seen, last_used, flags, language "
      "FROM %s WHERE id=%d", "nickname", id);
    if(result == NULL)
      return NULL;
    if(dbi_result_get_numrows(result) == 0)
    {
      printf("WTF: Master nick not found!!! %s %d\n", nick, id);
      dbi_result_free(result);
      return NULL;
    }
  }

  nick_p = MyMalloc(sizeof(struct Nick));
  dbi_result_first_row(result);
  dbi_result_get_fields(result, "id.%ui nick.%S password.%S salt.%S email.%S "
      "cloak.%S last_quit.%S last_quit_time.%l reg_time.%l last_seen.%l "
      "last_used.%l flags.%ui language.%ui",
      &nick_p->id, &retnick, &retpass, &retsalt, &nick_p->email, &retcloak, 
      &nick_p->last_quit, &nick_p->last_quit_time, &nick_p->reg_time, 
      &nick_p->last_seen, &nick_p->last_used, &nick_p->flags, 
      &nick_p->language);

  strlcpy(nick_p->nick, retnick, sizeof(nick_p->nick));
  strlcpy(nick_p->pass, retpass, sizeof(nick_p->pass));
  strlcpy(nick_p->salt, retsalt, sizeof(nick_p->salt));
  strlcpy(nick_p->cloak, retcloak, sizeof(nick_p->cloak));

  printf("db_find_nick: Found nick %s(asked for %s)\n", nick_p->nick, nick);

  MyFree(retnick);
  MyFree(retpass);
  MyFree(retsalt);
  MyFree(retcloak);

  dbi_result_free(result);

  return nick_p;
}

static unsigned int
db_get_id_from_nick(const char *nick)
{
  dbi_result result;
  char *escnick;
  int id;

  if(dbi_driver_quote_string_copy(Database.driv, nick, &escnick) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    return 0;
  }

  result = db_query("SELECT id from nickname WHERE nick=%s", escnick);
  MyFree(escnick);
  
  if(result == NULL)
    return 0;

  if(dbi_result_get_numrows(result) == 0)
  {
    printf("db: WTF. Didn't find nickname entry for %s\n", nick);
    dbi_result_free(result);
    return 0;
  }

  dbi_result_first_row(result);
  dbi_result_get_fields(result, "id.%ui", &id);
  dbi_result_free(result);

  return id;
}

struct Nick *
db_register_nick(const char *nick, const char *password, const char *salt,
    const char *email)
{
  char *escnick = NULL;
  char *escemail = NULL;
  char *escpass = NULL;
  char *escsalt = NULL;
  dbi_result result;
  
  assert(nick != NULL);

  if(dbi_driver_quote_string_copy(Database.driv, nick, &escnick) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    return NULL;
  }

  if(dbi_driver_quote_string_copy(Database.driv, email, &escemail) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    MyFree(escnick);
    return NULL;
  }

  if(dbi_driver_quote_string_copy(Database.driv, password, &escpass) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    MyFree(escnick);
    MyFree(escemail);
    return NULL;
  }
  
  if(dbi_driver_quote_string_copy(Database.driv, salt, &escsalt) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    MyFree(escnick);
    MyFree(escemail);
    MyFree(escpass);
    return NULL;
  }
  
  result = db_query("INSERT INTO %s (nick, password, salt, email, reg_time,"
      " last_seen, last_used) VALUES(%s, %s, %s, %s, %ld, %ld, %ld)", "nickname", 
      escnick, escpass, escsalt, escemail, CurrentTime, CurrentTime, CurrentTime);

  MyFree(escnick);
  MyFree(escemail);
  MyFree(escpass);
  MyFree(escsalt);
 
  if(result == NULL)
    return NULL;
  
  dbi_result_free(result);

  return db_find_nick(nick); 
}

int
db_delete_nick(const char *nick)
{
  dbi_result result;
  char *escnick;

  if(dbi_driver_quote_string_copy(Database.driv, nick, &escnick) == 0)
  {
    printf("db: Failed to delete nick: dbi_driver_quote_string_copy\n");
    return -1;
  }

  result = db_query("DELETE FROM %s WHERE nick=%s", "nickname", escnick);
 
  MyFree(escnick);
  
  if(result == NULL)
    return 0;
  
  dbi_result_free(result);

  return 0;
}

int
db_nick_set_string(unsigned int id, const char *key, const char *value)
{
  dbi_result result;
  char *escvalue;

  if(dbi_driver_quote_string_copy(Database.driv, value, &escvalue) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    return -1;
  }
  
  result = db_query("UPDATE %s SET %s=%s WHERE id=%d", "nickname", key, 
      escvalue, id);
  
  MyFree(escvalue);

  if(result == NULL)
    return -1;

  dbi_result_free(result);

  return 0;
}

int
db_nick_set_number(unsigned int id, const char *key, const unsigned long value)
{
  dbi_result result;

  if((result = db_query("UPDATE %s SET %s=%ld WHERE id=%d", 
          "nickname", key, value, id)) == NULL)
  {
    return -1;
  }

  dbi_result_free(result);

  return 0;
}

char *
db_nick_get_string(unsigned int id, const char *key)
{
  dbi_result result;
  char *esckey;
  char *value;
  char buffer[512];

  assert(key != NULL);
  
  if(dbi_driver_quote_string_copy(Database.driv, key, &esckey) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    return NULL;
  }
   
  result = db_query("SELECT %s FROM %s WHERE id=%d", esckey, "nickname", id);

  MyFree(esckey);
  
  if(result == NULL)
    return NULL;

  if(dbi_result_get_numrows(result) == 0)
  {
    printf("db: %s not found\n", key);
    return NULL;
  }

  snprintf(buffer, sizeof(buffer), "%s.%%S", key);

  dbi_result_first_row(result);
  dbi_result_get_fields(result, buffer, &value);

  return value;
}

unsigned int
db_get_id_from_chan(const char *chan)
{
  dbi_result result;
  char *escchan;
  int id;

  if(dbi_driver_quote_string_copy(Database.driv, chan, &escchan) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    return 0;
  }

  result = db_query("SELECT id from channel WHERE channel=%s", escchan);
  MyFree(escchan);
  
  if(result == NULL)
    return 0;

  if(dbi_result_get_numrows(result) == 0)
  {
    printf("db: WTF. Didn't find channel entry for %s\n", chan);
    dbi_result_free(result);
    return 0;
  }

  dbi_result_first_row(result);
  dbi_result_get_fields(result, "id.%ui", &id);
  dbi_result_free(result);

  return id;
}

int
db_chan_set_string(unsigned int id, const char *key, const char *value)
{
  dbi_result result;
  char *escvalue;

  if(dbi_driver_quote_string_copy(Database.driv, value, &escvalue) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    return -1;
  }
  
  result = db_query("UPDATE %s SET %s=%s WHERE id=%d", "channel", key, 
      escvalue, id);
  
  MyFree(escvalue);

  if(result == NULL)
    return -1;

  dbi_result_free(result);

  return 0;
}

int
db_chan_set_number(unsigned int id, const char *key, const unsigned long value)
{
  dbi_result result;

  if((result = db_query("UPDATE %s SET %s=%ld WHERE id=%d", 
          "channel", key, value, id)) == NULL)
  {
    return -1;
  }

  dbi_result_free(result);

  return 0;
}

struct RegChannel *
db_find_chan(const char *channel)
{
  dbi_result result;
  char *escchannel = NULL;
  char *findchannel; char *findfounder;
  struct RegChannel *channel_p;
  
  assert(channel != NULL);

  if(dbi_driver_quote_string_copy(Database.driv, channel, &escchannel) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    return NULL;
  }

  result = db_query("SELECT %s.id, channel, description, entrymsg, channel.flags, nickname.nick FROM %s "
          "INNER JOIN %s ON %s.founder=%s.id WHERE channel=%s", 
          "channel", "channel", "nickname", "channel", "nickname", 
          escchannel);
 
  MyFree(escchannel);
  
  if(result == NULL)
    return NULL;
  
  if(dbi_result_get_numrows(result) == 0)
  {
    printf("db: Channel %s not found\n", channel);
    return NULL;
  }

  channel_p = MyMalloc(sizeof(struct RegChannel));
  dbi_result_first_row(result);
  dbi_result_get_fields(result, "id.%ui channel.%S description.%S entrymsg.%S flags.%ui nick.%S",
      &channel_p->id, &findchannel, &channel_p->description, 
      &channel_p->entrymsg, &channel_p->flags, &findfounder);

  strlcpy(channel_p->channel, findchannel, sizeof(channel_p->channel));
  strlcpy(channel_p->founder, findfounder, sizeof(channel_p->founder));

  MyFree(findchannel);
  MyFree(findfounder);

  return channel_p;
}

int
db_register_chan(struct Client *client, char *channelname)
{
  struct RegChannel *channel;
  char *escchannel = NULL;
  char *escfounder = NULL;
  dbi_result result;
  
  assert(channel != NULL);

  if(dbi_driver_quote_string_copy(Database.driv, channelname, &escchannel) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    MyFree(escchannel);
    return -1;
  }

  if(dbi_driver_quote_string_copy(Database.driv, client->name, &escfounder) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    MyFree(escchannel);
    MyFree(escfounder);
    return -1;
  }
  
  result = db_query("INSERT INTO %s (channel, founder) VALUES(%s, "
          "(SELECT id FROM nickname WHERE nick=%s))", "channel", escchannel, 
          escfounder);

  MyFree(escchannel);
  MyFree(escfounder);
 
  if(result == NULL)
    return 0;

  dbi_result_free(result);

  return 0;
}

int
db_delete_chan(const char *chan)
{
  dbi_result result;
  char *escchan;

  if(dbi_driver_quote_string_copy(Database.driv, chan, &escchan) == 0)
  {
    printf("db: Failed to delete channel: dbi_driver_quote_string_copy\n");
    return -1;
  }

  if((result = db_query("DELETE FROM %s WHERE channel=%s", 
          "channel", escchan)) == NULL)
  {
    MyFree(escchan);
    return 0;
  }  
  
  MyFree(escchan);
  dbi_result_free(result);

  return 0;
}

int
db_set_founder(const char *channel, const char *nickname)
{
  dbi_result result;

  char *escchannel, *escnick;

  if(dbi_driver_quote_string_copy(Database.driv, nickname, &escnick) == 0)
  {
    printf("db: Failed to delete channel: dbi_driver_quote_string_copy\n");
    return -1;
  }

  if(dbi_driver_quote_string_copy(Database.driv, channel, &escchannel) == 0)
  {
    printf("db: Failed to delete channel: dbi_driver_quote_string_copy\n");
    MyFree(escnick);
    return -1;
  }

  result = db_query("UPDATE %s SET founder="
    "(SELECT id FROM nickname WHERE nick=%s) WHERE channel=%s",
    "channel", escnick, escchannel);
  if (result != NULL)
    MyFree(result);

  MyFree(escnick);
  MyFree(escchannel);

  return 0;
}

int
db_set_successor(const char *channel, const char *nickname)
{
  dbi_result result;

  char *escchannel, *escnick;

  if(dbi_driver_quote_string_copy(Database.driv, nickname, &escnick) == 0)
  {
    printf("db: Failed to delete channel: dbi_driver_quote_string_copy\n");
    return -1;
  }

  if(dbi_driver_quote_string_copy(Database.driv, channel, &escchannel) == 0)
  {
    printf("db: Failed to delete channel: dbi_driver_quote_string_copy\n");
    MyFree(escnick);
    return -1;
  }

  result = db_query("UPDATE %s SET successor="
    "(SELECT id FROM nickname WHERE nick=%s) WHERE channel=%s",
    "channel", escnick, escchannel);
  if (result != NULL)
    MyFree(result);

  MyFree(escnick);
  MyFree(escchannel);

  return 0;
}

int
db_list_add(const char *table, unsigned int id, const char *value)
{
  char *escvalue;
  dbi_result result;
  
  if(dbi_driver_quote_string_copy(Database.driv, value, &escvalue) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    return -1;
  }
  
  result = db_query("INSERT INTO %s (parent_id, entry) VALUES(%d, %s)", table, id, 
      escvalue);
  
  MyFree(escvalue);
  
  if(result == NULL)
    return -1;

  dbi_result_free(result);
  
  return 0;
}

void *
db_list_first(const char *table, unsigned int id, struct AccessEntry *entry)
{
  dbi_result result;
    
  if((result = db_query("SELECT id, entry FROM %s WHERE parent_id=%d", 
          table, id)) == NULL)
  {
    return NULL;
  }

  if(dbi_result_get_numrows(result) == 0)
  {
    printf("db: %d has no access list\n", id);
    return NULL;
  }

  if(dbi_result_first_row(result))
  {
    dbi_result_get_fields(result, "id.%ui entry.%S", &entry->id, &entry->value);
    return result;
  }

  return NULL;
}

void *
db_list_next(void *result, struct AccessEntry *entry)
{
  if(dbi_result_next_row(result))
  {
    dbi_result_get_fields(result, "id.%ui entry.%S", &entry->id, &entry->value);
    return result;
  }
  return NULL;
}

void
db_list_done(void *result)
{
  dbi_result_free(result);
}

int
db_list_del(const char *table, unsigned int id, const char *entry)
{
  dbi_result result;
  char *escentry;
  int numrows;
    
  if(dbi_driver_quote_string_copy(Database.driv, entry, &escentry) == 0)
  {
    printf("db: Failed to query: dbi_driver_quote_string_copy\n");
    return 0;
  }

  if((result = db_query("DELETE FROM %s WHERE parent_id=%d AND entry=%s", 
          table, id, escentry)) == NULL)
  {
    MyFree(escentry);
    return 0;
  }
    
  MyFree(escentry);

  numrows = dbi_result_get_numrows_affected(result);

  dbi_result_free(result);
  return numrows;
}

int 
db_list_del_index(const char *table, unsigned int id, unsigned int index)
{
  dbi_result result;
  unsigned int delid, numrows, j;
    
  if((result = db_query("SELECT id, entry FROM %s WHERE parent_id=%d",
          table, id)) == NULL)
  {
    return 0;
  }

  if((numrows = dbi_result_get_numrows(result)) == 0)
  {
    printf("db: %d has no access list\n", id);
    return 0;
  }

  if(dbi_result_first_row(result))
  {
    for(j = 1; j <= numrows; j++)
    {
      if(j == index)
      {
        dbi_result_get_fields(result, "id.%ui", &delid);
        dbi_result_free(result);
            
        if((result = db_query("DELETE FROM %s WHERE parent_id=%d AND id=%d", 
                table, id, delid)) == NULL)
        {
          return 0;
        }
        return 1;
      }
      dbi_result_next_row(result);
    }
  }
  dbi_result_free(result);
  return 0;
}

int
db_link_nicks(unsigned int master, unsigned int child)
{
  dbi_result result;

  result = db_query("INSERT INTO nickname_links (nick_id, link_id) VALUES "
      "(%d, %d)", master, child);

  if(result == NULL)
    return -1;
  
  return 0;
}

int
db_is_linked(const char *nick)
{
  dbi_result result;
  int ret, id;
  
  id = db_get_id_from_nick(nick);

  if(id <= 0)
    return FALSE;

  result = db_query("SELECT link_id FROM nickname_links WHERE link_id=%d", id);
  if(result == NULL)
    return FALSE;

  if(dbi_result_get_numrows(result) > 0)
    ret = TRUE;
  else
    ret = FALSE;

  dbi_result_free(result);

  return ret;
}

struct Nick *
db_unlink_nick(const char *nick)
{
  dbi_result result;
  struct Nick *nick_p;
  char *retnick, *retpass, *retcloak;
  int id;

  id = db_get_id_from_nick(nick);

  if(id <= 0)
    return NULL;

  result = db_query("DELETE FROM nickname_links WHERE link_id=%d", id);
  if(result == NULL)
    return NULL;

  dbi_result_free(result);

  if((result = db_query("SELECT id, nick, password, email, cloak, "
      "last_quit_time, reg_time, last_seen, last_used, flags, language "
      "FROM %s WHERE id=%d", "nickname", id)) == NULL)
  {
    return NULL;
  }

  nick_p = MyMalloc(sizeof(struct Nick));
  dbi_result_first_row(result);
  dbi_result_get_fields(result, "id.%ui nick.%S password.%S email.%S cloak.%S "
      "last_quit_time.%l reg_time.%l last_seen.%l last_used.%l "
      "flags.%ui language.%ui",
      &nick_p->id, &retnick, &retpass, &nick_p->email, &retcloak, 
      &nick_p->last_quit_time, &nick_p->reg_time, &nick_p->last_seen, 
      &nick_p->last_used, &nick_p->flags, &nick_p->language);

  strlcpy(nick_p->nick, retnick, sizeof(nick_p->nick));
  strlcpy(nick_p->pass, retpass, sizeof(nick_p->pass));
  strlcpy(nick_p->cloak, retcloak, sizeof(nick_p->cloak));

  printf("db_unlink_link: Found nick %s\n", nick_p->nick);

  MyFree(retnick);
  MyFree(retpass);
  MyFree(retcloak);

  dbi_result_free(result);

  return nick_p;
}
