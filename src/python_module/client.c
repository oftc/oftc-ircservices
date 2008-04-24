#include "Python.h"
#include "stdinc.h"
#include "libpython_module.h"
#include "structmember.h"
#include "interface.h"
#include "client.h"

static void PClient_dealloc(PClient *);
static int PClient_init(PClient *, PyObject *, PyObject *);
static PyObject *PClient_new(PyTypeObject *, PyObject *, PyObject *);
static PyObject *PClient_getname(PClient *, void *);
static PyObject *PClient_gethost(PClient *, void *);
static PyObject *PClient_getrealhost(PClient *, void *);
static PyObject *PClient_getid(PClient *, void *);
static PyObject *PClient_getinfo(PClient *, void *);
static PyObject *PClient_getusername(PClient *, void *);
static PyObject *PClient_getctcp_version(PClient *, void *);
static int PClient_setname(PClient *, PyObject *, void *);
static int PClient_sethost(PClient *, PyObject *, void *);
static int PClient_setrealhost(PClient *, PyObject *, void *);
static int PClient_setid(PClient *, PyObject *, void *);
static int PClient_setinfo(PClient *, PyObject *, void *);
static int PClient_setusername(PClient *, PyObject *, void *);
static int PClient_setctcp_version(PClient *, PyObject *, void *);

static PyMemberDef PClient_members[] = 
{
  {"client", T_OBJECT_EX, offsetof(PClient, client), 0, "C PClient pointer"},
  {"nick", T_OBJECT_EX, offsetof(PClient, nick), 0, "C PClient nick"},
  {"from", T_OBJECT_EX, offsetof(PClient, from), 0, "C PClient from pointer"},
  {NULL}
};

static PyMethodDef PClient_methods[] = 
{
  {NULL}
};

static PyGetSetDef PClient_getseters[] = 
{
  {"name", (getter)PClient_getname, (setter)PClient_setname, "PClient name", NULL},
  {"host", (getter)PClient_gethost, (setter)PClient_sethost, "PClient host", NULL},
  {"realhost", (getter)PClient_getrealhost, (setter)PClient_setrealhost, "PClient realhost", NULL},
  {"id", (getter)PClient_getid, (setter)PClient_setid, "PClient id", NULL},
  {"info", (getter)PClient_getinfo, (setter)PClient_setinfo, "PClient info", NULL},
  {"username", (getter)PClient_getusername, (setter)PClient_setusername, "PClient username", NULL},
  {"ctcp_version", (getter)PClient_getctcp_version, (setter)PClient_setctcp_version, "PClient CTCP version", NULL},
  {NULL}  
};

static PyTypeObject PClientType = 
{
  PyObject_HEAD_INIT(NULL)
  0,
  "ServiceModule.Client",
  sizeof(PClient),
  0,
  (destructor)PClient_dealloc,
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
  "Client objects",
  0,
  0,
  0,
  0,
  0,
  0,
  PClient_methods,
  PClient_members,
  PClient_getseters,
  0,
  0,
  0,
  0,
  0,
  (initproc)PClient_init,
  0,
  PClient_new,
};

static void
PClient_dealloc(PClient *self)
{
  Py_XDECREF(self->client);
  Py_XDECREF(self->from);
  Py_XDECREF(self->nick);
  self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
PClient_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  Service *self;

  self = (Service *)type->tp_alloc(type, 0);
  return (PyObject *)self;
}

static int
PClient_init(PClient*self, PyObject *args, PyObject *kwds)
{
  PyObject *client, *from, *nick;
  PyObject *tmp;
  static char *kwlist[] = { "client", "from", "nick", NULL };

  client = from = nick = NULL;
  
  if(!PyArg_ParseTupleAndKeywords(args, kwds, "|OOO", kwlist, &client,
        &from, &nick))
    return -1;

  if(client)
  {
    tmp = self->client;
    Py_INCREF(client);
    self->client = client;
    Py_XDECREF(tmp);
  }

  if(from)
  {
    tmp = self->from;
    Py_INCREF(from);
    self->from = from;
    Py_XDECREF(tmp);
  }

  if(nick)
  {
    tmp = self->nick;
    Py_INCREF(nick);
    self->nick = nick;
    Py_XDECREF(tmp);
  }

  return 0;
}

static PyObject *
PClient_getname(PClient *self, void *closure)
{
  struct Client *client;

  client = (struct Client *)PyCObject_AsVoidPtr(self->client);

  return PyString_FromString(client->name);
}

static PyObject *
PClient_gethost(PClient *self, void *closure)
{
  struct Client *client;

  client = (struct Client *)PyCObject_AsVoidPtr(self->client);

  return PyString_FromString(client->host);
}

static PyObject *
PClient_getrealhost(PClient *self, void *closure)
{
  struct Client *client;

  client = (struct Client *)PyCObject_AsVoidPtr(self->client);

  return PyString_FromString(client->realhost);
}

static PyObject *
PClient_getid(PClient *self, void *closure)
{
  struct Client *client;

  client = (struct Client *)PyCObject_AsVoidPtr(self->client);

  return PyString_FromString(client->id);
}

