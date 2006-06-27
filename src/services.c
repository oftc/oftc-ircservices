#include "stdinc.h"
#include "conf.h"
#include "conf/conf.h"

struct Client me;

int main(int argc, char *argv[])
{
  memset(&ServicesInfo, 0, sizeof(ServicesInfo));

  libio_init(FALSE);

  memset(&me, 0, sizeof(me));

  iorecv_cb = register_callback("iorecv", iorecv_default);
  connected_cb = register_callback("server connected", server_connected);
  iosend_cb = register_callback("iosend", iosend_default);
      
  db_init();
  channel_init();
  init_conf();

  read_services_conf(TRUE);

  printf("Services starting with name %s description %s sid %s\n",
      me.name, me.info, me.id);

  db_load_driver();
#ifndef STATIC_MODULES
  if(chdir(MODPATH))
  {
    printf("Could not load core modules. Terminating!");
    exit(EXIT_FAILURE);
  }

  boot_modules(1);
  /* Go back to DPATH after checking to see if we can chdir to MODPATH */
  chdir(DPATH);
#else
  load_all_modules(1);
#endif


  connect_server();

  for(;;)
  {
    comm_select();
    send_queued_all();
  }

  return 0;
}

void
services_die(const char *msg, int rboot)
{
  printf("Dying: %s\n", msg);
  exit(rboot);
}
