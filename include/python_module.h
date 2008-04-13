#ifndef PYTHON_MODULE_H
#define PYTHON_MODULE_H

void init_python();
void cleanup_python();
int load_python_module(const char *, const char *, const char *);
int unload_python_module(const char *);

#endif
