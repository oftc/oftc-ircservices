#include "stdinc.h"

services_info_t services_info;
client_t me;

int main(int argc, char *argv[])
{
  memset(&services_info, 0, sizeof(services_info));

  libio_init(FALSE);

  memset(&me, 0, sizeof(me));

  iorecv_cb = register_callback("iorecv", iorecv_default);
  connected_cb = register_callback("server connected", server_connected);
  iosend_cb = register_callback("iosend", iosend_default);
      
  db_init();

  read_services_conf(TRUE);
  strlcpy(me.id, services_info.sid, sizeof(me.id));
  strlcpy(me.name, services_info.name, sizeof(me.name));

  printf("Services starting with name %s description %s sid %s\n",
      services_info.name, services_info.description, services_info.sid);

  db_load_driver();
#ifndef STATIC_MODULES
  if(chdir(MODPATH))
  {
    printf(MODPATH);
    printf("Could not load core modules. Terminating!");
    exit(EXIT_FAILURE);
  }

  load_all_modules(1);
  load_conf_modules();
  load_core_modules(1);
  /* Go back to DPATH after checking to see if we can chdir to MODPATH */
#else
  load_all_modules(1);
#endif


  connect_server((connection_conf_t*)connection_confs.head->data);

  for(;;)
  {
    comm_select();
  }

  return 0;
}
