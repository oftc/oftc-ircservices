class FloodServ < ServiceModule
  def initialize()
    service_name("FloodServ")
    load_language("floodserv.en")
    register([["HELP", 0, 0, SFLG_UNREGOK|SFLG_NOMAXPARAM, 0, lm('FS_HELP_NOHELP'), lm('FS_HELP_NOHELP')]])
    add_hook([
      [NEWUSR_HOOK, 'newuser'],
      [PRIVMSG_HOOK, 'msg'],
      [NOTICE_HOOK, 'msg'],
      [JOIN_HOOK, 'join'],
      [PART_HOOK, 'part'],
      [NICK_HOOK, 'nick'],
      [CHAN_CREATED_HOOK, 'created'],
      [CHAN_DELETED_HOOK, 'deleted'],
    ])

    setup
  end

  def setup
    #@akill_duration = 7 * 24 * 3600
    @akill_duration = 30
    @msg_count = 5
    @msg_time = 60
    @line_time = 3

    @msg_gcount = 10
    @msg_gtime = 60

    @msg = {}
    @msg['global'] = {}

    @joined = {}

    channels_each do |channel|
      channel.regchan = regchan_by_name?(channel.name) unless channel.regchan
      add_channel(channel.name) if channel.regchan and channel.regchan.floodserv?
    end
  end

  def add_channel(channel)
    name = channel.downcase
    @joined[name] = false unless @joined.include?(name)
    join_channel(name) unless @joined[name]
    log(LOG_CRIT, "FloodServ: Already joined to channel: #{name}") if @joined[name]
    @msg[name] = {}
  end

  def unload
    @msg.clear
  end
  
  def HELP(client, parv = [])
		do_help(client, parv[1], parv)
  end
  
  def newuser(newuser)
  end
  
  def msg(source, channel, message)
    chtable = @msg[channel.name.downcase]
    gtable = @msg['global']

    chtable[source.host] = MLQueue.new(@msg_count, @msg_time, @line_time) unless chtable.include?(source.host)
    gtable[source.host] = FloodQueue.new(@msg_gcount, @msg_gtime) unless gtable.include?(source.host)
    
    uqueue = chtable[source.host]
    gqueue = @msg['global'][source.host]

    uqueue.add(message)
    gqueue.add(message)

    msg_enforce = uqueue.enforce
    lne_enforce = uqueue.enforce_lines

    gmsg_enforce = gqueue.enforce

    log(LOG_NOTICE, "#{source.name}@#{source.host} MESSAGE FLOOD in #{channel.name} MSG: #{message}") if msg_enforce
    log(LOG_NOTICE, "#{source.name}@#{source.host} LINE FLOOD in #{channel.name} #{@msg_count}/#{@line_time}") if lne_enforce
    
    if gmsg_enforce
      log(LOG_NOTICE, "#{source.name}@#{source.host} NETWORK FLOOD MSG: #{message}")
      ret = akill_add("*@#{source.host}", "Triggered Network Flood Protection, please email support@oftc.net", @akill_duration)
    end
  end
  
  def join(source, channel)
    if is_me?(source)
      name = channel.downcase
      @joined[name] = true
      @msg[name] = {} unless @msg.include?(name)
      log(LOG_DEBUG, "Something joined FloodServ to #{name}")
    end
  end

  def part(source, client, channel, reason)
    if is_me?(client)
      name = channel.name.downcase
      @joined.delete(name)
      @msg.delete(name)
      log(LOG_DEBUG, "Something parted FloodServ from #{name} #{reason}")
    end
  end

  def created(channel)
    name = channel.name.downcase
    channel.regchan = regchan_by_name?(name) unless channel.regchan
    add_channel(channel.name) if channel.regchan and channel.regchan.floodserv?
  end

  def deleted(channel)
    name = channel.name.downcase
    @joined.delete(name)
    @msg.delete(name)
  end
  
  def nick(source, oldnick)
  end
end

class FloodQueue
  def initialize(max, time)
    @max = max
    @time = time
    @table = Array.new(max)
    @last = @max-1
  end

  def add(mesg)
    drop_oldest if @last == 0 and @table[@last]
    @table[@last] = [mesg.downcase, Time.now.to_i]
    @last -= 1 if @last > 0
  end

  def age
    return @table[0][1] - @table[@max-1][1] if @table[@last] and @table[@max-1]
    return @time * 2
  end

  def enforce
    if @last == 0 and age <= @time
      should_enforce = true
      @max.times do |i|
        should_enforce = false unless @table[0][0] == @table[i][0]
      end
      return should_enforce
    end
    return false
  end

  def drop_oldest
    start = @table.length - 1
    while start > 0
      @table[start] = @table[start-1]
      start -= 1
    end
  end
end

class MLQueue < FloodQueue
  def initialize(max, mtime, ltime)
    @max = max
    @time = mtime
    @ltime = ltime
    @table = Array.new(@max)
    @last = @max-1
  end

  def enforce_lines
    return true if @last == 0 and age <= @ltime
    return false
  end
end
