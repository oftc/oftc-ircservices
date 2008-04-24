#include "Python.h"
#include "stdinc.h"
#include "libpython_module.h"
#include "structmember.h"
#include "interface.h"
#include "msg.h"

static void Service_dealloc(Service *);
static int Service_init(Service *, PyObject *, PyObject *);
static PyObject *Service_new(PyTypeObject *, PyObject *, PyObject *);
static PyObject *Service_register(Service *, PyObject *);
static PyObject *Service_do_help(Service *, PyObject *);

static void m_generic(struct Service *, struct Client *, int, char **);

static PyMethodDef ServiceModule_methods[] = {{NULL}};
static PyMemberDef Service_members[] = 
{
  {"cservice", T_OBJECT_EX, offsetof(Service, cservice), 0, "C Service struct"},
  {"client", T_OBJECT_EX, offsetof(Service, client), 0, "C Client struct"},
  {NULL}
};
static PyMethodDef Service_methods[] = 
{
  {"register", (PyCFunction)Service_register, METH_VARARGS, "Register a Service with the services core"},
  {"do_help", (PyCFunction)Service_do_help, METH_VARARGS, "Process help"},
  {NULL}
};

static PyTypeObject ServiceType = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "ServiceModule.Service",
  sizeof(Service),
  0,
  (destructor)Service_dealloc,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
  "Service objects",
  0,
  0,
  0,
  0,
  0,
  0,
  Service_methods,
  Service_members,
  0,
  0,
  0,
  0,
  0,
  0,
  (initproc)Service_init,
  0,
  Service_new,
};

static void
Service_dealloc(Service *self)
{
  Py_XDECREF(self->cservice);
  self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Service_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  Service *self;

  self = (Service *)type->tp_alloc(type, 0);

  return (PyObject *)self;
}

static int
Service_init(Service *self, PyObject *args, PyObject *kwds)
{
  PyObject *cservice = NULL, *client = NULL, *tmp;
  static char *kwlist[] = { "cservice", NULL };
  
  ilog(L_DEBUG, "INIT, init");

  if(!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, &cservice))
    return -1;

  if(cservice)
  {
    tmp = self->cservice;
    Py_INCREF(cservice);
    self->cservice = cservice;
    Py_XDECREF(tmp);
  }

  if(client)
  {
    tmp = self->client;
    Py_INCREF(client);
    self->client = client;
    Py_XDECREF(tmp);
  }
  return 0;
}

static PyObject *
Service_register(Service *self, PyObject *args)
{
  struct ServiceMessage *msg;
  struct Service *service;
  struct Client *client;
  PyObject *list;
  int i, size;

  if(!PyTuple_Check(args))
    return NULL;

  size = PyTuple_Size(args);

  service = (struct Service *)PyCObject_AsVoidPtr(self->cservice);

  for(i = 0; i < size; i++)
  {
    list = PyTuple_GetItem(args, i);
    if(list == NULL)
      return NULL;

    msg = MyMalloc(sizeof(struct ServiceMessage));
    if(!PyArg_ParseTuple(list, "sIIIIII:register", &msg->cmd, &msg->parameters,
          &msg->maxpara, &msg->flags, &msg->access, &msg->help_short,
          &msg->help_long))
    {
      MyFree(msg);
      return NULL;
    }
    msg->handler = m_generic;
    mod_add_servcmd(&service->msg_tree, msg);
  }
  
  client = introduce_client(service->name, service->name, TRUE);
  self->client = (PyObject*)PClient_from_client(client);

  return Py_None;
}

static PyObject *
Service_do_help(Service *self, PyObject *args)
{
  ilog(L_DEBUG, "Service_do_help!");
  return Py_None;
}

PyObject *
init_python_servicemodule()
{
  PyObject *m;

  if(PyType_Ready(&ServiceType) < 0)
    return NULL;

  m = Py_InitModule3("ServiceModule", ServiceModule_methods,
      "base class for Service modules.");

  if(m == NULL)
    return NULL;

  Py_INCREF(&ServiceType);
  PyModule_AddObject(m, "Service", (PyObject *)&ServiceType);

  return m;
}

static void
m_generic(struct Service *service, struct Client *client, int parc, 
    char *parv[])
{
  char *command = service->last_command;
  PyObject *pservice, *func, *args, *strargs, *value;
  PClient *pclient;

  pservice = service->data;
  strupper(command);

  func = PyObject_GetAttrString(pservice, command);
  if(func && PyCallable_Check(func))
  {
    int i;

    args = PyTuple_New(2);
    strargs = PyTuple_New(parc+1);
    for(i = 0; i <= parc; i++)
    {
      value = PyString_FromString(parv[i]);
      if(value == NULL)
      {
        Py_DECREF(args);
        PyErr_Print();
        return;
      }
      PyTuple_SetItem(strargs, i, value);
    }

    pclient = PClient_from_client(client);
    PyTuple_SetItem(args, 0, (PyObject*)pclient);
    PyTuple_SetItem(args, 1, strargs);
    value = PyObject_CallObject(func, args);

    Py_DECREF(strargs);
    Py_DECREF(args);

    if(value != NULL)
      Py_DECREF(value);
    else
    {
      Py_DECREF(func);
      PyErr_Print();
    }
  }
  else
  {
    PyErr_Print();
  }
}
