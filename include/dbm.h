#ifndef DBMH
#define DBMH

struct AccessEntry 
{
  unsigned int id;
  char *value;
};

void init_db();
void db_load_driver();

struct Nick *db_find_nick(const char *);
struct Nick *db_register_nick(const char *, const char *, const char *,
    const char *);
int db_delete_nick(const char*);

int db_register_chan(struct Client *, char *);
struct RegChannel *db_find_chan(const char *);
int db_delete_chan(const char *);

int db_nick_set_string(unsigned int, const char *, const char *);
int db_nick_set_number(unsigned int, const char *, const unsigned long);
char *db_nick_get_string(unsigned int, const char *);

int db_set_founder(const char *, const char *);
int db_set_successor(const char *, const char *);

int db_list_add(const char *, unsigned int, const char *);
void *db_list_first(const char *, unsigned int, struct AccessEntry *);
void *db_list_next(void *, struct AccessEntry *);
void db_list_done(void *);
int db_list_del(const char *, unsigned int, const char *);
int db_list_del_index(const char *, unsigned int, unsigned int);

int db_link_nicks(unsigned int, unsigned int);
int db_is_linked(const char *);
struct Nick *db_unlink_nick(const char *);

#endif
