LuaServ = {};


function LuaServ:init()
  LuaServ.handlers = {
    test = {
      name = "test",
      func = self.test
    },
    help = {
      name = "help",
      func = self.help
    }
  }

  for _, command in pairs(self.handlers) do 
    print (command.name)
    register_command(command.name) 
  end
end

function LuaServ:handle_command(client, cmd)
  self.handlers[cmd].func(self, client)
end

function LuaServ:test(client)
  reply_user(client, "Yup, it works.")
end

function LuaServ:help(client)
  reply_user(client, "HELP and TEST are the only two commands supported.");
end
  
register_module("LuaServ")
