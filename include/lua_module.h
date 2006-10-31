#ifndef LUA_MODULE_H
#define LUA_MODULE_H

void init_lua();
int load_lua_module(const char *name, const char *dir, const char *fname);

#endif
