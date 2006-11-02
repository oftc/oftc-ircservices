NickServ = {};

function NickServ:init(name)
  self.handlers = {
    drop = {
      name = "drop",
      func = self.drop,
      help_short = 19,
      help_long = 20,
      num_args = 0,
    },
    help = {
      name = "help",
      func = self.help,
      help_short = 7,
      help_long = 8,
      num_args = 0,
    },
    register = {
      name = "register",
      func = self.register,
      help_short = 9,
      help_long = 10,
      num_args = 2,
    },
    set = {
      name = "set",
      func = self.set,
      help_short = 0,
      help_long = 0,
      num_args = 1,
      options = {
        email = {
          name = "email",
          func = self.set_email,
        }
      }
    }
  }

  self.s = register_service(name)

  for _, command in pairs(self.handlers) do 
    self.s:register_command(command.name) 
  end

  self.s:load_language("nickserv.en")
end

function NickServ:handle_command(client, cmd, param)
  local params = {}
  local i = 0
  print("Handling message ".. cmd.. " from ".. client.name)

  for w in string.gmatch(param, "(%S+)") do
    params[i] = w
    i = i+1
  end

  if(i < self.handlers[cmd].num_args) then
    self.s:reply(client, "Insufficient Parameters: \002".. cmd.. 
      "\002. Got " .. i.. ", wanted at least ".. self.handlers[cmd].num_args.. ".", "")
  else
    self.handlers[cmd].func(self, client, params)
  end
end

function NickServ:set(client, param)
  table.remove(param, 0)
  local option = self.handlers["set"].options[param[0]]

  if(option) then
    option.func(self, client, param) 
  else
    self.s:reply(client, "Unknown \002SET\002 option: %s.", param[0])
  end
end

function NickServ:drop(client, param)
  if(nick.db_drop(client.name)) then
    self.s:reply(client, self.s:_L(client, 24), client.name)
  else
    self.s:reply(client, self.s:_L(client, 25), client.name)
  end
end

function NickServ:help(client, param)
  if(param[0] == nil or param[0] == "" or self.handlers[param[0]] == nil) then
    self.s:reply(client, self.s:_L(client, self.handlers["help"].help_long), "") 
  else
    self.s:reply(client, self.s:_L(client, self.handlers[param[0]].help_short), "")
  end
end

function NickServ:register(c, param)
  local n
  
  if(c.registered) then
    self.s:reply(c, self.s:_L(c, 1))
    return
  end
  
  if(nick.db_find(c.name)) then
    self.s:reply(c, self.s:_L(c, 1), c.name)
    return
  end

  n = nick.db_register(c.name, param[0], param[1])
  if(n) then
    self.s:reply(c, self.s:_L(c, 2), c.name)
  else
    self.s:reply(c, self.s:_L(c, 3), c.name)
  end
end

function NickServ:set_email(client, param)
  print "hi"
end

NickServ:init("NickServ")
