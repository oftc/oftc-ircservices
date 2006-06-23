#include "stdinc.h"

dlink_list connection_confs = { NULL, NULL, 0};

static CNCB serv_connect_callback;
struct Callback *connected_cb;

connection_conf_t *
make_connection_conf()
{
  connection_conf_t *connection_conf;

  connection_conf = MyMalloc(sizeof(connection_conf_t));
  memset(connection_conf, 0, sizeof(connection_conf_t));

  return connection_conf;
}

void 
free_connection_conf(connection_conf_t *connection)
{
  MyFree(connection->name);
  MyFree(connection->host);
  MyFree(connection->protocol);

  dlinkDelete(connection->node, &connection_confs);
  free_dlink_node(connection->node);
  MyFree(connection);
}

void 
add_connection_conf(connection_conf_t *connection)
{
  connection->node = make_dlink_node();
  dlinkAdd(connection, connection->node, &connection_confs);
}

static void
serv_connect_callback(fde_t *fd, int status, void *data)
{
  client_t *client = (client_t*)data;
  server_t *server = NULL;

  assert(client != NULL);

  server = client->server;

  assert(server != NULL);
  assert(&server->fd == fd);

  if(status != COMM_OK)
  {
    printf("serv_connect_callback: connection failed :(\n");
    exit(1);
  }

  printf("serv_connect_callback: connection succeeded!\n");
  comm_setselect(fd, COMM_SELECT_READ, read_packet, client, 0);

  execute_callback(connected_cb, client);
}

void 
connect_server(connection_conf_t *connection)
{
  client_t *client = make_client();
  server_t *server = make_server();

  client->server = server;

  if(comm_open(&server->fd, AF_INET, SOCK_STREAM, 0, NULL) < 0)
  {
    printf("connect_server: Could not open socket\n");
    exit(1);
  }

  comm_connect_tcp(&server->fd, connection->host, connection->port,
      NULL, 0, serv_connect_callback, client, AF_INET, CONNECTTIMEOUT);
}

void *
server_connected(va_list args)
{
  return NULL;
}
