#include "stdinc.h"
#include <dbi/dbi.h>

database_info_t database_info;

void
db_init()
{
  int num_drivers = dbi_initialize(NULL);

  memset(&database_info, 0, sizeof(database_info));

  if(num_drivers >= 0)
    printf("libdbi: loaded: %d drivers available.\n", num_drivers);
  else
  {
    printf("libdbi: Error loading libdbi.\n");
    exit(-1);
  }

  printf("libdbi: version: %s\n", dbi_version());
}

void
db_load_driver()
{
  dbi_conn *dbconn;

  dbconn = dbi_conn_new(database_info.driver);
  if(dbconn == NULL)
  {
    printf("libdbi: Error loading database driver %s\n", database_info.driver);
    exit(-1);
  }

  printf("libdbi: Driver %s loaded\n", database_info.driver);
}
