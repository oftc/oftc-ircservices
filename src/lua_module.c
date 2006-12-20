/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
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
static int service_new(lua_State *L);
static int lua_L(lua_State *);
static int lua_find_nick(lua_State *);
static int lua_register_nick(lua_State *);
static int lua_identify_nick(lua_State *);
static int lua_drop_nick(lua_State *);
static int lua_nick_set_email(lua_State *);
static int lua_nick_set_language(lua_State *);
static int lua_reply_user(lua_State *);
static int lua_load_language(lua_State *);
static int lua_language_name(lua_State *);
static int lua_register_command(lua_State *);
static struct Client *check_client(lua_State *, int);
static struct Nick *check_nick(lua_State *, int);
static struct Service *check_service(lua_State *, int);

static const struct luaL_reg client_m[] = {
  {"__newindex", client_set},
  {"__index", client_get},
  {"__tostring", client_to_string},
  {NULL, NULL}
};

static const struct luaL_reg nick_f[] = {
  {"__index", nick_get},
  {"__newindex", nick_set},
  {"__tostring", nick_to_string},
  {"db_drop", lua_drop_nick},
  {"db_find", lua_find_nick},
  {"db_register", lua_register_nick},
  {"identify", lua_identify_nick},
  {NULL, NULL}
};

static const struct luaL_reg nick_m[] = {
  {"__index", nick_get},
  {NULL, NULL}
};

static const struct luaL_reg service_f[] = {
  {"_L", lua_L}, 
  {"load_language", lua_load_language},
  {"language_name", lua_language_name},
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
    reply_user(service, service, client, 0, "You broke teh lua.");
  }
}

static int
lua_register_command(lua_State *L)
{ 
  struct ServiceMessage *handler_msgtab;
  struct Service *lua_service = check_service(L, 1);;
  const char *command = luaL_checkstring(L, 2); 

  handler_msgtab = MyMalloc(sizeof(struct ServiceMessage));
  
  handler_msgtab->cmd = command;
  handler_msgtab->handler = m_lua;

  mod_add_servcmd(&lua_service->msg_tree, handler_msgtab);

  return 0;
}

