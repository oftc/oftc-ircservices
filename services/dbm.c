#include "stdinc.h"
#include <dbi/dbi.h>

void
init_db()
{
  int num_drivers = dbi_initialize(NULL);

  if(num_drivers >= 0)
    printf("libdbi loaded: %d drivers available.\n", num_drivers);
  else
  {
    printf("Error loading libdbi.\n");
    exit(-1);
  }

  printf("libdbi version: %s\n", dbi_version());
}
