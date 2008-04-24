#ifndef LIBPYTHON_MODULE_H
#define LIBPYTHON_MODULE_H

typedef struct
{
  PyObject_HEAD
  PyObject *cservice;
  PyObject *client;
} Service;

typedef struct
{
  PyObject_HEAD
  PyObject *client;
  PyObject *nick;
  PyObject *from;
} PClient;

PyObject *init_python_servicemodule();
void init_python_client(PyObject *);
PClient *PClient_from_client(struct Client *);

#endif
