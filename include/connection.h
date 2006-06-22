#ifndef CONNECTIONH
#define CONNECTIONH

typedef struct
{
  char        *name;
  char        *host;
  int         port;
  int         flags;
  char        *protocol;
  dlink_node  *node;
} connection_conf_t;

#define CONF_FLAG_ZIP   0x0001
#define CONF_FLAG_SSL   0x0002

extern dlink_list connection_confs;

connection_conf_t *make_connection_conf();
void connect_server(connection_conf_t *);


#endif
