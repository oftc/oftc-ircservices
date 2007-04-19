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
    @msg_count = 5 
    @msg_time = 60

    @msg_global_count = 10
    @msg_global_time = 60

    @msg = {}
    @msg['global'] = {}

    @joined = {}

    channels_each do |channel|
      channel.regchan = regchan_by_name?(channel.name) unless channel.regchan
      name = channel.name.downcase
      if channel.regchan and channel.regchan.floodserv?
        @joined[name] = false unless @joined.include?(name)
        join_channel(name) unless @joined[name]
        log(LOG_CRIT, "FloodServ: WTF in setup and I'm already in channel #{name}") if @joined[name]
        @joined[name] = true
        @msg[name] = {}
      end
    end
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
    chtable = @msg[channel.name]
    gtable = @msg['global']

    chtable[source.host] = FloodQueue.new(@msg_count, @msg_time) unless chtable.include?(source.host)
    gtable[source.host] = FloodQueue.new(@msg_global_count, @msg_global_time) unless gtable.include?(source.host)
    
    uqueue = chtable[source.host]
    gqueue = @msg['global'][source.host]

    uqueue.add(message)
    gqueue.add(message)

    ur = uqueue.enforce
    gr = gqueue.enforce

    log(LOG_NOTICE, "#{source.name}@#{source.host} FLOOD in #{channel.name} MSG: #{message}") if ur
    log(LOG_NOTICE, "#{source.name}@#{source.host} NETWORK FLOOD MSG: #{message}") if gr 
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
      @joined[name] = false
      @msg.delete(name)
      log(LOG_DEBUG, "Something parted FloodServ from #{name} #{reason}")
    end
  end

  def created(channel)
    name = channel.name.downcase
    channel.regchan = regchan_by_name?(name) unless channel.regchan
    if channel.regchan and channel.regchan.floodserv?
      @joined[name] = false unless @joined.include?(name)
      join_channel(name) unless @joined[name]
      log(LOG_DEBUG, "FloodServ: WTF Channel created and I'm already in it?") if @joined[name]
      @joined[name] = true
    end
  end

  def deleted(channel)
    name = channel.name.downcase
    @joined[name] = false if @joined.include?(name)
    @msg.delete(name) if @msg.include?(name)
  end
  
  def nick(source, oldnick)
  end
end

class FloodQueue
  def initialize(max, time)
    @max = max
    @time = time
    @table = Array.new(max)
    @last = 0
  end

  def add(mesg)
    @table = @table.slice(1, @max) if @last == @max-1 and @table[@last]
    @table[@last] = [mesg, Time.now.to_i]
    @last += 1 if @last < @max-1
  end

  def age
    return @table[@last][1] - @table[0][1] if @table[@last]
    return @time * 2
  end

  def enforce
    if @last == @max-1 and age <= @time
      should_enforce = true
      @max.times do |i|
        should_enforce = false unless @table[0][0] == @table[i][0]
      end
      return should_enforce
    end
    return false
  end
end
