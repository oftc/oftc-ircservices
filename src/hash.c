/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  hash.c: Maintains hash tables
 *
 *  Copyright (C) 2006 Stuart Walsh and the OFTC Coding department
 *
 *  Some parts:
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
 *  $Id$
 */

#include "stdinc.h"
#include "hash.h"
#include "client.h"
#include "mqueue.h"
#include "channel_mode.h"
#include "channel.h"
#include "dbm.h"
#include "parse.h"
#include "language.h"
#include "nickname.h"
#include "interface.h"

/*static BlockHeap *service_heap = NULL;
static BlockHeap *namehost_heap = NULL;
static struct Service *find_or_add_service(const char *);
*/
static unsigned int ircd_random_key = 0;

/* The actual hash tables, They MUST be of the same HASHSIZE, variable
 * size tables could be supported but the rehash routine should also
 * rebuild the transformation maps, I kept the tables of equal size 
 * so that I can use one hash function.
 */
static struct Client *idTable[HASHSIZE];
static struct Client *clientTable[HASHSIZE];
static struct Channel *channelTable[HASHSIZE];
static struct Service *serviceTable[HASHSIZE];

/* init_hash()
 *
 * inputs       - NONE
 * output       - NONE
 * side effects - Initialize the maps used by hash
 *                functions and clear the tables
 */
void
init_hash(void)
{
  unsigned int i;

  /* Default the service/namehost sizes to CLIENT_HEAP_SIZE for now,
   * should be a good close approximation anyway
   * - Dianora
   */
//  service_heap = BlockHeapCreate("service", sizeof(struct Service), CLIENT_HEAP_SIZE);
//  namehost_heap = BlockHeapCreate("namehost", sizeof(struct NameHost), CLIENT_HEAP_SIZE);

  ircd_random_key = rand() % 256;  /* better than nothing --adx */

  /* Clear the hash tables first */
  for (i = 0; i < HASHSIZE; ++i)
  {
    idTable[i]          = NULL;
    clientTable[i]      = NULL;
    channelTable[i]     = NULL;
    serviceTable[i]     = NULL;
  }
}

/*
 * Even newer hash algorithm.  Murmur hash or something.
 */
unsigned int
strhash(const char *name, int len)
{
    // 'm' and 'r' are mixing constants generated offline.
    // They're not really 'magic', they just happen to work well.
   
    const unsigned int m = 0x5bd1e995;
    const int r = 24;

    // Initialize the hash to a 'random' value
   
    unsigned int h = ircd_random_key ^ len;
   
    // Mix 4 bytes at a time into the hash
   
    const unsigned char * data = (const unsigned char *)name;
    if(name[0] == '\0')
      len = 0;
    else if(name[1] == '\0')
      len = 1;
    else if(name[2] == '\0')
      len = 2;
    else if(name[3] == '\0')
      len = 3;

    while(len >= 4)
    {
      unsigned int k = *(unsigned int *)data;

      k *= m; 
      k ^= k >> r; 
      k *= m; 
      
      h *= m; 
      h ^= k;
   
      data += 4;
      len -= 4;

      if(data[0] == 0)
        len = 0;
      else if(data[1] == 0)
        len = 1;
      else if(data[2] == 0)
        len = 2;
      else if(data[3] == 0)
        len = 3;
    }
    
    // Handle the last few bytes of the input array
   
    switch(len)
    {
    case 3: h ^= data[2] << 16;
    case 2: h ^= data[1] << 8;
    case 1: h ^= data[0];
            h *= m;
    };
   
    // Do a few final mixes of the hash to ensure the last few
    // bytes are well-incorporated.
   
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;
   
    return h % HASHSIZE;
}

/************************** Externally visible functions ********************/

/* Optimization note: in these functions I supposed that the CSE optimization
 * (Common Subexpression Elimination) does its work decently, this means that
 * I avoided introducing new variables to do the work myself and I did let
 * the optimizer play with more free registers, actual tests proved this
 * solution to be faster than doing things like tmp2=tmp->hnext... and then
 * use tmp2 myself which would have given less freedom to the optimizer.
 */

