#include "Python.h"
#include "stdinc.h"
#include "libpython_module.h"
#include "structmember.h"

typedef struct 
{
  PyObject_HEAD
} Service;

static void Service_dealloc(Service *);
static int Service_init(Service *, PyObject *, PyObject *);
static PyObject *Service_new(PyTypeObject *, PyObject *, PyObject *);


static PyMemberDef Service_members[] = {{NULL}};
static PyMethodDef Service_methods[] = {{NULL}};
static PyMethodDef ServiceModule_methods[] = {{NULL}};

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
  ilog(L_DEBUG, "INIT, init");
  return 0;
}

void
init_python_servicemodule()
{
  PyObject *m;

  if(PyType_Ready(&ServiceType) < 0)
    return;

  m = Py_InitModule3("ServiceModule", ServiceModule_methods,
      "base class for Service modules.");

  if(m == NULL)
    return;

  Py_INCREF(&ServiceType);
  PyModule_AddObject(m, "Service", (PyObject *)&ServiceType);
}
