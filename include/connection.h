#ifndef CONNECTIONH
#define CONNECTIONH

extern struct Client me;
extern struct Callback *connected_cb;

void connect_server();
CBFUNC server_connected;

#endif
