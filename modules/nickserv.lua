NickServ = {};

function NickServ:init()
  self.service_name = "NickServ"
  self.handlers = {
    help = {
      name = "help",
      func = self.help,
      help_short = 7,
      help_long = 8,
    },
    register = {
      name = "register",
      func = self.register,
      help_short = 9,
      help_long = 10
    }
  }

  for _, command in pairs(self.handlers) do 
    register_command(self.service_name, command.name) 
  end

  load_language(self.service_name, "nickserv.en")
end

function NickServ:handle_command(client, cmd, param)
  local params = {}
  local i = 0
  print("Handling message ".. cmd.. " from ".. client.name)

  for w in string.gmatch(param, "(%a+)") do
    params[i] = w
    i = i+1
  end
  self.handlers[cmd].func(self, client, params)
end

function NickServ:help(client, param)
  if(param[0] == nil or param[0] == "" or self.handlers[param[0]] == nil) then
    reply_user(self.service_name, client, _L(self.service_name, client, self.handlers["help"].help_long)) 
  else
    reply_user(self.service_name, client, _L(self.service_name, client, self.handlers[param[0]].help_short))
  end
end

function NickServ:register(c, param)
  local n = nick()

  if(c.registered) then
    reply_user(self.service_name, _L(self.service_name, c, 1))
    return
  end
  reply_user(self.service_name, c, "Yeah, right.  You wish.")
end

register_module("NickServ")
