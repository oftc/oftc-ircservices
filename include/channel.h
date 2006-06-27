#ifndef CHANNEL_H
#define CHANNEL_H

struct Channel *make_channel(const char *);
void channel_init();

struct Channel
{
  struct Channel *hnextch;

  dlink_node node;

  dlink_list members;
  dlink_list invites;
  dlink_list banlist;
  dlink_list exceptlist;
  dlink_list invexlist;
  
  time_t channelts;
  char chname[CHANNELLEN + 1];
};

#endif
