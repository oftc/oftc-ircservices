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
      ["QUERY", 1, 0, SFLG_NOMAXPARAM, ADMIN_FLAG, lm('RS_HELP_PART_SHORT'), lm('RS_HELP_PART_LONG')],
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
      [CTCP_HOOK, 'ctcp_msg'],
      [EOB_HOOK, 'eob'],
    ])
    #add_event('timer', 3)

    @queryid = DB.prepare("SELECT nick,account_id,reg_time FROM nickname WHERE lower(nick) = lower($1)")
  end

  def loaded()
    chan = @client.join("#test")
    send_cmode(chan, "+s", "")
    Channel.all_each { |x| log(LOG_DEBUG, "Channel #{x.name} found") }
  end

  def timer()
    log(LOG_NOTICE, "Timer Called")
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
    @client.join(parv[1])
  end
  def PART(client, parv = [])
    reason = nil
    reason = parv[2, parv.length-2].join(' ') if parv.length > 2
    log(LOG_DEBUG, "RUBY Parting channel #{parv[1]}: #{reason}")
    @client.part(parv[1], reason)
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
    ctcp_user(newuser, "VERSION") unless newuser.is_server?
  end
  def privmsg(source, channel, message)
    log(LOG_DEBUG, "RUBY #{source.name} said #{message} in #{channel.name}")
  end
  def join(source, channel)
    log(LOG_DEBUG, "RUBY #{source.name} joined #{channel}")
    rchannel = Channel.find(channel)
    rchannel.members_each { |x| log(LOG_DEBUG, "#{x.name} is also in #{rchannel.name}") }
  end
  def part(client, source, channel, reason)
    @client.part('#floodtest', nil) if channel.name.downcase == '#floodtest' and channel.members_length == 1
  end
  def nick(source, oldnick)
    log(LOG_DEBUG, "RUBY #{oldnick} is now #{source.name}")
  end
  def notice(source, channel, message)
    log(LOG_DEBUG, "RUBY #{source.name} said #{message} in #{channel.name}")
  end
  def chan_created(channel)
    log(LOG_DEBUG, "RUBY #{channel.name} created")
    @client.join("#floodtest") if channel.name.downcase == "#floodtest"
  end
  def chan_deleted(channel)
    log(LOG_NOTICE, "#{channel.name} has been deleted")
  end
  def ctcp_msg(service, client, command, arg)
    log(LOG_NOTICE, "#{client.name} CTCP'd #{command}: #{arg}")
  end
  def eob()
    log(LOG_NOTICE, "EOB IS DONE")
  end

  def QUERY(client, parv = [])
    result = DB.execute(@queryid, "s", parv[1])
    reply_user(client, "#{result.row_count} results")
    result.row_each { |row|
      reply_user(client, "Nick: #{row[0]} Account: #{row[1]} RegTime: #{row[2]}")
    }
    result.free
  end
end
