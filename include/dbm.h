#ifndef DBMH
#define DBMH

void db_init();
void db_load_driver();
struct Nick *db_find_nick(const char *);
int db_register_nick(struct Client *, const char *, const char *);

#endif
