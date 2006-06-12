#include "stdinc.h"

dlink_list connection_confs = { NULL, NULL, NULL};

connection_conf_t *make_connection_conf()
{
  connection_conf_t *connection_conf;

  connection_conf = MyMalloc(sizeof(connection_conf_t));
  memset(connection_conf, 0, sizeof(connection_conf_t));

  return connection_conf;
}

void free_connection_conf(connection_conf_t *connection)
{
  MyFree(conf->name);
  MyFree(conf->host);
  MyFree(conf->protocol);

  dlinkDelete(conf->node, connection_confs);
  free_dlink_node(conf->node);
  MyFree(conf);
}
