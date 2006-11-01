#ifndef DBMH
#define DBMH

void init_db();
void db_load_driver();
struct Nick *db_find_nick(const char *);
int db_register_nick(const char *, const char *, const char *);
int db_set_language(struct Client *, int);
int db_set_password(struct Client *, char *);
struct RegChannel *db_find_chan(const char *);
int db_register_chan(struct Client *, char *);
int db_delete_nick(struct Client *);
int db_set_url(struct Client *, char *);
int db_set_email(struct Client *, char *);
int db_set_cloak(struct Nick *, char *);

#endif
