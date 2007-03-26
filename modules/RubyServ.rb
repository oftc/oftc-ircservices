class RubyServ < ServiceModule
  def initialize()
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
    reply_user(client, "#{client.name}  Said: #{message}")
  end
  def umode(client, what, mode)
    log(ServiceModule::LOG_DEBUG, "UMODE client.name: #{client.name} what: #{what} mode: %08x" % [mode])
    log(ServiceModule::LOG_DEBUG, "client.nick.nick: #{client.nick.nick}")
  end
  def cmode(source, channel, dir, letter, param = "")
    log(ServiceModule::LOG_DEBUG, "Made it to CMODE source.name: #{source.name} channel.name: #{channel.name}")
    log(ServiceModule::LOG_DEBUG, "\t#{dir} #{letter} #{param}")
  end
  def newuser(newuser)
    log(ServiceModule::LOG_DEBUG, "newuser.name: #{newuser.name}")
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
