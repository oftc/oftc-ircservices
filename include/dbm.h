#ifndef DBMH
#define DBMH

void init_db();
void db_load_driver();
struct Nick *db_find_nick(const char *);
struct Nick *db_register_nick(const char *, const char *, const char *);
int db_set_language(struct Client *, int);
int db_nick_set_string(unsigned int, const char *, const char *);
char *db_nick_get_string(unsigned int, const char *);
struct RegChannel *db_find_chan(const char *);
int db_register_chan(struct Client *, char *);
int db_delete_nick(const char*);

#endif
