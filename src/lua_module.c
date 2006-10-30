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
int client_to_string(lua_State *);
int client_set(lua_State *);
int client_get(lua_State *);
int nick_get(lua_State *);
int nick_set(lua_State *);
int nick_to_string(lua_State *);
int nick_new(lua_State *);
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

int
register_lua_command(lua_State *L)
{ 
  struct ServiceMessage *handler_msgtab;
  struct Service *lua_service;
  char *command = lua_tolstring(L, 2, NULL); 
  char *service = lua_tolstring(L, 1, NULL);
  int i;
  int n = lua_gettop(L);

  handler_msgtab = MyMalloc(sizeof(struct ServiceMessage));
  
  handler_msgtab->cmd = command;
  for(i = 0; i < SERVICES_LAST_HANDLER_TYPE; i++)
    handler_msgtab->handlers[i] = m_lua;

  lua_service = find_service(service);
  if(lua_service == NULL)
  {
    lua_pushfstring(L, "service '%s' does not exist.", service);
    lua_error(L);
    return 0;
  }

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
  char *message, *service;
  struct Client *client = check_client(L, 2);
  struct Service *lua_service;

  service = lua_tolstring(L, 1, NULL);
  message = lua_tolstring(L, 3, NULL);
  
  lua_service = find_service(service);
  
  reply_user(lua_service, client, message);

  return 0;
}

int
lua_load_language(lua_State *L)
{
  char *service = lua_tolstring(L, 1, NULL);
  char *language = lua_tolstring(L, 2, NULL);
  struct Service *lua_service;
  
  lua_service = find_service(service);
  
  load_language(lua_service, language);

  return 0;
}

int
lua_L(lua_State *L)
{
  struct Client *client;
  int message;
  char *service = lua_tolstring(L, 1, NULL);
  struct Service *lua_service;

  lua_service = find_service(service);
  
  client = (struct Client*) lua_touserdata(L, 2);
  message = lua_tointeger(L, 3);

  lua_pushstring(L, _L(lua_service, client, message));

  return 1;
}

int
lua_register_client_struct(lua_State *L)
{
  luaL_newmetatable(L, "OFTC.client");
  luaL_register(L, NULL, client_m);

  return 0;
}

int
lua_register_nick_struct(lua_State *L)
{
  luaL_newmetatable(L, "OFTC.nick");
  luaL_register(L, NULL, nick_m);

  lua_register(L, "nick", nick_new);

  return 0;
}

static struct Nick *
check_nick(lua_State *L, int index)
{
  void *ud = luaL_checkudata(L, index, "OFTC.nick");
  luaL_argcheck(L, ud != NULL, index, "'nick' expected");

  return ud;
}

int
nick_get(lua_State *L)
{
  struct Nick *nick = check_nick(L, 1);
  char *index = luaL_checkstring(L, 2);

  printf("pushing error for: %s\n", index);

  lua_pushfstring(L, "index %s is not supported", index);
  lua_error(L);

  return 1;
}

int
nick_set(lua_State *L)
{
  struct Nick *nick = check_nick(L, 1);
  char *index = luaL_checkstring(L, 2);

  lua_pushfstring(L, "index %s is not supported", index);
  lua_error(L);

  return 0;
}

int
nick_to_string(lua_State *L)
{
  struct Nick *nick = check_nick(L, 1);
  lua_pushfstring(L, "Nick: %p", nick);

  return 1;
}
             
int
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

int
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

int
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

int
client_to_string(lua_State *L)
{
  struct Client *client = check_client(L, 1);
  lua_pushfstring(L, "Client: %p", client);

  return 1;
}

LUALIB_API int luaopen_oftc(lua_State *L)
{
  luaL_newmetatable(L, "OFTC");

  lua_register_client_struct(L);
  lua_register_nick_struct(L);
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

  /* This can be removed when its made a module and loaded from LUA */
  luaopen_oftc(L);

}
