class RubyServ < ServiceModule
  def initialize()
    service_name("RubyServ")
    register([
      #["COMMAND", PARAM_MIN, PARAM_MAX, FLAGS, ACCESS, HLP_SHORT, HELP_LONG]
      ["HELP", 0, 2, SFLG_UNREGOK|SFLG_NOMAXPARAM, 0, 0, 0],
      ["SAY", 1, 0, SFLG_NOMAXPARAM, 0, 1, 2],
      ["COLLECT", 0, 0, 0, ADMIN_FLAG, 0, 0],
      ])
    add_hook([
      [CMODE_HOOK, 'cmode'],
      [UMODE_HOOK, 'umode'],
      [NEWUSR_HOOK, 'newuser'],
      [PRIVMSG_HOOK, 'privmsg'],
      [JOIN_HOOK, 'join'],
      [NICK_HOOK, 'nick'],
      [NOTICE_HOOK, 'notice'],
    ])
    load_language("rubyserv.en")
    join_channel("#test")
  end
  def HELP(client, parv = [])
    log(LOG_DEBUG, "RubyServ::Help")
  end
  def SAY(client, parv = [])
    parv.shift
    message = parv.join(" ")
    reply_user(client, "#{client.name} Said: #{message}")
  end
  def COLLECT(client, parv = [])
    reply_user(client, "Starting GC Call")
    log(LOG_NOTICE, "#{client.name} request Ruby GC Collect")
    GC.start
  end
  def umode(client, what, mode)
    log(LOG_DEBUG, "UMODE client.name: #{client.name} what: #{what} mode: %08x" % [mode])
    log(LOG_DEBUG, "client.nick.nick: #{client.nick.nick}")
  end
  def cmode(source, channel, dir, letter, param = "")
    log(LOG_DEBUG, "Made it to CMODE source.name: #{source.name} channel.name: #{channel.name}")
    log(LOG_DEBUG, "\t#{dir} #{letter} #{param}")
  end
  def newuser(newuser)
    log(LOG_DEBUG, "newuser.name: #{newuser.name}")
  end
  def privmsg(source, channel, message)
    log(LOG_DEBUG, "#{source.name} said #{message} in #{channel.name}")
  end
  def join(source, channel)
    log(LOG_DEBUG, "#{source.name} joined #{channel}")
  end
  def nick(source, oldnick)
    log(LOG_DEBUG, "#{oldnick} is now #{source.name}")
  end
  def notice(source, channel, message)
    log(LOG_DEBUG, "#{source.name} said #{message} in #{channel.name}")
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
