/* TODO: add copyright block */

#ifndef INCLUDED_interface_h
#define INCLUDED_interface_h

#include "jupe.h"
#include "nickname.h"
#include "parse.h"
#include "language.h"

struct Service;
struct Client;
struct ServiceMask;

struct Service
{
  dlink_node node;
  struct Service *hnext;

  char name[NICKLEN+1];
  struct ServiceMessageTree msg_tree;
  char *last_command;
  struct LanguageFile languages[LANG_LAST];
  void *data;
};

struct ModeList
{
  unsigned int mode;
  unsigned char letter;
};

extern dlink_list services_list;
extern struct Callback *send_newuser_cb;
extern struct Callback *send_privmsg_cb;
extern struct Callback *send_notice_cb;
extern struct Callback *send_gnotice_cb;
extern struct Callback *send_umode_cb;
extern struct Callback *send_cloak_cb;
extern struct Callback *send_nick_cb;
extern struct Callback *send_akill_cb;
extern struct Callback *send_unakill_cb;
extern struct Callback *send_kick_cb;
extern struct Callback *send_cmode_cb;
extern struct Callback *send_invite_cb;
extern struct Callback *send_topic_cb;
extern struct Callback *send_kill_cb;
extern struct Callback *send_resv_cb;
extern struct Callback *send_unresv_cb;
extern struct Callback *send_newserver_cb;
extern struct Callback *send_join_cb;
extern struct Callback *send_part_cb;
extern struct Callback *send_nosuchsrv_cb;
extern struct Callback *send_chops_notice_cb;
extern struct Callback *send_squit_cb;

extern struct Callback *on_umode_change_cb;
extern struct Callback *on_cmode_change_cb;
extern struct Callback *on_squit_cb;
extern struct Callback *on_quit_cb;
extern struct Callback *on_part_cb;
extern struct Callback *on_join_cb;
extern struct Callback *on_nick_change_cb;
extern struct Callback *on_identify_cb;
extern struct Callback *on_newuser_cb;
extern struct Callback *on_channel_created_cb;
extern struct Callback *on_channel_destroy_cb;
extern struct Callback *on_topic_change_cb;
extern struct Callback *on_privmsg_cb;
extern struct Callback *on_notice_cb;
extern struct Callback *on_burst_done_cb;
extern struct Callback *on_certfp_cb;
extern struct Callback *on_nick_reg_cb;
extern struct Callback *on_chan_reg_cb;
extern struct Callback *on_group_reg_cb;
extern struct Callback *on_nick_drop_cb;
extern struct Callback *on_chan_drop_cb;
extern struct Callback *on_group_drop_cb;
extern struct Callback *on_db_init_cb;
extern struct Callback *on_ctcp_cb;

extern struct Callback *do_event_cb;

extern struct ModeList *ServerModeList;

void init_interface();
void cleanup_interface();

struct Service *make_service(char *);
struct Client *introduce_client(const char *, const char*, char);
struct Client *introduce_server(const char*, const char*);
void squit_server(const char *, const char *);
struct Channel *join_channel(struct Client *, struct Channel*);
void part_channel(struct Client *, const char*, const char*);
size_t strtime(struct Client *, time_t, char *);
void reply_time(struct Service *, struct Client *, unsigned int, time_t);
void reply_user(struct Service *,struct Service *, struct Client *, 
    unsigned int, ...);
void reply_mail(struct Service *, struct Client *, unsigned int, 
    unsigned int, ...);
void global_notice(struct Service *, char *, ...);
void cloak_user(struct Client *, const char *);
void do_help(struct Service *, struct Client *, const char *, int, char **);
void identify_user(struct Client *);
void send_nick_change(struct Service *, struct Client *, const char *);
void send_umode(struct Service *, struct Client *, const char *);
void send_akill(struct Service *, char *, struct ServiceMask *);
void send_resv(struct Service *, char *, char *, time_t);
void send_unresv(struct Service *, char *);
void remove_akill(struct Service *, struct ServiceMask *);
void send_cmode(struct Service *, struct Channel *, const char *, const char *);
void send_topic(struct Service *, struct Channel *, struct Client *, 
    const char *);
void send_kill(struct Service *, struct Client *, const char *);
void set_limit(struct Service *, struct Channel *, int);

unsigned int enforce_mode_lock(struct Service *, struct Channel *, const char *, char *,
    int *);

int set_mode_lock(struct Service *, const char *, struct Client *, 
    const char *, char);

char *replace_string(char *, const char *);
int check_list_entry(unsigned int, unsigned int, const char *);
int check_nick_pass(struct Client *, Nickname *, const char *);
void make_random_string(char *, size_t);
int enforce_matching_serviceban(struct Service *, struct Channel *, 
    struct Client *);
int enforce_client_serviceban(struct Service *, struct Channel *, struct Client *,
    struct ServiceMask *);

void kick_user(struct Service *, struct Channel *, const char *, const char *);
void op_user(struct Service *, struct Channel *, struct Client *);
void deop_user(struct Service *, struct Channel *, struct Client *);
void voice_user(struct Service *, struct Channel *, struct Client *);
void devoice_user(struct Service *, struct Channel *, struct Client *);
void invite_user(struct Service *, struct Channel *, struct Client *);
void kill_user(struct Service *, struct Client *, const char *);
void ban_mask(struct Service *, struct Channel *, const char *);
void ban_mask_many(struct Service *, struct Channel *, dlink_list *);
void unban_mask(struct Service *, struct Channel *, const char *);
void unban_mask_many(struct Service *, struct Channel *, dlink_list *);
void quiet_mask(struct Service *, struct Channel *, const char *);
void quiet_mask_many(struct Service *, struct Channel *, dlink_list *);
void unquiet_mask(struct Service *, struct Channel *, const char *);
void unquiet_mask_many(struct Service *, struct Channel *, dlink_list *);
void invex_mask(struct Service *, struct Channel *, const char *);
void invex_mask_many(struct Service *, struct Channel *, dlink_list *);
void uninvex_mask(struct Service *, struct Channel *, const char *);
void uninvex_mask_many(struct Service *, struct Channel *, dlink_list *);
void except_mask(struct Service *, struct Channel *, const char *);
void except_mask_many(struct Service *, struct Channel *, dlink_list *);
void unexcept_mask(struct Service *, struct Channel *, const char *);
void unexcept_mask_many(struct Service *, struct Channel *, dlink_list *);
int valid_wild_card(const char *);

void free_jupeentry(struct JupeEntry *);

void ctcp_user(struct Service *, struct Client *, const char *);
void sendto_channel(struct Service *, struct Channel *, const char *);

unsigned int get_mode_from_letter(char);
void get_modestring(unsigned int, char *, int);

char *generate_hmac(const char *);

char *check_masterless_channels(unsigned int);

void send_chops_notice(struct Service *, struct Channel *, const char *, ...);

int drop_nickname(struct Service *, struct Client *, const char *);

extern struct LanguageFile ServicesLanguages[LANG_LAST];


void clump_masks(struct Service *, struct Channel *, const char *, int, int,
  dlink_list *);

void mask_normalize(char *, char *);
#endif /* INCLUDED_interface_h */
