/* TODO: add copyright block */

#ifndef INCLUDED_packet_h
#define INCLUDED_packet_h

extern struct Callback *iorecv_cb;
extern struct Callback *iosend_cb;

void *iorecv_default(va_list args);  
void *iosend_default(va_list args);
void read_packet(fde_t *, void *);

#endif /* INCLUDED_packet_h */