/* hash_add_client()
 *
 * inputs       - pointer to client
 * output       - NONE
 * side effects - Adds a client's name in the proper hash linked
 *                list, can't fail, client must have a non-null
 *                name or expect a coredump, the name is infact
 *                taken from client->name
 */
void
hash_add_client(struct Client *client)
{
  unsigned int hashv = strhash(client->name, NICKLEN);

  client->hnext = clientTable[hashv];
  clientTable[hashv] = client;
}

void
hash_add_service(struct Service *service)
{
  unsigned int hashv = strhash(service->name, NICKLEN);

  service->hnext = serviceTable[hashv];
  serviceTable[hashv] = service;
}

/* hash_add_channel()
 *
 * inputs       - pointer to channel
 * output       - NONE
 * side effects - Adds a channel's name in the proper hash linked
 *                list, can't fail. chptr must have a non-null name
 *                or expect a coredump. As before the name is taken
 *                from chptr->name, we do hash its entire lenght
 *                since this proved to be statistically faster
 */
void
hash_add_channel(struct Channel *chptr)
{
  unsigned int hashv = strhash(chptr->chname, CHANNELLEN);

  chptr->hnextch = channelTable[hashv];
  channelTable[hashv] = chptr;
}

void
hash_add_id(struct Client *client)
{
  unsigned int hashv = strhash(client->id, IDLEN);

  client->idhnext = idTable[hashv];
  idTable[hashv] = client;
}

/* hash_del_id()
 *
 * inputs       - pointer to client
 * output       - NONE
 * side effects - Removes an ID from the hash linked list
 */
void
hash_del_id(struct Client *client)
{
  unsigned int hashv = strhash(client->id, IDLEN);
  struct Client *tmp = idTable[hashv];

  if (tmp != NULL)
  {
    if (tmp == client)
    {
      idTable[hashv] = client->idhnext;
      client->idhnext = client;
    }
    else
    {
      while (tmp->idhnext != client)
      {
        if ((tmp = tmp->idhnext) == NULL)
          return;
      }

      tmp->idhnext = tmp->idhnext->idhnext;
      client->idhnext = client;
    }
  }
}

/* hash_del_client()
 *
 * inputs       - pointer to client
 * output       - NONE
 * side effects - Removes a Client's name from the hash linked list
 */
void
hash_del_client(struct Client *client)
{
  unsigned int hashv = strhash(client->name, NICKLEN);
  struct Client *tmp = clientTable[hashv];

  if (tmp != NULL)
  {
    if (tmp == client)
    {
      clientTable[hashv] = client->hnext;
      client->hnext = client;
    }
    else
    {
      while (tmp->hnext != client)
      {
        if ((tmp = tmp->hnext) == NULL)
          return;
      }

      tmp->hnext = tmp->hnext->hnext;
      client->hnext = client;
    }
  }
}
/* hash_del_service()
 *
 * inputs       - pointer to service 
 * output       - NONE
 * side effects - Removes a service from the hash linked list
 */
void
hash_del_service(struct Service *service)
{
  unsigned int hashv = strhash(service->name, NICKLEN);
  struct Service *tmp = serviceTable[hashv];

  if (tmp != NULL)
  {
    if (tmp == service)
    {
      serviceTable[hashv] = service->hnext;
      service->hnext = service;
    }
    else
    {
      while (tmp->hnext != service)
      {
        if ((tmp = tmp->hnext) == NULL)
          return;
      }

      tmp->hnext = tmp->hnext->hnext;
      service->hnext = service;
    }
  }
}

/* hash_del_channel()
 *
 * inputs       - pointer to client
 * output       - NONE
 * side effects - Removes the channel's name from the corresponding
 *                hash linked list
 */
void
hash_del_channel(struct Channel *chptr)
{
  unsigned int hashv = strhash(chptr->chname, CHANNELLEN);
  struct Channel *tmp = channelTable[hashv];

  if (tmp != NULL)
  {
    if (tmp == chptr)
    {
      channelTable[hashv] = chptr->hnextch;
      chptr->hnextch = chptr;
    }
    else
    {
      while (tmp->hnextch != chptr)
      {
        if ((tmp = tmp->hnextch) == NULL)
          return;
      }

      tmp->hnextch = tmp->hnextch->hnextch;
      chptr->hnextch = chptr;
    }
  }
}

