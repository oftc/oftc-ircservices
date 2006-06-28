#ifndef CHANNEL_H
#define CHANNEL_H

struct Channel *make_channel(const char *);
void channel_init();
void remove_ban(struct Ban *bptr, dlink_list *list);
struct Membership *find_channel_link(struct Client *, struct Channel *);
void add_user_to_channel(struct Channel *, struct Client *, unsigned int, int);
void destroy_channel(struct Channel *);

struct Channel
{
  struct Channel *hnextch;

  dlink_node node;

  struct Mode mode;

  char *topic;
  char *topic_info;

  dlink_list members;
  dlink_list invites;
  dlink_list banlist;
  dlink_list exceptlist;
  dlink_list invexlist;
  
  time_t channelts;
  char chname[CHANNELLEN + 1];
};

struct Membership
{
  dlink_node channode;     /*!< link to chptr->members    */
  dlink_node usernode;     /*!< link to source_p->channel */
  struct Channel *chptr;   /*!< Channel pointer */
  struct Client *client_p; /*!< Client pointer */
  unsigned int flags;      /*!< user/channel flags, e.g. CHFL_CHANOP */
};

#define IsMember(who, chan) ((find_channel_link(who, chan)) ? 1 : 0)
#define AddMemberFlag(x, y) ((x)->flags |=  (y))
#define DelMemberFlag(x, y) ((x)->flags &= ~(y))

#endif
