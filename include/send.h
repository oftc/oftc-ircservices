#ifndef SEND_H
#define SEND_H

void send_queued_write(struct Client *to);
void sendto_server(struct Client *to, const char *pattern, ...);

#endif
