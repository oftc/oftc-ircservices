class RubyServ < ServiceModule
  def initialize
    service_name("RubyServ")
    register(["HELP", "SAY"])
    add_hook([
      [ServiceModule::CMODE_HOOK, 'cmode'],
      [ServiceModule::UMODE_HOOK, 'umode'],
      [ServiceModule::NEWUSR_HOOK, 'newuser']
    ])
  end
  def HELP(client, parv = [])
    log(ServiceModule::LOG_DEBUG, "RubyServ::Help")
  end
  def SAY(client, parv = [])
    parv.shift
    message = parv.join(" ")
    reply_user(client, client.name + " Said: "+message)
  end
  def umode(client, what, mode)
    log(ServiceModule::LOG_DEBUG, "UMODE client.name: %s what: %d mode: %08x" % [client.name, what, mode])
  end
  def cmode(clientp, sourcep, channel, params)
    log(ServiceModule::LOG_DEBUG, "Made it to CMODE clientp.name: %s sourcep.name: %s in channel.name: %s | %s" % [clientp.name, sourcep.name, channel.name, params.join(" ")])
  end
  def newuser(newuser)
    log(ServiceModule::LOG_DEBUG, "newuser.name: %s" % [newuser.name])
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
