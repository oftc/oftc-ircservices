#include "Python.h"
#include "stdinc.h"
#include "conf/modules.h"
#include "libpython_module.h"
#include "interface.h"
#include "hash.h"

void
init_python()
{
  dlink_list *modpaths = get_modpaths();
  dlink_node *ptr;
  PyObject *module;

  Py_Initialize();

  PyRun_SimpleString("import sys");

  DLINK_FOREACH(ptr, modpaths->head)
  {
    char str[PATH_MAX + 19 + 1]; /* path + "sys.path.append("")" */
    snprintf(str, sizeof(str)-1, "sys.path.append(\"%s\")", (char*)ptr->data);
    PyRun_SimpleString(str);
  }

  module = init_python_servicemodule();
  if(module == NULL)
  {
    PyErr_Print();
    return;
  }

  init_python_client(module);
}

void
cleanup_python()
{
  Py_Finalize();
}

int
load_python_module(const char *name, const char *dir, const char *fname)
{
  char path[PATH_MAX+1];
  PyObject *pname, *module, *value, *args;
  Service *class;
  struct Service *service;

  snprintf(path, PATH_MAX, "%s/%s", dir, fname);

  ilog(L_DEBUG, "PYTHON INFO: Loading python module: %s", path);
  pname = PyString_FromString(name);

  module = PyImport_Import(pname);
  Py_DECREF(pname);

  chdir(DPATH);

  if(module == NULL)
  {
    ilog(L_CRIT, "PYTHON ERR: Error Loading python module %s", path);
    PyErr_Print();
    return -1;
  }

  class = (Service *)PyObject_GetAttrString(module, (char*)name);
  if(class == NULL)
  {
    ilog(L_CRIT, "PYTHON ERR: Unable to find class named %s in module %s",
        name, path);
    PyErr_Print();
    return -1;
  }

  service = make_service((char*)name);
  args = PyTuple_New(1);
  PyTuple_SetItem(args, 0, PyCObject_FromVoidPtr((void *)service, NULL));
  value = PyObject_CallObject((PyObject*)class, args);
  if(value == NULL)
  {
    ilog(L_CRIT, "PYTHON ERR: Unable to call class %s in module %s",
          name, path);
    Py_DECREF(class);
    PyErr_Print();
    return -1;
  }
  
  ilog(L_DEBUG, "PYTHON INFO: Python module %s loaded.", path);

  dlinkAdd(service, &service->node, &services_list);
  hash_add_service(service);

  service->data = value;
  return 1;
}

void
unload_python_module(const char *name)
{
}
