#ifndef LIBPYTHON_MODULE_H
#define LIBPYTHON_MODULE_H

typedef struct
{
  PyObject_HEAD
  PyObject *cservice;
  PyObject *client;
} Service;


void init_python_servicemodule();

#endif
