/* TODO: add copyright block */

#ifndef INCLUDED_connection_h
#define INCLUDED_connection_h

extern struct Client me;
extern struct Callback *connected_cb;

void connect_server();
CBFUNC server_connected;

#endif /* INCLUDED_connection_h */
