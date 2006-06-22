#ifndef PACKET_H
#define PACKET_H

extern struct Callback *iorecv_cb;
extern struct Callback *iosend_cb;

void *iorecv_default(va_list args);  
void read_packet(fde_t *, void *);

#endif