static PyObject *
PClient_getinfo(PClient *self, void *closure)
{
  struct Client *client;

  client = (struct Client *)PyCObject_AsVoidPtr(self->client);

  return PyString_FromString(client->info);
}

static PyObject *
PClient_getusername(PClient *self, void *closure)
{
  struct Client *client;

  client = (struct Client *)PyCObject_AsVoidPtr(self->client);

  return PyString_FromString(client->username);
}

static PyObject *
PClient_getctcp_version(PClient *self, void *closure)
{
  struct Client *client;

  client = (struct Client *)PyCObject_AsVoidPtr(self->client);

  return PyString_FromString(client->ctcp_version);
}

static int
PClient_setname(PClient *self, PyObject *value, void *closure)
{
  struct Client *client;

  if(value == NULL)
  {
    PyErr_SetString(PyExc_TypeError, "name cannot be empty");
    return -1;
  }

  if(!PyString_Check(value))
  {
    PyErr_SetString(PyExc_TypeError, "name must be a string");
    return -1;
  }

  client = PyCObject_AsVoidPtr(self->client);
  strlcpy(client->name, PyString_AsString(value), sizeof(client->name));

  return 0;
}

static int
PClient_sethost(PClient *self, PyObject *value, void *closure)
{
  struct Client *client;

  if(value == NULL)
  {
    PyErr_SetString(PyExc_TypeError, "host cannot be empty");
    return -1;
  }

  if(!PyString_Check(value))
  {
    PyErr_SetString(PyExc_TypeError, "host must be a string");
    return -1;
  }

  client = PyCObject_AsVoidPtr(self->client);
  strlcpy(client->host, PyString_AsString(value), sizeof(client->host));

  return 0;
}

static int
PClient_setrealhost(PClient *self, PyObject *value, void *closure)
{
  struct Client *client;

  if(!PyString_Check(value))
  {
    PyErr_SetString(PyExc_TypeError, "realhost must be a string");
    return -1;
  }

  client = PyCObject_AsVoidPtr(self->client);
  if(value != NULL)
    strlcpy(client->realhost, PyString_AsString(value), sizeof(client->realhost));
  else
    memset(client->realhost, 0, sizeof(client->realhost));

  return 0;
}

static int
PClient_setid(PClient *self, PyObject *value, void *closure)
{
  struct Client *client;

  if(value == NULL)
  {
    PyErr_SetString(PyExc_TypeError, "id cannot be empty");
    return -1;
  }

  if(!PyString_Check(value))
  {
    PyErr_SetString(PyExc_TypeError, "id must be a string");
    return -1;
  }

  client = PyCObject_AsVoidPtr(self->client);
  strlcpy(client->id, PyString_AsString(value), sizeof(client->id));

  return 0;
}

static int
PClient_setinfo(PClient *self, PyObject *value, void *closure)
{
  struct Client *client;

  if(value == NULL)
  {
    PyErr_SetString(PyExc_TypeError, "info cannot be empty");
    return -1;
  }

  if(!PyString_Check(value))
  {
    PyErr_SetString(PyExc_TypeError, "info must be a string");
    return -1;
  }

  client = PyCObject_AsVoidPtr(self->client);
  strlcpy(client->info, PyString_AsString(value), sizeof(client->info));

  return 0;
}

static int
PClient_setusername(PClient *self, PyObject *value, void *closure)
{
  struct Client *client;

  if(value == NULL)
  {
    PyErr_SetString(PyExc_TypeError, "username cannot be empty");
    return -1;
  }

  if(!PyString_Check(value))
  {
    PyErr_SetString(PyExc_TypeError, "username must be a string");
    return -1;
  }

  client = PyCObject_AsVoidPtr(self->client);
  strlcpy(client->username, PyString_AsString(value), sizeof(client->username));

  return 0;
}

static int
PClient_setctcp_version(PClient *self, PyObject *value, void *closure)
{
  struct Client *client;

  if(value == NULL)
  {
    PyErr_SetString(PyExc_TypeError, "ctcp_version cannot be empty");
    return -1;
  }

  if(!PyString_Check(value))
  {
    PyErr_SetString(PyExc_TypeError, "ctcp_version must be a string");
    return -1;
  }

  client = PyCObject_AsVoidPtr(self->client);
  strlcpy(client->ctcp_version, PyString_AsString(value), sizeof(client->ctcp_version));

  return 0;
}


PClient *
PClient_from_client(struct Client *client)
{
  PyObject *pname, *module;
  PClient *class, *value;

  pname = PyString_FromString("ServiceModule");

  module = PyImport_Import(pname);
  Py_DECREF(pname);

  if(module == NULL)
    return NULL;

  class = (PClient *)PyObject_GetAttrString(module, "Client");
  if(class == NULL)
    return NULL;

  value = (PClient *)PyObject_CallObject((PyObject*)class, NULL);
  if(value == NULL)
  {
    Py_DECREF(class);
    return NULL;
  }

  value->client = PyCObject_FromVoidPtr(client, NULL);

  Py_DECREF(class);
  return value;
}

void
init_python_client(PyObject *module)
{
  if(PyType_Ready(&PClientType) < 0)
    return;

  if(module == NULL)
    return;

  Py_INCREF(&PClientType);
  PyModule_AddObject(module, "Client", (PyObject *)&PClientType);
}
