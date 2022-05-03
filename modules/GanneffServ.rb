# GanneffServ - the friendly bot who klines all those idiots away that
# we don't want in our network.
class GanneffServ < ServiceModule

  # We need some additional modules
  require "yaml"
  require "time"

  # Set a few basic parameters, register the commands we know, link to the hooks we want.
  def initialize()
    service_name("GanneffServ")
    load_language("ganneffserv.en")

    # do we want debugmode (everything logged as snotes) by default? Can be toggled
    # online, using the DEBUG command
    # @debug=true
    @debug=false

    # Are we running in some kind of limited environment, where lots of stuff wont work?
    # Ie. a second services instances in case someone broke the real one?
    # Then we wont do as much as usual. NEVER CHANGE HERE; USE THE FUNCTIONS PROVIDED ONLINE
    # FIXME: Not implemented yet
    @crap=false

    # How long does an akill last?
    @akill_duration = 14*24*3600
    # @akill_duration = 63

    # Delay for connect/register/join channel checks
    @delay = 15

    # How many kills since startup time / total
    @skills = 0
    @tkills = 0

    # A hash to store the channels in we monitor
    # @channels is built up like:
    # @channels[channel]                - Hash for one channel
    # @channels[channel]["reason"]      - Kill reason
    # @channels[channel]["monitoronly"] - CRFJ/J channel? (Boolean, true = CRFJ)
    # @channels[channel]["kills"]       - How many kills for this channel
    @channels = Hash.new

    # A hash to store all the nicks we saw connecting
    # Gets cleaned from timer function cleanup_event
    # @nicks is built up like:
    # @nicks[id]                    - Hash for the whole data
    # @nicks[id]["client"]          - Contains the clientstruct
    # @nicks[id]["joined"]          - Last channel joined that matched a CRFJ channel (see def LIST)
    # @nicks[id]["jointime"]        - Timestamp of that join
    # @nicks[id]["registered"]      - Timestamp the nick was registered, if we saw this happen
    @nicks = Hash.new

    # Do we have a "bad" server?
    # (this is a server on which we kline (nearly) all connects
    @badserver = ""

    # What commands do we support?
    register([
      #["COMMAND", PAR_MIN, PAR_MAX, FLAGS,                         ACCESS,     HLP_SHORT,              HLP_LONG]
       ["HELP",     0,       2,       SFLG_UNREGOK|SFLG_NOMAXPARAM, ADMIN_FLAG, lm('GS_HLP_SHORT'),     lm('GS_HLP_LONG')],
       ["ADD",      1,       2,       SFLG_NOMAXPARAM,              ADMIN_FLAG, lm('GS_HLP_ADD_SHORT'), lm('GS_HLP_ADD_LONG')],
       ["DEL",      1,       2,       SFLG_NOMAXPARAM,              ADMIN_FLAG, lm('GS_HLP_DEL_SHORT'), lm('GS_HLP_DEL_LONG')],
       ["LIST",     0,       0,       SFLG_NOMAXPARAM,              ADMIN_FLAG, lm('GS_HLP_LST_SHORT'), lm('GS_HLP_LST_LONG')],
       ["CLEANUP",  0,       0,       0,                            ADMIN_FLAG, lm('GS_HLP_CLT_SHORT'), lm('GS_HLP_CLT_LONG')],
       ["DEBUG",    0,       0,       0,                            ADMIN_FLAG, lm('GS_HLP_DBG_SHORT'), lm('GS_HLP_DBG_LONG')],
       ["CRAP",     0,       0,       0,                            ADMIN_FLAG, lm('GS_HLP_CRP_SHORT'), lm('GS_HLP_CRP_LONG')],
       ["SAVE",     0,       0,       0,                            ADMIN_FLAG, lm('GS_HLP_SAV_SHORT'), lm('GS_HLP_SAV_LONG')],
       ["STATS",    0,       0,       SFLG_NOMAXPARAM,              ADMIN_FLAG, lm('GS_HLP_STS_SHORT'), lm('GS_HLP_STS_LONG')],
       ["ENFORCE",  0,       0,       SFLG_NOMAXPARAM,              ADMIN_FLAG, lm('GS_HLP_ENF_SHORT'), lm('GS_HLP_ENF_LONG')],
       ["BADSERV",  0,       1,       SFLG_NOMAXPARAM,              ADMIN_FLAG, lm('GS_HLP_SRV_SHORT'), lm('GS_HLP_SRV_LONG')],
      ]) # register

    # Which hooks do we want?
    add_hook([
       [JOIN_HOOK,         'join_hook'],
       [NICK_REG_HOOK,     'nick_registered'],
       [NEWUSR_HOOK,       'newuser'],
       [EOB_HOOK,          'eob'],
       [CTCP_REPLY_HOOK,   'ctcp_reply'],
       [QUIT_HOOK,         'quit'],
      ]) # add_hook

    # We want to do stuff every now and then, run a timer event
    add_event('cleanup_event', 90, nil)

    # Make sure our channel config is saved every now and then. But dont
    # run as often as cleanup_event
    add_event('save_data', 600, nil)

    debug(LOG_DEBUG, "Startup done, lets wait for trolls to kill.")

    @dbq = Hash.new
    @dbq['GET_ALL_CHANNELS'] = DB.prepare('SELECT channel, reason, kills,
      monitor_only, nick, time FROM ganneffserv, account, nickname WHERE
      ganneffserv.setter = account.id AND account.primary_nick = nickname.id')
    @dbq['INSERT_CHAN'] = DB.prepare('INSERT INTO ganneffserv(setter, time,
      channel, reason, monitor_only) VALUES($1, $2, $3, $4, $5)')
    @dbq['DELETE_CHAN'] = DB.prepare('DELETE FROM ganneffserv WHERE irc_lower(channel) =
      irc_lower($1)')
    @dbq['INCREASE_KILLS'] = DB.prepare('UPDATE ganneffserv SET kills = kills+1
      WHERE irc_lower(channel) = irc_lower($1)')
  end # def initialize

########################################################################
########################################################################
# Stuff we registered help for                                         #
########################################################################
########################################################################

  # Help about GanneffServ
  def HELP(client, parv = [])
    debug(LOG_DEBUG, "#{client.name} called HELP")
    do_help(client, parv[1], parv)

    true
  end # def HELP

# ------------------------------------------------------------------------

  # Save data
  def SAVE(client, parv = [])
    debug(LOG_DEBUG, "#{client.name} called SAVE")
    save_data(nil)
    reply(client, "SAVE done")

    true
  end # def SAVE

# ------------------------------------------------------------------------

  # Cleanup - remove old nicks from @nicks
  def CLEANUP(client, parv = [])
    debug(LOG_DEBUG, "#{client.name} called CLEANUP")
    clean
    debug(LOG_DEBUG, "CLEANUP done")
    reply(client, "CLEANUP done")

    true
  end # def COLLECT

# ------------------------------------------------------------------------

  # Add a channel to GanneffServ monitoring
  def ADD(client, parv = [])
    parv[1].downcase!
    debug(LOG_DEBUG, "#{client.name} called ADD and the params are #{parv.join(",")}")
    enforce = false

    channel = parv[1].downcase
    param   = parv[2].downcase

    if @channels.has_key?(channel)
      reply(client, "Channel #{parv[1]} is already known")
      return true
    end # if @channels.has_key?

    debug(LOG_DEBUG, "Param 2 was #{param}")
    if param == "crfj"
      @channels[channel] = Hash.new
      @channels[channel]["monitoronly"] = true
    elsif param == "j"
      @channels[channel] = Hash.new
      @channels[channel]["monitoronly"] = false
      enforce=true
    else # None of the known values -> don't add channel
      debug(LOG_DEBUG, "Param 2 was invalid.")
      reply(client, "<type> value #{param} is unknown, has to be one of J/CRFJ, see help.")
      return true
    end # if parv[2]

    ret = DB.execute_nonquery(@dbq['INSERT_CHAN'], 'iissb', client.nick.account_id,
      Time.now.to_i, channel, parv[-1], @channels[channel]['monitoronly'])
    
    @channels[channel]["reason"] = parv[-1]
    @channels[channel]["kills"] = 0

    if ret then
      debug(LOG_NOTICE, "#{client.name} added #{parv[1]}, type #{param}, reason #{parv[-1]}")

      #save_data

      reply(client, "Channel #{channel} successfully added")

      # In case its not a "monitoronly" channel lets enforce it and kill
      # everyone who is in it.
      if enforce
        chan = Channel.find(channel)
        do_enforce(chan, parv[-1]) if chan
      end # if enforce
    else
      @channels.delete(channel)
      reply(client, "Failed to add #{channel}")
    end

    true
  end # def ADD

# ------------------------------------------------------------------------

  # Delete a channel from the monitoring
  def DEL(client, parv = [])
    parv[1].downcase!
    debug(LOG_DEBUG, "#{client.name} called DEL and the params are #{parv.join(",")}")
    channel = parv[1].downcase
    return unless @channels.has_key?(channel)

    ret = DB.execute_nonquery(@dbq['DELETE_CHAN'], 's', channel)

    if ret then
      debug(LOG_NOTICE, "#{client.name} deleted channel #{channel}. Its old reason was #{@channels[channel]["reason"]} and monitoring only was #{@channels[channel]["monitoronly"]}")
      @channels.delete(channel)

      #save_data

      reply(client, "Channel #{channel} successfully deleted.")
    else
      reply(client, "Failed to delete channel #{channel}.")
    end

    true
  end # def DEL

# ------------------------------------------------------------------------

  # List all channels we monitor
  def LIST(client, parv = [])
    debug(LOG_DEBUG, "#{client.name} called LIST")
    reply(client, "Known Channels\n\n")
    reply(client, "%-20s %-4s %-10s %-19s %s" % [ "Channel", "Type", "By", "When", "Action" ])

    result = DB.execute(@dbq['GET_ALL_CHANNELS'], '')

    debug(LOG_DEBUG, "LIST #{result.row_count}")

    result.row_each { |row|
      check = "J"
      check = "CRJF" if row[3].to_i == 1
      
      chan = row[0]
      by = row[4]
      time = Time.at(row[5].to_i).strftime('%Y-%m-%d %H:%M:%S')
      reason = row[1]
      reply(client, "%-20s %-4s %-10s %-19s %s" % [ chan, check, by, time, "AKILL: #{reason}" ])
    }
    result.free

    reply(client, "\nCRFJ - checks Connect, Register nick, Join channel within 15 seconds (i.e. Fast)")
    reply(client, "J - triggers on every Join")

    true
  end # def LIST

# ------------------------------------------------------------------------

  # Enforce all channels again, just in case.
  def ENFORCE(client, parv = [])
    debug(LOG_NOTICE, "#{client.name} called ENFORCE, simulating an EOB")
    reply(client, "ENFORCE triggered")
    eob

    true
  end # def ENFORCE

# ------------------------------------------------------------------------

  # toggle debug notices
  def DEBUG(client, parv = [])
    debug(LOG_DEBUG, "#{client.name} asked to toggle the debug mode")
    if @debug
      @debug=false
    else # if @debug
      @debug=true
    end # if @debug

    reply(client, "Toggled DEBUG mode to #{@debug}")

    true
  end # def DEBUG

# ------------------------------------------------------------------------

  # toggle crap mode with missing services
  def CRAP(client, parv = [])
    debug(LOG_DEBUG, "#{client.name} asked to toggle the crap mode")
    if @crap
      @crap=false
    else # if @crap
      @crap=true
    end # if @crap

    debug(LOG_NOTICE, "#{client.name} asked to toggle the crap mode, it is now #{@crap}")
    reply(client, "Toggled CRAP mode to #{@crap}")

    true
  end # def CRAP

# ------------------------------------------------------------------------

  # display some statistics
  def STATS(client, parv = [])
    debug(LOG_DEBUG, "#{client.name} asked me for some stats pron")

    reply(client, "STATS PRON")
    reply(client, "I know about the following channels:\n\n")
    reply(client, "%-20s  %-5s %s" % [ "Channel", "Kills", "AKILL Reason" ])

    @channels.sort.each do |name, data|
      reply(client, "%-20s  %5d %s" % [ name, data["kills"], data["reason"] ] )
    end # channels.each_pair

    reply(client, "\nI know about #{@nicks.length} clients.")
    reply(client, "\nI killed #{@skills} users since my last startup and #{@tkills} users in my whole lifetime.")
    reply(client, "\nDEBUG mode: #{@debug}")
    reply(client, "CRAP  mode: #{@crap}")

    true
  end # def STATS

# ------------------------------------------------------------------------

  # Mark a server as "bad"
  def BADSERV(client, parv = [])
    if parv.length > 0
      parv[1].downcase!
      debug(LOG_DEBUG, "#{client.name} called BADSERV and the params are #{parv.join(",")}")
      server = parv[1].downcase

      if server =~ /.*\.oftc.net$/
        debug(LOG_DEBUG, "#{server} seems to be an oftc server, proceeding")
        @badserver = server
        reply(client, "#{server} is now marked as a bad server, all new connections will be killed")
        debug(LOG_NOTICE, "#{client.to_str} has enabled BADSERV for #{server} all new connections will be killed")
      elsif server =~ /OFF/i
        if @badserver != ""
          reply(client, "#{@badserver} is no longer listed as a bad server.")
          debug(LOG_NOTICE, "#{client.to_str} has disabled BADSERV for #{@badserver}")
          @badserver = ""
        else
          reply(client, "There was no defined bad server, it was already off")
        end
      else
        false
      end
    else
      if @badserver != ""
        reply(client, "Killing all new connections from #{@badserver}")
      else
        reply(client, "No server is currently listed as a bad server")
      end
      false
    end # if server

    true
  end # def BADSERV

########################################################################
########################################################################
# Hook functions                                                       #
########################################################################
########################################################################

  #this method is sure to be called after EOB (if a conf loaded module) and
  #after a reload. this means we know we're connected to the network and the
  #database is alive
  def loaded()
    # Now load the data.
    load_data
    eob
  end

  # Called via event handlers, every X seconds.
  def cleanup_event(arg)
    debug(LOG_DEBUG, "Timer event CLEANUP starting")
    clean
    debug(LOG_DEBUG, "Timer event CLEANUP done")

    true
  end # cleanup_event

# ------------------------------------------------------------------------

  # Someone joined some channel
  def join_hook(client, channel)
    channel.downcase!
    debug(LOG_DEBUG, "#{client.name} joined #{channel}")
    nick=client.name.downcase
    channel.downcase!
    ret=true
    if not client.is_services_client?
      if @channels.has_key?(channel)
        debug(LOG_DEBUG, "#{nick} is not some services instance joining #{channel}")
        if @channels[channel]["monitoronly"] == true
          if @nicks.has_key?(client.id)
            @nicks[client.id]["joined"] = channel
            @nicks[client.id]["jointime"] = Time.new.to_i
            ret = timecheck(client)
          end
        else # if @channels...["monitoronly"]
          debug(LOG_NOTICE, "#{nick} joined channel #{channel}, killing")
          drop_nick(nick) if @nicks.has_key?(client.id) and @nicks[client.id]["registered"]
          ret = akill(client, "#{@channels[channel]["reason"]}", "J:#{channel}", channel)
        end # if @channels...["monitoronly"]
        debug(LOG_DEBUG, "join_hook says that ret is #{ret}")
      end # if @channels.has_key?
    end # not client.is_services_client
    debug(LOG_DEBUG, "Done with the join_hook for #{nick}, returning #{ret}")
    ret
  end # def join_hook

# ------------------------------------------------------------------------

  # Check if someone did connect/register/joinchannel too fast
  # Triggered by join in a channel who is set to monitoronly (def join_hook) or
  # by a fast (within 60seconds) nick registration
  def timecheck(client)
    nick = client.name.downcase
    debug(LOG_DEBUG, "Checking connect/register/join time for #{client.name}")
    ret = true

    # Do nothing if this client hasnt joined any of our join-monitored channels yet.
    # Needed as this function is called by both, nick_registered and join_hook, and we
    # only want to trigger here when both are done in a too small timeframe
    if @nicks[client.id]["joined"].nil? or @nicks[client.id]["registered"].nil?
      debug(LOG_DEBUG, "#{client.name} doesn't pass the timecheck")
      return true
    end # if @nicks... or @nicks...

    # Check if join and register both happened within @delay seconds from connect
    rdiff = @nicks[client.id]["registered"] - client.firsttime
    jdiff = @nicks[client.id]["jointime"] - client.firsttime
    debug(LOG_DEBUG, "#{client.name} rdiff is #{rdiff}, jdiff is #{jdiff}")

    if rdiff < @delay and jdiff < @delay
      debug(LOG_NOTICE, "#{nick} hit #{@delay} seconds delay for register/join, channel #{@nicks[client.id]["joined"]}, killing and dropping nick")
      ret = akill(client, "#{@channels[@nicks[client.id]["joined"]]["reason"]}", "CRFJ:#{@nicks[client.id]["joined"]}", @nicks[client.id]["joined"])
      # Now drop the nick
      ret = drop_nick(nick)
      debug(LOG_DEBUG, "drop_nick returned #{ret}")
    end # if rdiff/jdiff
    ret
  end # def timecheck

# ------------------------------------------------------------------------

  # Record time of nick registration
  def nick_registered(client)
    debug(LOG_DEBUG, "#{client.name} registered its nick")
    ret = true
    now = Time.new.to_i
    diff = now - client.firsttime
    nick = client.name.downcase

    if diff < 60
      # If the nick got registered in the first 60 seconds after connect we save this and warn about it
      debug(LOG_NOTICE, "#{client.name} registered the nick after being online for only #{diff} seconds")

      @nicks[client.id] = Hash.new unless @nicks[client.id]  # in case we dont know them already (like services restarted)
      @nicks[client.id]["registered"] = now

      # Now ctcp them so we record versions of such nicks.
      ctcp_user(client, "VERSION") unless client.is_services_client?
      ret = timecheck(client)
    end
    ret
  end # def nick_registered

# ------------------------------------------------------------------------

  # We got a ctcp reply
  def ctcp_reply(service, client, command, arg)
    debug(LOG_DEBUG, "Got a ctcp reply for #{client.name} and command #{command}")
    return unless command == 'VERSION'
    msg = ""
    nick = client.name.downcase
    if @nicks.has_key?(client.id)
      diff = @nicks[client.id]["registered"] - @nicks[client.id]["client"].firsttime

      if diff < 60
        msg = " (online for #{diff} seconds) "
      end # if diff

    end # if @nicks.has_key
    debug(LOG_NOTICE, "#{nick} #{msg} CTCP'd #{command}: #{arg}")
  end

# ------------------------------------------------------------------------

  # A new user connected
  def newuser(client)
    nick = client.name.downcase
    debug(LOG_DEBUG, "#{nick} connected at timestamp #{client.firsttime}")

    # add more ips if wanted
    whitelist = [ '89.16.166.62' ];

    if client.ip_or_hostname =~ /\.oftc\.net$/ or whitelist.include?(client.ip_or_hostname)
      return true
    end

    if @badserver.length > 0
      ## Check when they were connected, this prevents against a badserv that splits
      ## comes back and all the old connects get considered as spammers
      ## XXX XXX XXX
      ## If the server is split longer than @delay and new connects happen, upon
      ## reintroduction those new connects won't be considered new enough to kill.
      if @badserver == client.from.name and Time.new.to_i - client.firsttime < @delay
        return akill(client, "Spammer", "Badserv:#{@badserver}", "")
      else
        debug(LOG_DEBUG, "#{nick} not on badserv #{@badserver} but on #{client.from.name}, not killing")
      end
    end # if @badserv.length

    @nicks[client.id] = Hash.new
    @nicks[client.id]["client"] = client
    true
  end # def newuser

# ------------------------------------------------------------------------

  # Server connect burst is done
  def eob()
    debug(LOG_NOTICE, "EOB is done, enforcing channels")
    @channels.each_pair do |name, data|
      debug(LOG_DEBUG, "I see #{name} with #{data}")
      chan = Channel.find(name)
      if chan
        debug(LOG_DEBUG, "I found that channel #{name} exists, enforcing")
        do_enforce(chan, data["reason"])
      end # if chan
    end # @channels.each_pair
    debug(LOG_DEBUG, "All eob action done")
    true
  end # def eob

# ------------------------------------------------------------------------

  # A client quits, lets delete his entry in @nicks
  def quit(client, reason)
    debug(LOG_DEBUG, "#{client.name} quits")
    @nicks.delete(client.id)
    true
  end # def quit

########################################################################
########################################################################
# Other stuff, including private functions                             #
########################################################################
########################################################################

  private

  # a little helper to set akills. Just to have the checks for hostmasks and stuff
  # at one place only
  def akill(client, reason, operreason, channel="")
    ret = false
    debug(LOG_DEBUG, "Should akill #{client.name} with reason #{reason}, operreason #{operreason}, channel #{channel}")
    if client.realhost.nil?
      host = client.host
    else 
      host = client.realhost
    end # if client..nil?

    if not reason.include?("support@oftc.net")
      reason = "#{reason} - Contact support@oftc.net for help."
    end # if not reason.include?

    #client.host is always filled, check it for the cloak value
    if client.host =~ /.*\.(noc|netop|netrep|chair|ombudsman|advisor)\.oftc\.net$/
      debug(LOG_DEBUG, "Not issuing AKILL for #{client.name} having cloak #{client.host}, real host #{client.realhost}")
      ret = false # continue with callbacks, we haven't set any kill
    elsif client.is_tor?
      debug(LOG_DEBUG, "Using /kill instead of AKILL for Tor user #{client.name}")
      ret = kill_user(client, reason)
    else # if host
      reason = "#{reason}|#{operreason}"
      debug(LOG_DEBUG, "Issuing AKILL: *@#{host}, #{reason} lasting for #{@akill_duration} seconds")
      ret = akill_add("*@#{host}", reason, @akill_duration)
    end # if host

    channel.downcase!

    if channel.length > 0 and not @channels[channel].nil?
      @channels[channel]["kills"]+=1
      DB.execute_nonquery(@dbq['INCREASE_KILLS'], 's', channel)
    end # if channel.length

    @tkills+=1
    @skills+=1

    # The following is inverse to logic, but its the way to stop callbacks
    if ret
      return false  # AKILL got set, return false to signal "stop callbacks"
    else 
      return true # Continue with callbacks, something went wrong
    end # if kill_user
  end # def akill

# ------------------------------------------------------------------------

  # enforce a channel - kill all of its users
  def do_enforce(channel, reason)
    if channel.name.nil?
      cname=channel
    else
      cname=channel.name
    end # if channel.name.nil?
    cname.downcase!

    cname.downcase!

    if @channels.has_key?(cname)
      if @channels[cname]["monitoronly"] == true
        return true # Nothing to do here
      else # if @channels...monitoronly
        debug(LOG_NOTICE, "Asked to enforce #{cname} with reason \"#{reason}\"")
        chan = Channel.find(cname)
        chan.members_each do |client|
          akill(client, reason, "J: #{cname}", cname)
        end # chan.members_each
      end # if @channels...monitoronly
    end # if @channels.has_key
  end # def enforce

# ------------------------------------------------------------------------

  # Load data from yaml
  def load_data()
    debug(LOG_DEBUG, "Loading channel data")
    result = DB.execute(@dbq['GET_ALL_CHANNELS'], '')
    @channels = Hash.new

    count = 0
    result.row_each { |row|
      debug(LOG_DEBUG, "Checking row #{count}")
      channel = row[0].downcase
      @channels[channel] = Hash.new
      @channels[channel]['reason'] = row[1]
      @channels[channel]['kills'] = row[2].to_i
      mo = row[3].to_i
      if mo == 1 then
        mo = true
      else
        mo = false
      end
      @channels[channel]['monitoronly'] = mo
      @tkills += row[2].to_i
      count += 1
    }
    result.free
    debug(LOG_DEBUG, "All channel data successfully loaded")
  end # def load_data

# ------------------------------------------------------------------------

  # Save data to yaml
  def save_data(arg)
    debug(LOG_DEBUG, "Saving channel data")
    #File.open("#{@langpath}/ganneffserv-channels.yaml", 'w') do |out|
    #  YAML.dump(@channels, out)
    #end # gs-channels
    #debug(LOG_DEBUG, "All channel data successfully saved")
  end # def save_data

# ------------------------------------------------------------------------

  # little debug log function. In case we turn @debug on we "convert" all
  # logging levels to be LOG_NOTICE AKA snotes.
  def debug(level, message)
    if @debug
      level=LOG_NOTICE
    end # @debug
    log(level, message)
  end # def debug

# ------------------------------------------------------------------------

  # Cleanup - remove old nicks from @nicks
  def clean()
    @nicks.each_pair do |id, data|
      client = data["client"]
      diff = Time.new.to_i - client.firsttime
      nick = client.name
      debug(LOG_DEBUG, "Looking at #{nick} which I know for #{diff} seconds.")
      if diff > 120
        debug(LOG_DEBUG, "Deleting knowledge of #{nick}, known for #{diff} seconds now.")
        @nicks.delete(id)
      end # if diff
    end # @nicks.each_pair
    debug(LOG_DEBUG, "Done with all nicks")
    true
  end # def clean

end # class GanneffServ
