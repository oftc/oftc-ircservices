/* TODO: add copyright block */

#ifndef INCLUDED_lua_module_h
#define INCLUDED_lua_module_h

void init_lua();
int load_lua_module(const char *name, const char *dir, const char *fname);

#endif /* INCLUDED_lua_module_h */
