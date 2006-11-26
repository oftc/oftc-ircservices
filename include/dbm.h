#ifndef DBMH
#define DBMH

struct AccessEntry 
{
  unsigned int id;
  char *value;
};

struct ChannelAccessEntry
{
  unsigned int id;
  unsigned int nick_id;
  unsigned int channel_id;
  unsigned int level;
};

enum db_list_type
{
  ACCESS_LIST = 0,
  NICK_FLAG_LIST,
  AKILL_LIST,
  CHACCESS_LIST
};

void init_db();
void db_load_driver();

int   db_set_string(const char *, unsigned int, const char *, const char *);
int   db_set_number(const char *, unsigned int, const char *, 
    const unsigned long);
char *db_get_string(const char *, unsigned int, const char *);

struct Nick *db_find_nick(const char *);
struct Nick *db_register_nick(const char *, const char *, const char *,
    const char *);

int   db_delete_nick(const char *);
char  *db_get_nickname_from_id(unsigned int id);
unsigned int db_get_id_from_nick(const char *);

int db_link_nicks(unsigned int, unsigned int);
int db_nick_is_linked(const char *);
struct Nick *db_unlink_nick(const char *);

int db_register_chan(struct Client *, char *);
int db_delete_chan(const char *);
struct RegChannel *db_find_chan(const char *);

int  db_chan_access_add(struct ChannelAccessEntry*);
int  db_chan_access_del(struct RegChannel *, int);
struct ChannelAccessEntry *db_chan_access_get(int, int);

unsigned int db_get_id_from_chan(const char *);
int db_set_founder(const char *, const char *);
int db_set_successor(const char *, const char *);
int db_chan_success_founder(const char *);

int   db_list_add(const char *, unsigned int, const char *);
void *db_list_first(const char *, unsigned int, unsigned int, void **);
void *db_list_next(void *, unsigned int, void **);
void  db_list_done(void *);
int   db_list_del(const char *, unsigned int, const char *);
int   db_list_del_index(const char *, unsigned int, unsigned int);

struct AKill *db_add_akill(struct AKill *akill);

#endif