/* find_client()
 *
 * inputs       - pointer to name
 * output       - NONE
 * side effects - New semantics: finds a client whose name is 'name'
 *                if can't find one returns NULL. If it finds one moves
 *                it to the top of the list and returns it.
 */
struct Client *
find_client(const char *name)
{
  unsigned int hashv = strhash(name, NICKLEN);
  struct Client *client;

  if ((client = clientTable[hashv]) != NULL)
  {
    if (irccmp(name, client->name))
    {
      struct Client *prev;

      while (prev = client, (client = client->hnext) != NULL)
      {
        if (!irccmp(name, client->name))
        {
          prev->hnext = client->hnext;
          client->hnext = clientTable[hashv];
          clientTable[hashv] = client;
          break;
        }
      }
    }
  }

  return client;
}

/* find_service()
 *
 * inputs       - pointer to name
 * output       - NONE
 * side effects - New semantics: finds a service whose name is 'name'
 *                if can't find one returns NULL. If it finds one moves
 *                it to the top of the list and returns it.
 */
struct Service *
find_service(const char *name)
{
  unsigned int hashv = strhash(name, NICKLEN);
  struct Service *service;

  if ((service = serviceTable[hashv]) != NULL)
  {
    if (irccmp(name, service->name))
    {
      struct Service *prev;

      while (prev = service, (service = service->hnext) != NULL)
      {
        if (!irccmp(name, service->name))
        {
          prev->hnext = service->hnext;
          service->hnext = serviceTable[hashv];
          serviceTable[hashv] = service;
          break;
        }
      }
    }
  }

  return service;
}

struct Client *
hash_find_id(const char *name)
{
  unsigned int hashv = strhash(name, IDLEN);
  struct Client *client;

  if ((client = idTable[hashv]) != NULL)
  {
    if (irccmp(name, client->id))
    {
      struct Client *prev;

      while (prev = client, (client = client->idhnext) != NULL)
      {
        if (!irccmp(name, client->id))
        {
          prev->idhnext = client->idhnext;
          client->idhnext = idTable[hashv];
          idTable[hashv] = client;
          break;
        }
      }
    }
  }

  return client;
}

/*
 * Whats happening in this next loop ? Well, it takes a name like
 * foo.bar.edu and proceeds to earch for *.edu and then *.bar.edu.
 * This is for checking full server names against masks although
 * it isnt often done this way in lieu of using matches().
 *
 * Rewrote to do *.bar.edu first, which is the most likely case,
 * also made const correct
 * --Bleep
 */
static struct Client *
hash_find_masked_server(const char *name)
{
  char buf[HOSTLEN + 1];
  char *p = buf;
  char *s = NULL;
  struct Client *server = NULL;

  if (*name == '*' || *name == '.')
    return NULL;

  /*
   * copy the damn thing and be done with it
   */
  strlcpy(buf, name, sizeof(buf));

  while ((s = strchr(p, '.')) != NULL)
  {
    *--s = '*';

    /* Dont need to check IsServer() here since nicknames cant
     * have *'s in them anyway.
     */
    if ((server = find_client(s)) != NULL)
      return server;
    p = s + 2;
  }

  return NULL;
}

struct Client *
find_server(const char *name)
{
  unsigned int hashv = strhash(name, NICKLEN);
  struct Client *client = NULL;

  if (IsDigit(*name) && strlen(name) == IRC_MAXSID)
    client = hash_find_id(name);

  if ((client == NULL) && (client = clientTable[hashv]) != NULL)
  {
    if ((!IsServer(client) && !IsMe(client)) ||
        irccmp(name, client->name))
    {
      struct Client *prev;

      while (prev = client, (client = client->hnext) != NULL)
      {
        if ((IsServer(client) || IsMe(client)) &&
            !irccmp(name, client->name))
        {
          prev->hnext = client->hnext;
          client->hnext = clientTable[hashv];
          clientTable[hashv] = client;
          break;
        }
      }
    }
  }

  return (client != NULL) ? client : hash_find_masked_server(name);
}

