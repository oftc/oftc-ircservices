#ifndef CONNECTIONH
#define CONNECTIONH

typedef struct
{
  char        *name;
  char        *host;
  char        *protocol;
  char        *password;
  int         port;
  int         flags;
  dlink_node  *node;
} connection_conf_t;

#define CONF_FLAG_ZIP   0x0001
#define CONF_FLAG_SSL   0x0002

extern dlink_list connection_confs;
extern client_t me;
extern struct Callback *connected_cb;

connection_conf_t *make_connection_conf();
void connect_server(connection_conf_t *);
CBFUNC server_connected;


#endif
