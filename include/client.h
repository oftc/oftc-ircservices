#ifndef CLIENT_H
#define CLIENT_H

extern dlink_list global_client_list;
extern dlink_list global_server_list;

typedef struct server
{
  dlink_node node;
  fde_t fd;
  struct dbuf_queue buf_recvq;
} server_t;

typedef struct client
{
  dlink_node node;
  char name[NAMELEN];
  server_t *server;
  struct client *from;
} client_t;

client_t *make_client();
server_t *make_server();

#endif
