/* TODO: add copyright block */

#ifndef INCLUDED_dbm_h
#define INCLUDED_dbm_h

struct ChanAccess
{
  unsigned int id;
  unsigned int account;
  unsigned int channel;
  unsigned int level;
};

struct JupeEntry
{
  unsigned int id;
  unsigned int setter;
  char *name;
  char *reason;
};

struct InfoChanList
{
  int channel_id;
  int ilevel;
  char *channel;
  char *level;
};

typedef struct row
{
  int col_count;
  char **cols;
} row_t;

typedef struct result_set
{
  int row_count;
  row_t *rows;
} result_set_t;

typedef struct DataBaseModule
{
  void *connection;
  int last_index;
  int(*connect)(const char *);
  char*(*execute_scalar)(int, int *, const char *, va_list);
  result_set_t*(*execute)(int, int *, const char *, va_list);
  int (*execute_nonquery)(int, const char *, va_list);
  void(*free_result)(result_set_t*);
  int (*prepare)(int, const char *);
  int (*begin_transaction)();
  int (*commit_transaction)();
  int (*rollback_transaction)();
  int64_t (*next_id)(const char *, const char *);
  int64_t (*insert_id)(const char *, const char *);
} database_t;

enum db_list_type
{
  ADMIN_LIST = 0,
  CHACCESS_LIST,
  AKICK_LIST,
  NICKLINK_LIST,
  NICKCHAN_LIST,
  CHMASTER_LIST,
  NICK_LIST,
  NICK_LIST_OPER,
  NICK_FORBID_LIST,
  CHAN_LIST,
  CHAN_LIST_OPER,
  CHAN_FORBID_LIST,
  EXPIRING_AKILL_LIST,
  JUPE_LIST
};

enum db_queries
{
  GET_FULL_NICK = 0,
  GET_NICK_FROM_ACCID,
  GET_NICK_FROM_NICKID,
  GET_ACCID_FROM_NICK,
  GET_NICKID_FROM_NICK,
  INSERT_ACCOUNT,
  INSERT_NICK,
  DELETE_NICK,
  DELETE_ACCOUNT,
  INSERT_NICKACCESS,
  GET_NICKACCESS,
  GET_ADMINS,
  GET_AKILLS,
  GET_CHAN_ACCESSES,
  GET_CHANID_FROM_CHAN,
  GET_FULL_CHAN,
  INSERT_CHAN,
  INSERT_CHANACCESS,
  SET_CHAN_LEVEL,
  DELETE_CHAN_ACCESS,
  GET_CHAN_ACCESS,
  DELETE_CHAN,
  GET_AKILL,
  INSERT_AKILL,
  INSERT_SERVICES_AKILL,
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
  SET_NICK_ADMIN,
  SET_NICK_PRIVATE,
  DELETE_NICKACCESS,
  DELETE_ALL_NICKACCESS,
  DELETE_NICKACCESS_IDX,
  SET_NICK_LINK,
  SET_NICK_LINK_EXCLUDE,
  INSERT_NICK_CLONE,
  GET_NEW_LINK,
  SET_CHAN_DESC,
  SET_CHAN_URL,
  SET_CHAN_EMAIL,
  SET_CHAN_ENTRYMSG,
  SET_CHAN_TOPIC,
  SET_CHAN_MLOCK,
  SET_CHAN_PRIVATE,
  SET_CHAN_RESTRICTED,
  SET_CHAN_TOPICLOCK,
  SET_CHAN_VERBOSE,
  SET_CHAN_AUTOLIMIT,
  SET_CHAN_EXPIREBANS,
  SET_CHAN_FLOODSERV,
  SET_CHAN_AUTOOP,
  SET_CHAN_AUTOVOICE,
  SET_CHAN_LEAVEOPS,
  INSERT_FORBID,
  GET_FORBID,
  DELETE_FORBID,
  INSERT_CHAN_FORBID,
  GET_CHAN_FORBID,
  DELETE_CHAN_FORBID,
  INSERT_AKICK_ACCOUNT,
  INSERT_AKICK_MASK,
  GET_AKICKS,
  DELETE_AKICK_IDX,
  DELETE_AKICK_MASK,
  DELETE_AKICK_ACCOUNT,
  SET_NICK_MASTER,
  DELETE_AKILL,
  GET_CHAN_MASTER_COUNT,
  GET_NICK_LINKS,
  GET_NICK_CHAN_INFO,
  GET_CHAN_MASTERS,
  DELETE_ACCOUNT_CHACCESS,
  DELETE_DUPLICATE_CHACCESS,
  MERGE_CHACCESS,
  GET_EXPIRED_AKILL,
  INSERT_SENT_MAIL,
  GET_SENT_MAIL,
  DELETE_EXPIRED_SENT_MAIL,
  GET_NICKS,
  GET_NICKS_OPER,
  GET_FORBIDS,
  GET_CHANNELS,
  GET_CHANNELS_OPER,
  GET_CHANNEL_FORBID_LIST,
  SAVE_NICK,
  INSERT_NICKCERT,
  GET_NICKCERTS,
  DELETE_NICKCERT,
  DELETE_NICKCERT_IDX,
  DELETE_ALL_NICKCERT,
  INSERT_JUPE,
  GET_JUPES,
  DELETE_JUPE_NAME,
  FIND_JUPE,
  COUNT_CHANNEL_ACCESS_LIST,
  GET_NICKCERT,
  SET_EXPIREBANS_LIFETIME,
  QUERY_COUNT
};

enum query_types 
{
  QUERY,
  EXECUTE
};

typedef struct query 
{
  int index;
  const char *name;
  int type;
} query_t;

#define Query(m, args...) Database.yada->query(Database.yada, m, args)
#define Execute(m, args...) Database.yada->execute(Database.yada, m, args)
#define Bind(m, args...) Database.yada->bind(Database.yada, m, args)
#define Fetch(r, b) Database.yada->fetch(Database.yada, r, b)
//#define Prepare(s, l) Database.yada->prepare(Database.yada, s, l)
#define Free(r) Database.yada->free(Database.yada, r)

void init_db();
void db_load_driver();
void cleanup_db();

int db_prepare(int, const char *);
char *db_execute_scalar(int, int*, const char *, ...);
result_set_t *db_execute(int, int *, const char *, ...);
int db_execute_nonquery(int, const char *, ...);

void db_free_result(result_set_t *result);

int64_t db_nextid(const char *, const char *);
int64_t db_insertid(const char *, const char *);

int db_begin_transaction();
int db_commit_transaction();
int db_rollback_transaction();

int db_set_string(unsigned int, unsigned int, const char *);
int db_set_number(unsigned int, unsigned int, unsigned long);
int db_set_bool(unsigned int, unsigned int, unsigned char);
char *db_get_string(const char *, unsigned int, const char *);

struct JupeEntry  *db_find_jupe(const char *);

int   db_list_add(unsigned int, const void *);
void *db_list_first(unsigned int, unsigned int, void **);
void *db_list_next(void *, unsigned int, void **);
void  db_list_done(void *);
int   db_list_del(unsigned int, unsigned int, const char *);
int   db_list_del_index(unsigned int, unsigned int, unsigned int);

int db_get_num_masters(unsigned int);
int db_get_num_channel_accesslist_entries(unsigned int);

int db_add_sentmail(unsigned int, const char *);
int db_is_mailsent(unsigned int, const char *);

void db_reopen_log();
void db_log(const char *, ...);

int db_string_list(unsigned int, dlink_list *, const char *, ...);
void db_string_list_free(dlink_list *);

#endif /* INCLUDED_dbm_h */
