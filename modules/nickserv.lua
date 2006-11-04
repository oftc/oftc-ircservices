NickServ = {};

function NickServ:init(name)
  self.handlers = {
    drop = {
      func = self.drop,
      help_short = 19,
      help_long = 20,
      num_args = 0,
    },
    help = {
      func = self.help,
      help_short = 7,
      help_long = 8,
      num_args = 0,
    },
    identify = {
      func = self.identify,
      help_short = 11,
      help_short = 12,
      num_args = 1,
    },
    register = {
      func = self.register,
      help_short = 9,
      help_long = 10,
      num_args = 2,
    },
    set = {
      func = self.set,
      help_short = 0,
      help_long = 0,
      num_args = 1,
      options = {
        email = {
          func = self.set_email,
        },
        enforce = {
          func = self.set_enforce,
        },
        language = {
          func = self.set_language
        },
        password = {
          func = self.set_password,
        },
        secure = {
          func = self.set_secure,
        },
        url = {
          func = self.set_url,
        },
      }
    }
  }

  self.s = register_service(name)

  for name, command in pairs(self.handlers) do 
    self.s:register_command(name) 
  end

  self.s:load_language("nickserv.en")
  self.s:load_language("nickserv.rude")
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
  local option = self.handlers["set"].options[param[0]]

  if not(self.n) then
    self.s:reply(client, self.s:_L(client, 21), client.name)
    return
  end

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

function NickServ:register(client, param)
  if(client.identified) then
    self.s:reply(client, self.s:_L(client, 1))
    return
  end
  
  if(nick.db_find(client.name)) then
    self.s:reply(client, self.s:_L(client, 1), client.name)
    return
  end

  self.n = nick.db_register(client.name, param[0], param[1])
  if(self.n) then
    self.s:reply(client, self.s:_L(client, 2), client.name)
  else
    self.s:reply(client, self.s:_L(client, 3), client.name)
  end
end

function NickServ:identify(client, param)
  err, self.n = nick.identify(client, param[0])
  if not(self.n) then
    if(err == 1) then
      self.s:reply(client, self.s:_L(client, 4), client.name)
      return
    elseif(err == 2) then
      self.s:reply(client, self.s:_L(client, 6), client.name)
      return
    end
    return
  end
  self.s:reply(client, self.s:_L(client, 5), client.name)
end

function NickServ:set_email(client, param)
  if(param[1] == "" or not param[1]) then
    self.s:reply(client, self.s:_L(client, 27), self.n.email)
  else 
    self.n:db_setemail(param[1])
    self.s:reply(client, self.s:_L(client, 26), self.n.email)
  end
end

NickServ:init("NickServ")
