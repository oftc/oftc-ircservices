#include "stdinc.h"

services_info_t services_info;
client_t me;

int main(int argc, char *argv[])
{
  memset(&services_info, 0, sizeof(services_info));

  libio_init(FALSE);

  memset(&me, 0, sizeof(me));

  iorecv_cb = register_callback("iorecv", iorecv_default);
  //iosend_cb = register_callback("iosend", iosend_default);
      
  db_init();

  read_services_conf(TRUE);

  printf("Services starting with name %s description %s sid %s\n",
      services_info.name, services_info.description, services_info.sid);

  db_load_driver();

  connect_server((connection_conf_t*)connection_confs.head->data);

  for(;;)
  {
    comm_select();
  }

  return 0;
}
