/*
 *  oftc-ircservices: an exstensible and flexible IRC Services package
 *  lua_module.c: Interface to run LUA scripts
 *
 *  Copyright (C) 2006 Stuart Walsh and the OFTC Coding department
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 *
 *  $Id$
 */


#include "stdinc.h"
#include <lua5.1/lua.h>
#include <lua5.1/lualib.h>
#include <lua5.1/lauxlib.h>

static lua_State *L;
static void m_lua(struct Service *, struct Client *, int, char *[]);
static int client_to_string(lua_State *);
static int client_set(lua_State *);
static int client_get(lua_State *);
static int nick_get(lua_State *);
static int nick_set(lua_State *);
static int nick_to_string(lua_State *);
static int nick_new(lua_State *);
static struct Client *check_client(lua_State *L, int index);

static const struct luaL_reg client_m[] = {
  {"__newindex", client_set},
  {"__index", client_get},
  {"__tostring", client_to_string},
  {NULL, NULL}
};

static const struct luaL_reg nick_m[] = {
  {"__newindex", nick_set},
  {"__index", nick_get},
  {"__tostring", nick_to_string},
  {NULL, NULL}
};

static const struct luaL_reg service_f[] = {
  {"_L", lua_L}, 
  {"load_language", lua_load_language},
  {"register_command", lua_register_command},
  {"reply",  lua_reply_user},
  {NULL, NULL}
};

static const struct luaL_reg service_m[] = {
  {NULL, NULL}
};

static void
m_lua(struct Service *service, struct Client *client, int parc, char *parv[])
{
  char param[IRC_BUFSIZE+1];
  struct Client *ptr;

  memset(param, 0, sizeof(param));
  
  lua_getfield(L, LUA_GLOBALSINDEX, service->name);
  lua_getfield(L, -1, "handle_command");
  lua_getfield(L, LUA_GLOBALSINDEX, service->name);
  ptr = (struct Client *)lua_newuserdata(L, sizeof(struct Client));
  memcpy(ptr, client, sizeof(struct Client));

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
  struct Service *lua_service;

  lua_service = make_service(service_name);
  dlinkAdd(lua_service, &lua_service->node, &services_list);
  hash_add_service(lua_service);
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

  introduce_service(lua_service);
  
  return 0;
}

static int
register_lua_command(lua_State *L)
{ 
  struct ServiceMessage *handler_msgtab;
  struct Service *lua_service = check_service(L, 1);;
  char *command = lua_tolstring(L, 2, NULL); 
  int i;

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

static int
lua_reply_user(lua_State *L)
{
  char *message, *service;
  struct Service *lua_service = check_service(L, 1);
  struct Client *client = check_client(L, 2);

  message = luaL_checkstring(L, 3);
  
  reply_user(lua_service, client, message);

  return 0;
}

static int
lua_load_language(lua_State *L)
{
  struct Service *lua_service = check_service(L, 1);
  char *language = luaL_checkstring(L, 2);
  
  load_language(lua_service, language);

  return 0;
}

static int
lua_L(lua_State *L)
{
  struct Client *client;
  struct Service *lua_service = check_service(L, 1);
  int message;

  client = (struct Client*) luaL_checkudata(L, 2, "OFTC.client");
  message = luaL_checkinteger(L, 3);

  lua_pushstring(L, _L(lua_service, client, message));

  return 1;
}

static int
lua_register_client_struct(lua_State *L)
{
  luaL_newmetatable(L, "OFTC.client");
  luaL_register(L, NULL, client_m);

  return 0;
}

static int
lua_register_nick_struct(lua_State *L)
{
  luaL_newmetatable(L, "OFTC.nick");
  luaL_register(L, NULL, nick_m);

  lua_register(L, "nick", nick_new);

  return 0;
}

static struct Service *
check_service(lua_State *L, int index)
{
  void *ud = luaL_checkudata(L, index, "OFTC.service");
  luaL_argcheck(L, ud != NULL, index, "'service' expected");

  return (struct Service *)ud;
}

static int
service_new(lua_State *L)
{
  struct Service *service = 
    (struct Service*)lua_newuserdata(L, sizeof(struct Service));
  char *service_name = luaL_checkstring(L, 1);

  memset(service, 0, sizeof(struct Service));

  luaL_getmetatable(L, "OFTC.service");
  lua_setmetatable(L, -2);
  
  

  strlcpy(service->name, service_name, sizeof(service->name));

  dlinkAdd(service, &service->node, &services_list);
  hash_add_service(service);

  introduce_service(service);

  return 1;
}

static struct Nick *
check_nick(lua_State *L, int index)
{
  void *ud = luaL_checkudata(L, index, "OFTC.nick");
  luaL_argcheck(L, ud != NULL, index, "'nick' expected");

  return (struct Nick *)ud;
}

static int
nick_get(lua_State *L)
{
  struct Nick *nick = check_nick(L, 1);
  char *index = luaL_checkstring(L, 2);

  printf("pushing error for: %s\n", index);

  lua_pushfstring(L, "index %s is not supported", index);
  lua_error(L);

  return 1;
}

static int
nick_set(lua_State *L)
{
  struct Nick *nick = check_nick(L, 1);
  char *index = luaL_checkstring(L, 2);

  lua_pushfstring(L, "index %s is not supported", index);
  lua_error(L);

  return 0;
}

static int
nick_to_string(lua_State *L)
{
  struct Nick *nick = check_nick(L, 1);
  lua_pushfstring(L, "Nick: %p", nick);

  return 1;
}
             
static int
nick_new(lua_State *L)
{
  struct Nick *nick = (struct Nick*)lua_newuserdata(L, sizeof(struct Nick));

  luaL_getmetatable(L, "OFTC.nick");
  lua_setmetatable(L, -2);

  return 1;  
}

static struct Client *
check_client(lua_State *L, int index)
{
  void *ud = luaL_checkudata(L, index, "OFTC.client");
  luaL_argcheck(L, ud != NULL, index, "'client' expected");

  return (struct Client *)ud;
}

static int
client_get(lua_State *L)
{
  struct Client *client = check_client(L, 1);
  char *index = luaL_checkstring(L, 2);

  if(strcmp(index, "name") == 0)
    lua_pushstring(L, client->name);
  else if(strcmp(index, "registered") == 0)
    lua_pushboolean(L, IsRegistered(client));
  else
  { 
    lua_pushfstring(L, "index %s is not supported", index);
    lua_error(L);
  }
  
  return 1;
}

static int
client_set(lua_State *L)
{
  struct Client *client = check_client(L, 1);
  
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

static int
client_to_string(lua_State *L)
{
  struct Client *client = check_client(L, 1);
  lua_pushfstring(L, "Client: %p", client);

  return 1;
}

LUALIB_API int luaopen_oftc(lua_State *L)
{
  luaL_newmetatable(L, "OFTC");

  lua_register_service(L);
  lua_register_client_struct(L);
  lua_register_nick_struct(L);
}

void
init_lua()
{
  L = lua_open();
  
  luaL_openlibs(L);

  /* This can be removed when its made a module and loaded from LUA */
  luaopen_oftc(L);

}
