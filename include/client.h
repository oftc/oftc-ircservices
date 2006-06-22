#ifndef CLIENT_H
#define CLIENT_H

extern dlink_list global_client_list;
extern dlink_list global_server_list;

#define FLAGS_PINGSENT      0x00000001UL /* Unreplied ping sent*/
#define FLAGS_DEADSOCKET    0x00000002UL /* Local socket is dead--Exiting soon*/
#define FLAGS_KILLED        0x00000004UL /* Prevents "QUIT" from being sent to this */
#define FLAGS_CLOSING       0x00000008UL /* set when closing */

#define STAT_SERVER         0x01
#define STAT_CLIENT         0x02
#define STAT_ME             0x04

#define IsDefunct(x)            ((x)->flags & (FLAGS_DEADSOCKET|FLAGS_CLOSING))

#define IsServer(x)             ((x)->status == STAT_SERVER)
#define IsClient(x)             ((x)->status == STAT_CLIENT)
#define IsMe(x)                 ((x)->status == STAT_ME)


#define IDLEN           12 /* this is the maximum length, not the actual
                              generated length; DO NOT CHANGE! */



typedef struct server
{
  dlink_node node;
  fde_t fd;
  int flags;
  struct dbuf_queue buf_recvq;
} server_t;

typedef struct client
{
  dlink_node node;

  struct client *hnext;         /* For client hash table lookups by name */
  struct client *idhnext;       /* For SID hash table lookups by sid */
  struct client *from;

  char          name[HOSTLEN+1];
  char          id[IDLEN + 1];  /* client ID, unique ID per client */
  server_t      *server;

  unsigned int  status;
  unsigned char handler;        /* Handler index */
} client_t;

client_t *make_client();
server_t *make_server();
client_t *find_person(const client_t *source, const char *name);

#endif
