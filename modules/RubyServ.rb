class RubyServ
	def initialize
		Oftc::ModuleServer.register('RubyServ', ["HELP", "SAY"])
	end
	def RubyServ.HELP(client, parv = [])
		puts "RubyServ::Help"
	end
	def RubyServ.SAY(client, parv = [])
		message = parv.join(" ")
		Oftc::ModuleServer.reply_user('RubyServ', client, "You Said: "+message)
	end
end

class ClientStruct
	def initialize
	end
end
