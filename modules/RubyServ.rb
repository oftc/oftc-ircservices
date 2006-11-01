class RubyServ < ServiceModule
	def initialize
		service_name("RubyServ")
		register(["HELP", "SAY"])
		add_cmode_hook('cmode')
		add_umode_hook('umode')
	end
	def HELP(client, parv = [])
		puts "RubyServ::Help"
	end
	def SAY(client, parv = [])
		parv.shift
		message = parv.join(" ")
		reply_user(client, client.name + " Said: "+message)
	end
	def umode(clientp, sourcep, params)
		puts "Made it to UMODE"
		puts "UMODE clientp.name: "+ clientp.name + " sourcep.name: "+ sourcep.name
		puts params.join(" ")
	end
	def cmode(clientp, sourcep, channel, params)
		puts "Made it to CMODE"
		puts "CMODE clientp.name: "+ clientp.name + " sourcep.name: "+ sourcep.name + " in channel.name: " + channel.name
		puts params.join(" ")
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
