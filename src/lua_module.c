#include "stdinc.h"
#include <lua5.1/lua.h>
#include <lua5.1/lualib.h>
#include <lua5.1/lauxlib.h>

static lua_State *L;
static void m_lua(struct Service *, struct Client *, int, char *[]);
struct Service *lua_service;

static void
m_lua(struct Service *service, struct Client *client, int parc, char *parv[])
{
  lua_getfield(L, LUA_GLOBALSINDEX, service->name);
  lua_getfield(L, -1, "handle_command");
  lua_getfield(L, LUA_GLOBALSINDEX, service->name);
  lua_pushlightuserdata(L, client);
  lua_pushstring(L, service->last_command);
  lua_call(L, 3, 0);
}

static int register_lua_module(lua_State *L)
{
  int n = lua_gettop(L);
  char *service_name = lua_tolstring(L, 1, NULL);

  lua_service = make_service(service_name);
  dlinkAdd(lua_service, &lua_service->node, &services_list);
  hash_add_service(lua_service);
  introduce_service(lua_service);

  /* 
   * find the object with the same name as the service and call its init
   * function.
   */
  lua_getfield(L, LUA_GLOBALSINDEX, service_name);
  lua_getfield(L, -1, "init");
  lua_getfield(L, LUA_GLOBALSINDEX, service_name);
  lua_call(L, 1, 0);
  
  return 0;
}

int
register_lua_command(lua_State *L)
{ 
  struct ServiceMessage *handler_msgtab;
  char *command = lua_tolstring(L, 1, NULL); 
  int i;
  int n = lua_gettop(L);

  handler_msgtab = MyMalloc(sizeof(struct ServiceMessage));
  
  handler_msgtab->cmd = command;
  for(i = 0; i < SERVICES_LAST_HANDLER_TYPE; i++)
    handler_msgtab->handlers[i] = m_lua;

  mod_add_servcmd(&lua_service->msg_tree, handler_msgtab);
}

int
load_lua_module(const char *name, const char *dir, const char *fname)
{
  int status;
  char path[PATH_MAX];

  snprintf(path, sizeof(path), "%s/%s", dir, fname);
  printf("Loading LUA module: %s\n", path);
  if(luaL_dofile(L, path))
    return 0;

  return 1;
}

int
lua_reply_user(lua_State *L)
{
  char *message;
  struct Client *client;

  client = (struct Client*) lua_touserdata(L, 1);
  message = lua_tolstring(L, 2, NULL);
  
  reply_user(lua_service, client, message);
}

void
init_lua()
{
  L = lua_open();
  
  luaL_openlibs(L);

  lua_register(L, "register_module", register_lua_module);
  lua_register(L, "register_command", register_lua_command);
  lua_register(L, "reply_user", lua_reply_user);
}
