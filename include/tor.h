#ifndef INCLUDED_tor_h
#define INCLUDED_tor_h

struct TorNode {
  /* hash linked list */
  struct TorNode *next;
  /* iteratable list */
  dlink_node node;

  char host[HOSTLEN+1];
};

void init_tor();
void cleanup_tor();

#endif
