NickServ = {};

function NickServ:init()
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
    register_command(command.name) 
  end

  load_language("nickserv.en")
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
    reply_user(client, _L(client, self.handlers["help"].help_long)) 
  else
    reply_user(client, _L(client, self.handlers[param[0]].help_short))
  end
end

function NickServ:register(client, param)
  reply_user(client, "Yeah, right.  You wish.")
end

register_module("NickServ")
