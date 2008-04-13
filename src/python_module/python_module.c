#include "Python.h"
#include "stdinc.h"
#include "conf/modules.h"
#include "libpython_module.h"

void
init_python()
{
  dlink_list *modpaths = get_modpaths();
  dlink_node *ptr;

  Py_Initialize();

  PyRun_SimpleString("import sys");

  DLINK_FOREACH(ptr, modpaths->head)
  {
    char str[PATH_MAX + 19 + 1]; /* path + "sys.path.append("")" */
    snprintf(str, sizeof(str)-1, "sys.path.append(\"%s\")", (char*)ptr->data);
    PyRun_SimpleString(str);
  }

  init_python_servicemodule();
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
  PyObject *pName, *pModule;

  chdir(dir);

  printf(getcwd(NULL, 0));

  snprintf(path, PATH_MAX, "%s/%s", dir, fname);

  ilog(L_DEBUG, "PYTHON INFO: Loading python module: %s", path);
  pName = PyString_FromString(name);

  pModule = PyImport_Import(pName);
  Py_DECREF(pName);

  chdir(DPATH);

  if(pModule != NULL)
  {
    ilog(L_DEBUG, "PYTHON INFO: Python module %s loaded.", path);
    return TRUE;
  }
  else
  {
    ilog(L_WARN, "PYTHON WARN: Error Loading python module %s", path);
    PyErr_Print();
    return -1;
  }
}

void
unload_python_module(const char *name)
{
}
