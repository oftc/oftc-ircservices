#ifndef DBMH
#define DBMH

#include <yada.h>

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

struct DBResult
{
  yada_rc_t *rc;
  yada_rc_t *brc;
};

enum db_list_type
{
  ACCESS_LIST = 0,
  ADMIN_LIST,
  AKILL_LIST,
  CHACCESS_LIST,
  AKICK_LIST
};

enum db_queries
{
  GET_FULL_NICK = 0,
  GET_NICK_FROM_ID,
  GET_NICKID_FROM_NICK,
  INSERT_ACCOUNT,
  INSERT_NICK,
  DELETE_NICK,
  DELETE_ACCOUNT,
  INSERT_NICKACCESS,
  GET_NICKACCESS,
  GET_ADMINS,
  GET_AKILLS,
  GET_CHAN_ACCESS,
  GET_CHANID_FROM_CHAN,
  GET_FULL_CHAN,
  INSERT_CHAN,
  INSERT_CHANACCESS,
  SET_CHAN_LEVEL,
  DELETE_CHAN_ACCESS,
  GET_CHAN_ACCESSES,
  DELETE_CHAN,
  SET_CHAN_FOUNDER,
  SET_CHAN_FOUNDER_AS_SUCCESSOR,
  SET_CHAN_SUCCESSOR,
  INSERT_AKILL,
  GET_AKILL,
  SET_NICK_PASSWORD,
  SET_NICK_URL,
  SET_NICK_EMAIL,
  SET_NICK_CLOAK,
  SET_NICK_LAST_QUIT,
  SET_NICK_LAST_HOST,
  SET_NICK_LAST_REALNAME,
  SET_NICK_LANGUAGE,
  SET_NICK_LAST_QUITTIME,
  SET_NICK_LAST_SEEN,
  SET_NICK_CLOAKON,
  SET_NICK_SECURE,
  SET_NICK_ENFORCE,
  DELETE_NICKACCESS,
  DELETE_ALL_NICKACCESS,
  DELETE_NICKACCESS_IDX,
  SET_NICK_LINK,
  INSERT_NICK_CLONE,
  GET_NICK_COUNT,
  SET_CHAN_DESC,
  SET_CHAN_URL,
  SET_CHAN_EMAIL,
  SET_CHAN_ENTRYMSG,
  SET_CHAN_TOPIC,
  SET_CHAN_FORBIDDEN,
  SET_CHAN_PRIVATE,
  SET_CHAN_RESTRICTED,
  SET_CHAN_TOPICLOCK,
  SET_CHAN_SECURE,
  SET_CHAN_VERBOSE,
  SET_NICK_ADMIN,
  INSERT_FORBID,
  GET_FORBID,
  DELETE_FORBID,
  INSERT_AKICK_ACCOUNT,
  INSERT_AKICK_MASK,
  GET_AKICKS,
  DELETE_AKICK_IDX,
  DELETE_AKICK_MASK,
  DELETE_AKICK_ACCOUNT,
  QUERY_COUNT
};

enum query_types {
  QUERY,
  EXECUTE
};

typedef struct query {
  const char *name;
  yada_rc_t *rc;
  int type;
} query_t;

#define TransBegin() Database.yada->trx(Database.yada, 0)
#define TransCommit() Database.yada->commit(Database.yada)
#define TransRollback() Database.yada->rollback(Database.yada, 0)
#define Query(m, args...) Database.yada->query(Database.yada, m, args)
#define Execute(m, args...) Database.yada->execute(Database.yada, m, args)
#define Bind(m, args...) Database.yada->bind(Database.yada, m, args)
#define Fetch(r, b) Database.yada->fetch(Database.yada, r, b)
#define Prepare(s, l) Database.yada->prepare(Database.yada, s, l)
#define Free(r) Database.yada->free(Database.yada, r)
#define InsertID(t, c) Database.yada->insert_id(Database.yada, t, c)

void init_db();
void db_load_driver();

int db_set_string(unsigned int, unsigned int, const char *);
int db_set_number(unsigned int, unsigned int, unsigned long);
int db_set_bool(unsigned int, unsigned int, unsigned char);
char *db_get_string(const char *, unsigned int, const char *);

struct Nick *db_find_nick(const char *);
int db_register_nick(struct Nick *);
int db_delete_nick(const char *);
char *db_get_nickname_from_id(unsigned int);
unsigned int db_get_id_from_name(const char *, unsigned int);

int db_forbid_nick(const char *nick);
int db_is_forbid(const char *nick);
int db_delete_forbid(const char *nick);

int db_link_nicks(unsigned int, unsigned int);
unsigned int db_unlink_nick(unsigned int);

int db_register_chan(struct RegChannel *);
int db_delete_chan(const char *);
struct RegChannel *db_find_chan(const char *);

int db_chan_access_add(struct ChannelAccessEntry*);
int db_chan_access_del(struct RegChannel *, int);
struct ChannelAccessEntry *db_chan_access_get(int, int);

int db_set_founder(const char *, const char *);
int db_set_successor(const char *, const char *);
int db_chan_success_founder(const char *);

int   db_list_add(unsigned int, const void *);
void *db_list_first(unsigned int, unsigned int, void **);
void *db_list_next(void *, unsigned int, void **);
void  db_list_done(void *);
int   db_list_del(unsigned int, unsigned int, const char *);
int   db_list_del_index(unsigned int, unsigned int, unsigned int);

struct AKill *db_add_akill(struct AKill *akill);

#endif
