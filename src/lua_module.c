#include "stdinc.h"
#include <lua5.1/lua.h>
#include <lua5.1/lualib.h>
#include <lua5.1/lauxlib.h>

static lua_State *L;

static int register_lua_module(lua_State *L)
{
  int n = lua_gettop(L);
  struct Service *lua_service;
  char *service_name = lua_tolstring(L, 1, NULL);

  printf("register_lua_module: Called with %d args and %s as a name\n",
      n, service_name);

  lua_service = make_service(service_name);
  dlinkAdd(lua_service, &lua_service->node, &services_list);
  hash_add_service(lua_service);
  introduce_service(lua_service);
  
  return 0;
}

int
load_lua_module(const char *name, const char *dir, const char *fname)
{
  int status;
  char path[PATH_MAX];

  snprintf(path, sizeof(path), "%s/%s", dir, fname);
  printf("Loading LUA  module: %s\n", path);
  if(luaL_dofile(L, path))
    return 0;

  return 1;
}

void
init_lua()
{
  L = lua_open();
  
  luaL_openlibs(L);

  lua_register(L, "register_lua_module", register_lua_module);
}
