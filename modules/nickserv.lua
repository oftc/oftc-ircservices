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
      "\002. Got " .. i.. ", wanted ".. self.handlers[cmd].num_args.. ".", "")
  else
    self.handlers[cmd].func(self, client, params)
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

NickServ:init("NickServ")
