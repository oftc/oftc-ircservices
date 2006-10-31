class RubyServ < ServiceModule
	def initialize
		service_name("RubyServ")
		register(["HELP", "SAY"])
	end
	def HELP(client, parv = [])
		puts "RubyServ::Help"
	end
	def SAY(client, parv = [])
		message = parv.join(" ")
		reply_user(client, client.name + " Said: "+message)
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
