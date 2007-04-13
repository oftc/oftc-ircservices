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
      [NICK_HOOK, 'nick'],
    ])

    setup
  end

  def setup
    @msg_count = 5 
    @msg_time = 60

    @msg_global_count = 10
    @msg_global_time = 60

    @msg = {}
    @msg['#floodtest'] = {}
    @msg['#test'] = {}
    @msg['global'] = {}

    join_channel("#floodtest")
    join_channel("#test")
  end

  def unload
    #part_channel("#text")
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
