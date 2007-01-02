/* TODO: add copyright block */

#ifndef INCLUDED_conf_h
#define INCLUDED_conf_h

int conf_fbgets(char *, int, FBFILE *);
void read_services_conf(int);

void split_nuh(struct split_nuh_item *const);

#endif /* INCLUDED_conf_h */