/* hash_find_channel()
 *
 * inputs       - pointer to name
 * output       - NONE
 * side effects - New semantics: finds a channel whose name is 'name', 
 *                if can't find one returns NULL, if can find it moves
 *                it to the top of the list and returns it.
 */
struct Channel *
hash_find_channel(const char *name)
{
  unsigned int hashv = strhash(name, CHANNELLEN);
  struct Channel *chptr = NULL;

  if ((chptr = channelTable[hashv]) != NULL)
  {
    if (irccmp(name, chptr->chname))
    {
      struct Channel *prev;

      while (prev = chptr, (chptr = chptr->hnextch) != NULL)
      {
        if (!irccmp(name, chptr->chname))
        {
          prev->hnextch = chptr->hnextch;
          chptr->hnextch = channelTable[hashv];
          channelTable[hashv] = chptr;
          break;
        }
      }
    }
  }

  return chptr;
}

/* hash_get_bucket(int type, unsigned int hashv)
 *
 * inputs       - hash value (must be between 0 and HASHSIZE - 1)
 * output       - NONE
 * returns      - pointer to first channel in channelTable[hashv]
 *                if that exists;
 *                NULL if there is no channel in that place;
 *                NULL if hashv is an invalid number.
 * side effects - NONE
 */
void *
hash_get_bucket(int type, unsigned int hashv)
{
  assert(hashv < HASHSIZE);
  if (hashv >= HASHSIZE)
      return NULL;

  switch (type)
  {
    case HASH_TYPE_ID:
      return idTable[hashv];
      break;
    case HASH_TYPE_CHANNEL:
      return channelTable[hashv];
      break;
    case HASH_TYPE_CLIENT:
      return clientTable[hashv];
      break;
    case HASH_TYPE_SERVICE:
      return serviceTable[hashv];
      break;
    default:
      assert(0);
  }

  return NULL;
}

struct Service *
hash_find_service(const char *host)
{
  unsigned int hashv = strhash(host, NICKLEN);
  struct Service *service;

  if ((service = serviceTable[hashv]))
  {
    if (irccmp(host, service->name))
    {
      struct Service *prev;

      while (prev = service, (service = service->hnext) != NULL)
      {
        if (!irccmp(host, service->name))
        {
          prev->hnext = service->hnext;
          service->hnext = serviceTable[hashv];
          serviceTable[hashv] = service;
          break;
        }
      }
    }
  }

  return service;
}

struct MessageQueue *
hash_find_mqueue_host(struct MessageQueue **hash, const char *host)
{
  unsigned int hashv = strhash(host, IRC_BUFSIZE);
  struct MessageQueue *queue;
  if((queue = hash[hashv]))
  {
    if(irccmp(host, queue->name))
    {
      struct MessageQueue *prev;

      while(prev = queue, (queue = prev->hnext) != NULL)
      {
        if(!irccmp(host, queue->name))
        {
          prev->hnext = queue->next;
          queue->next = hash[hashv];
          hash[hashv] = queue;
          break;
        }
      }
    }
  }

  return queue;
}

void
hash_del_mqueue(struct MessageQueue **hash, struct MessageQueue *queue)
{
  unsigned int hashv = strhash(queue->name, IRC_BUFSIZE);
  struct MessageQueue *tmp = hash[hashv];

  if (tmp != NULL)
  {
    if (tmp == queue)
    {
      hash[hashv] = queue->hnext;
      queue->hnext = queue;
    }
    else
    {
      while (tmp->hnext != queue)
      {
        if ((tmp = tmp->hnext) == NULL)
          return;
      }

      tmp->hnext = tmp->hnext->hnext;
      queue->hnext = queue;
    }
  }
}

void
hash_add_mqueue(struct MessageQueue **hash, struct MessageQueue *queue)
{
  unsigned int hashv = strhash(queue->name, IRC_BUFSIZE);

  queue->hnext = hash[hashv];
  hash[hashv] = queue;
}

struct MessageQueue **
new_mqueue_hash()
{
  struct MessageQueue **tmp;
  unsigned int i;

  tmp = MyMalloc(sizeof(struct MessageQueue *) * HASHSIZE);

  for(i = 0; i < HASHSIZE; ++i)
    tmp[i] = NULL;

  return tmp;
}
