#ifndef DBMH
#define DBMH

typedef struct 
{
  char *driver;
  char *dbname;
  char *username;
  char *password;
} database_info_t;

extern database_info_t database_info;

void db_init();
void db_load_driver();

#endif
