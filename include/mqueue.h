/* TODO: add copyright block */

#ifndef INCLUDED_mqueue_h
#define INCLUDED_mqueue_h

struct FloodMsg
{
  time_t time;
  char *message;
  struct FloodMsg *hnext;
  struct FloodMsg *next;
  dlink_node node;
};

struct MessageQueue
{
  char *name;
  int max;
  int msg_enforce_time;
  int lne_enforce_time;
  dlink_list entries;
  unsigned int type;
  time_t last_used;
  struct MessageQueue *hnext;
  struct MessageQueue *next;
  dlink_node node;
};

struct MessageQueue *mqueue_new(const char *, unsigned int, int, int,
  int);
void mqueue_hash_free(struct MessageQueue **, dlink_list *);
void mqueue_free(struct MessageQueue *);
void floodmsg_free(struct FloodMsg *);
struct FloodMsg *floodmsg_new(const char *);

void init_mqueue();
void cleanup_mqueue();

#endif /* INCLUDED_mqueue_h */
