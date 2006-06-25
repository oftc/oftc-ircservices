#include "stdinc.h"
#include <dbi/dbi.h>
#include "conf/conf.h"

void
db_init()
{
  int num_drivers = dbi_initialize(NULL);

  memset(&Database, 0, sizeof(Database));

  if(num_drivers >= 0)
    printf("db: loaded: %d drivers available.\n", num_drivers);
  else
  {
    printf("db: Error loading db.\n");
    exit(-1);
  }

  printf("db: version: %s\n", dbi_version());
}

void
db_load_driver()
{
  dbi_conn *dbconn;

  dbconn = dbi_conn_new(Database.driver);
  if(dbconn == NULL)
  {
    printf("db: Error loading database driver %s\n", Database.driver);
    exit(-1);
  }

  printf("db: Driver %s loaded\n", Database.driver);


  dbi_conn_set_option(dbconn, "username", Database.username);
  dbi_conn_set_option(dbconn, "password", Database.password);
  dbi_conn_set_option(dbconn, "dbname", Database.dbname);

  if(dbi_conn_connect(dbconn) < 0)
  {
    const char *error;
    dbi_conn_error(dbconn, &error);
    printf("db: Failed to connect to database %s\n", error);
   // exit(-1);
  }
  else
    printf("db: Database connection succeeded.\n");


  dbi_conn_close(dbconn);
  dbi_shutdown();
}
