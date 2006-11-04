#ifndef DBMH
#define DBMH

void init_db();
void db_load_driver();

struct Nick *db_find_nick(const char *);
struct Nick *db_register_nick(const char *, const char *, const char *);
int db_delete_nick(const char*);

int db_register_chan(struct Client *, char *);
struct RegChannel *db_find_chan(const char *);

int db_nick_set_string(unsigned int, const char *, const char *);
int db_nick_set_number(unsigned int, const char *, const unsigned long);
char *db_nick_get_string(unsigned int, const char *);

#endif
