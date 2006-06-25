#ifndef CONNECTIONH
#define CONNECTIONH

extern client_t me;
extern struct Callback *connected_cb;

void connect_server();
CBFUNC server_connected;

#endif
