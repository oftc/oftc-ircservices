class RubyServ < ServiceModule
  def initialize()
    service_name("RubyServ")
    load_language("rubyserv.en")
    register([
      #["COMMAND", PARAM_MIN, PARAM_MAX, FLAGS, ACCESS, HLP_SHORT, HELP_LONG]
      ["HELP", 0, 2, SFLG_UNREGOK|SFLG_NOMAXPARAM, 0, lm('RS_HELP_SHORT'), lm('RS_HELP_LONG')],
      ["SAY", 1, 0, SFLG_NOMAXPARAM, 0, lm('RS_HELP_SAY_SHORT'), lm('RS_HELP_SAY_LONG')],
      ["COLLECT", 0, 0, 0, ADMIN_FLAG, lm('RS_HELP_COLLECT_SHORT'), lm('RS_HELP_COLLECT_LONG')],
      ["JOIN", 1, 0, SFLG_NOMAXPARAM, ADMIN_FLAG, lm('RS_HELP_JOIN_SHORT'), lm('RS_HELP_JOIN_LONG')],
      ["PART", 1, 0, SFLG_NOMAXPARAM, ADMIN_FLAG, lm('RS_HELP_PART_SHORT'), lm('RS_HELP_PART_LONG')],
      ])
    add_hook([
      [CMODE_HOOK, 'cmode'],
      [UMODE_HOOK, 'umode'],
      [NEWUSR_HOOK, 'newuser'],
      [PRIVMSG_HOOK, 'privmsg'],
      [JOIN_HOOK, 'join'],
      [PART_HOOK, 'part'],
      [NICK_HOOK, 'nick'],
      [NOTICE_HOOK, 'notice'],
      [CHAN_CREATED_HOOK, 'chan_created'],
      [CHAN_DELETED_HOOK, 'chan_deleted'],
    ])
    #join_channel("#test")
  end
  def HELP(client, parv = [])
    log(LOG_DEBUG, "RubyServ::Help")
		do_help(client, parv[1], parv)
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
  def JOIN(client, parv = [])
    log(LOG_DEBUG, "RUBY Joining channel #{parv[1]}")
    join_channel(parv[1])
  end
  def PART(client, parv = [])
    reason = nil
    reason = parv[2, parv.length-2].join(' ') if parv.length > 2
    log(LOG_DEBUG, "RUBY Parting channel #{parv[1]}: #{reason}")
    part_channel(parv[1], reason)
  end
  def umode(client, what, mode)
    log(LOG_DEBUG, "RUBY UMODE client.name: #{client.name} what: #{what} mode: %08x" % [mode])
    log(LOG_DEBUG, "client.nick.nick: #{client.nick.nick}")
  end
  def cmode(source, channel, dir, letter, param = "")
    log(LOG_DEBUG, "RUBY Made it to CMODE source.name: #{source.name} channel.name: #{channel.name}")
    log(LOG_DEBUG, "\t#{dir} #{letter} #{param}")
  end
  def newuser(newuser)
    log(LOG_DEBUG, "RUBY newuser.name: #{newuser.name}")
  end
  def privmsg(source, channel, message)
    log(LOG_DEBUG, "RUBY #{source.name} said #{message} in #{channel.name}")
  end
  def join(source, channel)
    log(LOG_DEBUG, "RUBY #{source.name} joined #{channel}")
  end
  def part(client, source, channel, reason)
    part_channel('#floodtest', nil) if channel.name.downcase == '#floodtest' and channel.members_length == 1
  end
  def nick(source, oldnick)
    log(LOG_DEBUG, "RUBY #{oldnick} is now #{source.name}")
  end
  def notice(source, channel, message)
    log(LOG_DEBUG, "RUBY #{source.name} said #{message} in #{channel.name}")
  end
  def chan_created(channel)
    log(LOG_DEBUG, "RUBY #{channel.name} created")
    join_channel("#floodtest") if channel.name.downcase == "#floodtest"
  end
  def chan_deleted(channel)
    log(LOG_NOTICE, "#{channel.name} has been deleted")
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