int
load_lua_module(const char *name, const char *dir, const char *fname)
{
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
lua_register_nick(lua_State *L)
{
  const char *nick, *password, *email;
  struct Nick *nick_p, *n;

  nick = luaL_checkstring(L, 1);
  password = luaL_checkstring(L, 2);
  email = luaL_checkstring(L, 3);

  /* XXX */
// XXX  nick_p = db_register_nick(nick, password, password, email);
  if(nick_p == NULL)
  {
    lua_pushnil(L);
    return 1;
  }

  n = (struct Nick*)lua_newuserdata(L, sizeof(struct Nick));
  memcpy(n, nick_p, sizeof(struct Nick));
  MyFree(nick_p);

  luaL_getmetatable(L, "OFTC.nick");
  lua_setmetatable(L, -2);

  return 1;
}

static int
lua_identify_nick(lua_State *L)
{
  struct Nick *nick;
  struct Client *client = check_client(L, 1);
//  const char *password = luaL_checkstring(L, 2);
  int error = 0;
/* XXX
  if((nick = db_find_nick(client->name)) == NULL)
    error = 1;
  else if(strncmp(nick->pass, servcrypt(password, nick->pass), 
          sizeof(nick->pass)) != 0)
  {
    MyFree(nick);
    error = 2;
  }
*/
  client->nickname = nick;

  identify_user(client);
  
  lua_pushinteger(L, 0);

  if(nick == NULL || error != 0)
  {
    lua_pushnil(L);
    return 2;
  }
 
  nick = client->nickname;
  client->nickname = (struct Nick*)lua_newuserdata(L, sizeof(struct Nick));
  memcpy(client->nickname, nick, sizeof(struct Nick));

  MyFree(nick);

  luaL_getmetatable(L, "OFTC.nick");
  lua_setmetatable(L, -2);
  
  return 2;
}

static int
lua_find_nick(lua_State *L)
{
  struct Nick *nick, *n;
  const char *nickname = luaL_checkstring(L, 1);

  nick = db_find_nick(nickname);
  if(nick == NULL)
  {
    lua_pushnil(L);
    return 1;
  }

  n = (struct Nick*)lua_newuserdata(L, sizeof(struct Nick));
  memcpy(n, nick, sizeof(struct Nick));
  MyFree(nick);

  luaL_getmetatable(L, "OFTC.nick");
  lua_setmetatable(L, -2);

  return 1;
}

static int
lua_drop_nick(lua_State *L)
{
  struct Client *client;
  const char *nick = luaL_checkstring(L, 1);

  // XXX if(db_delete_nick(nick) == 0)
  //{
  //  lua_pushboolean(L, FALSE);
  //  return 1;
  //}

  client = find_client(nick);
  ClearIdentified(client);

  lua_pushboolean(L, TRUE);

  return 1;
}

static int
lua_nick_set_email(lua_State *L)
{
  struct Nick *nick = check_nick(L, 1);
  const char *email = luaL_checkstring(L, 2);
 // XXX int ret = db_set_string("nickname", nick->id, "email", email);

  //if(ret == 0)
  {
    strlcpy(nick->email, email, sizeof(nick->email));
    lua_pushboolean(L, 1);
  }
 // else
    lua_pushboolean(L, 0);
  
  return 1;
}

static int
lua_nick_set_language(lua_State *L)
{
  struct Nick *nick = check_nick(L, 1);
  const int lang = luaL_checkinteger(L, 2);
// XXX  int ret = db_set_number("nickname", nick->id, "language", lang);

  //if(ret == 0)
  {
    nick->language = lang;
    lua_pushboolean(L, 1);
  }
 // else
    lua_pushboolean(L, 0);
  
  return 1;
}

static int
lua_reply_user(lua_State *L)
{
  const char *message, *param;
  struct Service *lua_service = check_service(L, 1);
  struct Client *client = check_client(L, 2);
  
  message = luaL_checkstring(L, 3);
  param = luaL_checkstring(L, 4);
  
  reply_user(lua_service, lua_service, client, 0, message, param);

  return 0;
}

static int
lua_load_language(lua_State *L)
{
  struct Service *lua_service = check_service(L, 1);
  const char *language = luaL_checkstring(L, 2);
  
  load_language(lua_service->languages, language);

  return 0;
}

static int
lua_language_name(lua_State *L)
{
  struct Service *lua_service = check_service(L, 1);
  const int language = luaL_checkinteger(L, 2);

  lua_pushstring(L, lua_service->languages[language].name);

  return 1;
}

static int
lua_L(lua_State *L)
{
  struct Client *client;
//  struct Service *lua_service = check_service(L, 1);
  int message;

  client = (struct Client*) luaL_checkudata(L, 2, "OFTC.client");
  message = luaL_checkinteger(L, 3);

//  lua_pushstring(L, _L(lua_service, client, message));

  return 1;
}

static int
lua_register_service(lua_State *L)
{
  luaL_newmetatable(L, "OFTC.service");
  lua_newtable(L);
  luaL_register(L, NULL, service_f);
  lua_setfield(L, -2, "__index");
  luaL_register(L, NULL, service_m);

  lua_register(L, "register_service", service_new);

  return 0;
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
  luaL_register(L, "nick", nick_f);

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
  const char *service_name = luaL_checkstring(L, 1);

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
  const char *index = luaL_checkstring(L, 2);

  if(strcmp(index, "email") == 0)
    lua_pushstring(L, nick->email);
  else if(strcmp(index, "language") == 0)
    lua_pushinteger(L, nick->language);
  else if(strcmp(index, "db_setemail") == 0)
    lua_pushcfunction(L, lua_nick_set_email);
  else if(strcmp(index, "db_setlanguage") == 0)
    lua_pushcfunction(L, lua_nick_set_language);
  else
  {
    lua_pushfstring(L, "index %s is not supported", index);
    lua_error(L);
  }

  return 1;
}

static int
nick_set(lua_State *L)
{
  struct Nick *nick = check_nick(L, 1);
  const char *index = luaL_checkstring(L, 2);

  nick = nick;

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
  const char *index = luaL_checkstring(L, 2);

  if(strcmp(index, "name") == 0)
    lua_pushstring(L, client->name);
  else if(strcmp(index, "identified") == 0)
    lua_pushboolean(L, IsIdentified(client));
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
  const char *index = luaL_checkstring(L, 2);
  
  if(strcmp(index, "name") == 0)
  {
    strlcpy(client->name, luaL_checkstring(L, 3), sizeof(client->name));
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

  return 0;
}

void
init_lua()
{
  L = lua_open();
  
  luaL_openlibs(L);

  /* This can be removed when its made a module and loaded from LUA */
  luaopen_oftc(L);

}
