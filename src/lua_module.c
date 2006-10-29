#include "stdinc.h"
#include <lua5.1/lua.h>
#include <lua5.1/lualib.h>
#include <lua5.1/lauxlib.h>

static lua_State *L;
static void m_lua(struct Service *, struct Client *, int, char *[]);
struct Service *lua_service;
int client_to_string(lua_State *);
int set_client(lua_State *);
int get_client(lua_State *);

static const struct luaL_reg client_m[] = {
  {"set", set_client},
  {"get", get_client},
  {"__tostring", client_to_string},
  {NULL, NULL}
};

static void
m_lua(struct Service *service, struct Client *client, int parc, char *parv[])
{
  char param[IRC_BUFSIZE+1];
  struct Client **ptr;

  memset(param, 0, sizeof(param));
  
  lua_getfield(L, LUA_GLOBALSINDEX, service->name);
  lua_getfield(L, -1, "handle_command");
  lua_getfield(L, LUA_GLOBALSINDEX, service->name);
  ptr = (struct Client **)lua_newuserdata(L, sizeof(struct Client));
  *ptr = client;

  luaL_getmetatable(L, "OFTC.client");
  lua_setmetatable(L, -2);
  
  lua_pushstring(L, service->last_command);
  
  join_params(param, parc, &parv[1]);
  lua_pushstring(L, param);
  
  if(lua_pcall(L, 4, 0, 0))
  {
    printf("m_lua: LUA ERROR: %s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
    reply_user(service, client, "You broke teh lua.");
  }
}

static int 
register_lua_module(lua_State *L)
{
  int n = lua_gettop(L);
  char *service_name = lua_tolstring(L, 1, NULL);

  lua_service = make_service(service_name);
  /* 
   * find the object with the same name as the service and call its init
   * function.
   */
  lua_getfield(L, LUA_GLOBALSINDEX, service_name);
  lua_getfield(L, -1, "init");
  lua_getfield(L, LUA_GLOBALSINDEX, service_name);
  if(lua_pcall(L, 1, 0, 0))
  {
    printf("register_lua_module: LUA_ERROR: %s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
    /* XXX Unregister any sucessfully registered command handlers here. */
    MyFree(lua_service);
    lua_service = NULL;
    return 0;
  }

  dlinkAdd(lua_service, &lua_service->node, &services_list);
  hash_add_service(lua_service);
  introduce_service(lua_service);
  
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

  return 0;
}

int
load_lua_module(const char *name, const char *dir, const char *fname)
{
  int status;
  char path[PATH_MAX];

  snprintf(path, sizeof(path), "%s/%s", dir, fname);
  printf("Loading LUA module: %s\n", path);
  if(luaL_dofile(L, path))
  {
    printf("Failed to load LUA module: %s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
    return 0;
  }

  return 1;
}

int
lua_reply_user(lua_State *L)
{
  char *message;
  struct Client *client = check_client(L);

  message = lua_tolstring(L, 2, NULL);
  
  reply_user(lua_service, client, message);

  return 0;
}

int
lua_load_language(lua_State *L)
{
  char *language = lua_tolstring(L, 1, NULL);
  load_language(lua_service, language);

  return 0;
}

int
lua_L(lua_State *L)
{
  struct Client *client;
  int message;

  client = (struct Client*) lua_touserdata(L, 1);
  message = lua_tointeger(L, 2);

  lua_pushstring(L, _L(lua_service, client, message));

  return 1;
}

int
lua_register_client_struct(lua_State *L)
{
  luaL_newmetatable(L, "OFTC.client");
  luaL_openlib(L, "client", client_m, 0);

  lua_pushstring(L, "__index");
  lua_pushstring(L, "get");
  lua_gettable(L, 2);
  lua_settable(L, 1);

  lua_pushstring(L, "__newindex");
  lua_pushstring(L, "set");

  lua_gettable(L, 2);
  lua_settable(L, 1);

  return 0;
}

static struct Client *
check_client(lua_State *L)
{
  void *ud = luaL_checkudata(L, 1, "OFTC.client");
  struct Client **ptr = (struct Client **)ud;
  luaL_argcheck(L, ud != NULL, 1, "'client' expected");

  return *ptr;
}

int
get_client(lua_State *L)
{
  struct Client *client = check_client(L);
  char *index = luaL_checkstring(L, 2);

  if(strcmp(index, "name") == 0)
    lua_pushstring(L, client->name);
  else
  { 
    lua_pushfstring(L, "index %s is not supported", index);
    lua_error(L);
  }
  
  return 1;
}

int
set_client(lua_State *L)
{
  struct Client *client = check_client(L);
  
  if(strcmp(index, "name") == 0)
  {
    strlcpy(client->name, luaL_checkstring(L, 1), sizeof(client->name));
  }
  else
  {
    lua_pushfstring(L, "index %s is not supported", index);
    lua_error(L);
  }
  printf("setclient says hi.\n");
  return 0;
}

int
client_to_string(lua_State *L)
{
  struct Client *client = check_client(L);
  lua_pushfstring(L, "Client: %p", client);
}

void
init_lua()
{
  L = lua_open();
  
  luaL_openlibs(L);

  lua_register(L, "_L", lua_L);
  lua_register(L, "load_language", lua_load_language);
  lua_register(L, "register_module", register_lua_module);
  lua_register(L, "register_command", register_lua_command);
  lua_register(L, "reply_user", lua_reply_user);

  lua_register_client_struct(L);
}
