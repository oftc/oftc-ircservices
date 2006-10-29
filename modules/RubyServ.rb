class RubyServ
	def initialize
		Oftc::ModuleServer.register('RubyServ', ["HELP", "SAY"])
	end
	def RubyServ.HELP(client, parv = [])
		puts "RubyServ::Help"
	end
	def RubyServ.SAY(client, parv = [])
		message = parv.join(" ")
		Oftc::ModuleServer.reply_user('RubyServ', client, client.name + " Said: "+message)
	end
end

#this class is declared in C, but you can use it in Ruby
#everything is readonly
#class ClientStruct
#	def name
#	end
#	def host
#	end
#	def id
#	end
#	def info
#	end
#	def username
#	end
#	def umodes
#	end
#end
