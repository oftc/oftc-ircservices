#ifndef CONFHINC
#define CONFHINC

int conf_fbgets(char *, int, FBFILE *);
void read_services_conf(int);

void split_nuh(struct split_nuh_item *const);

#endif
