#include "stdinc.h"

dlink_list connection_confs = { NULL, NULL, 0};

connection_conf_t *make_connection_conf()
{
  connection_conf_t *connection_conf;

  connection_conf = MyMalloc(sizeof(connection_conf_t));
  memset(connection_conf, 0, sizeof(connection_conf_t));

  return connection_conf;
}

void free_connection_conf(connection_conf_t *connection)
{
  MyFree(connection->name);
  MyFree(connection->host);
  MyFree(connection->protocol);

  dlinkDelete(connection->node, &connection_confs);
  free_dlink_node(connection->node);
  MyFree(connection);
}
