#ifndef SEND_H
#define SEND_H

void send_queued_write(struct Client *);
void send_queued_all(void);
void sendto_server(struct Client *, const char *, ...);

#endif
