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

    # Now load the data.
    load_data

    # A hash to store all the nicks we saw connecting
    # Gets cleaned from timer function cleanup_event
    # @nicks is built up like:
    # @nicks[nick]                    - Hash for the whole data
    # @nicks[nick]["client"]          - Contains the clientstruct
    # @nicks[nick]["joined"]          - Last channel joined that matched a CRFJ channel (see def LIST)
    # @nicks[name]["jointime"]        - Timestamp of that join
    # @nicks[nick]["registered"]      - Timestamp the nick was registered, if we saw this happen
    @nicks = Hash.new

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
      ]) # register

    # Which hooks do we want?
    add_hook([
       [JOIN_HOOK,         'join_hook'],
       [NICK_REG_HOOK,     'nick_registered'],
       [NEWUSR_HOOK,       'newuser'],
       [EOB_HOOK,          'eob'],
       [CTCP_HOOK,         'ctcp_reply'],
       [QUIT_HOOK,         'quit'],
       [NICK_HOOK,         'nick_changed'],
      ]) # add_hook

    # We want to do stuff every now and then, run a timer event
    add_event('cleanup_event', 90)

    # Make sure our channel config is saved every now and then. But dont
    # run as often as cleanup_event
    add_event('save_data', 600)

    debug(LOG_DEBUG, "Startup done, lets wait for trolls to kill.")
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
    save_data
    reply_user(client, "SAVE done")

    true
  end # def SAVE

# ------------------------------------------------------------------------

  # Cleanup - remove old nicks from @nicks
  def CLEANUP(client, parv = [])
    debug(LOG_DEBUG, "#{client.name} called CLEANUP")
    clean
    debug(LOG_DEBUG, "CLEANUP done")
    reply_user(client, "CLEANUP done")

    true
  end # def COLLECT

# ------------------------------------------------------------------------

  # Add a channel to GanneffServ monitoring
  def ADD(client, parv = [])
    debug(LOG_DEBUG, "#{client.name} called ADD and the parms are #{parv.join(",")}")
    enforce = false

    if @channels.has_key?(parv[1])
      reply_user(client, "Channel #{parv[1]} is already known")
      return true
    end # if @channels.has_key?

    debug(LOG_DEBUG, "Param 2 was #{parv[2]}")
    if parv[2] == "CRFJ"
      @channels[parv[1]] = Hash.new
      @channels[parv[1]]["monitoronly"] = true
    elsif parv[2] == "J"
      @channels[parv[1]] = Hash.new
      @channels[parv[1]]["monitoronly"] = false
      enforce=true
    else # None of the known values -> don't add channel
      debug(LOG_DEBUG, "Param 2 was invalid.")
      reply_user(client, "<type> value #{parv[2]} is unknown, has to be one of J/CRFJ, see help.")
      return true
    end # if parv[2]

    @channels[parv[1]]["reason"] = parv[-1]
    @channels[parv[1]]["kills"] = 0

    debug(LOG_NOTICE, "#{client.name} added #{parv[1]}, type #{parv[2]}, reason #{parv[-1]}")

    save_data

    reply_user(client, "Channel #{parv[1]} successfully added")

    # In case its not a "monitoronly" channel lets enforce it and kill
    # everyone who is in it.
    if enforce
      channel = Channel.find(parv[1])
      do_enforce(channel, parv[-1]) if channel
    end # if enforce

    true
  end # def ADD

# ------------------------------------------------------------------------

  # Delete a channel from the monitoring
  def DEL(client, parv = [])
    debug(LOG_DEBUG, "#{client.name} called DEL and the parms are #{parv.join(",")}")
    return unless @channels.has_key?(parv[1])
    debug(LOG_NOTICE, "#{client.name} deleted channel #{parv[1]}. It's old reason was #{@channels[parv[1]]["reason"]} and monitoring only was #{@channels[parv[1]]["monitoronly"]}")
    @channels.delete(parv[1])

    save_data

    reply_user(client, "Channel #{parv[1]} successfully deleted.")

    true
  end # def DEL

# ------------------------------------------------------------------------

  # List all channels we monitor
  def LIST(client, parv = [])
    debug(LOG_DEBUG, "#{client.name} called LIST")
    reply_user(client, "Known Channels\n\n")
    reply_user(client, "%-20s %-6s %s" % [ "Channel", "Type", "Action" ])

    @channels.sort.each do |name, data|
      check = "J"
      if data["monitoronly"]
        check = "CRFJ"
      end # if data

      reply_user(client, "%-20s %-6s %s" % [ name, check, "AKILL: #{data["reason"]}" ])
    end # @channels.each_pair

    reply_user(client, "\n\nCRFJ - checks Connect, Register nick, Join channel within 15 seconds (i.e. Fast)")
    reply_user(client, "J - triggers on every Join")

    true
  end # def LIST

# ------------------------------------------------------------------------

  # Enforce all channels again, just in case.
  def ENFORCE(client, parv = [])
    debug(LOG_NOTICE, "#{client.name} called ENFORCE, simulating an EOB")
    reply_user(client, "ENFORCE triggered")
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

    reply_user(client, "Toggled DEBUG mode to #{@debug}")

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
    reply_user(client, "Toggled CRAP mode to #{@crap}")

    true
  end # def CRAP

# ------------------------------------------------------------------------

  # display some statistics
  def STATS(client, parv = [])
    debug(LOG_DEBUG, "#{client.name} asked me for some stats pron")

    reply_user(client, "STATS PRON")
    reply_user(client, "I know about the following channels:\n\n")
    reply_user(client, "%-20s  %-5s %s" % [ "Channel", "Kills", "AKILL Reason" ])

    @channels.sort.each do |name, data|
      reply_user(client, "%-20s  %5d %s" % [ name, data["kills"], data["reason"] ] )
    end # channels.each_pair

    reply_user(client, "\nI know about #{@nicks.length} clients.")
    reply_user(client, "\nI killed #{@skills} users since my last startup and #{@tkills} users in my whole lifetime.")
    reply_user(client, "\nDEBUG mode: #{@debug}")
    reply_user(client, "CRAP  mode: #{@crap}")

    true
  end # def STATS

########################################################################
########################################################################
# Hook functions                                                       #
########################################################################
########################################################################

  # Called via event handlers, every X seconds.
  def cleanup_event()
    debug(LOG_DEBUG, "Timer event CLEANUP starting")
    clean
    debug(LOG_DEBUG, "Timer event CLEANUP done")

    true
  end # cleanup_event

# ------------------------------------------------------------------------

  # Someone joined some channel
  def join_hook(client, channel)
    debug(LOG_DEBUG, "#{client.name} joined #{channel}")
    nick=client.name
    ret=true
    if not client.is_services_client?
      if @channels.has_key?(channel)
        debug(LOG_DEBUG, "#{nick} is not some services instance joining #{channel}")
        if @channels[channel]["monitoronly"]
          @nicks[nick]["joined"] = channel
          @nicks[nick]["jointime"] = Time.new.to_i
          ret = timecheck(client)
        else # if @channels...["monitoronly"]
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
    nick = client.name
    debug(LOG_DEBUG, "Checking connect/register/join time for #{client.name}")
    ret = true

    # Do nothing if this client hasnt joined any of our join-monitored channels yet.
    # Needed as this function is called by both, nick_registered and join_hook, and we
    # only want to trigger here when both are done in a too small timeframe
    if @nicks[nick]["joined"].nil? or @nicks[nick]["registered"].nil?
      debug(LOG_DEBUG, "#{client.name} doesn't pass the timecheck")
      return true
    end # if @nicks... or @nicks...

    # Check if join and register both happened within @delay seconds from connect
    rdiff = @nicks[nick]["registered"] - client.firsttime
    jdiff = @nicks[nick]["jointime"] - client.firsttime
    debug(LOG_DEBUG, "#{client.name} rdiff is #{rdiff}, jdiff is #{jdiff}")

    if rdiff < @delay and jdiff < @delay
      debug(LOG_NOTICE, "#{nick} hit #{@delay} seconds delay for register/join, channel #{@nicks[nick]["joined"]}, killing and dropping nick")
      ret = akill(client, "#{@channels[@nicks[nick]["joined"]]["reason"]}", "CRFJ:#{@nicks[nick]["joined"]}", @nicks[nick]["joined"])
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

    if diff < 60
      # If the nick got registered in the first 60 seconds after connect we save this and warn about it
      debug(LOG_NOTICE, "#{client.name} registered the nick after being online for only #{diff} seconds")

      @nicks[client.name] = Hash.new unless @nicks[client.name]  # in case we dont know them already (like services restarted)
      @nicks[client.name]["registered"] = now

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
    if @nicks.has_key?(client.name)
      diff = @nicks[client.name]["registered"] - @nicks[client.name]["client"].firsttime

      if diff < 60
        msg = " (online for #{diff} seconds) "
      end # if diff

    end # if @nicks.has_key
    debug(LOG_NOTICE, "#{client.name} #{msg} CTCP'd #{command}: #{arg}")
  end

# ------------------------------------------------------------------------

  # A new user connected
  def newuser(client)
    debug(LOG_DEBUG, "#{client.name} connected at timestamp #{client.firsttime}")
    @nicks[client.name] = Hash.new
    @nicks[client.name]["client"] = client
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
    @nicks.delete(client.name)
    true
  end # def quit

# ------------------------------------------------------------------------

  # Nick changed, track it. You can run, but you can't hide!
  def nick_changed(client, oldnick)
    debug(LOG_DEBUG, "#{oldnick} is now #{client.name}")
    if not @nicks[oldnick].nil?
      @nicks[client.name] = Hash.new
      @nicks[client.name]["client"] = client
      @nicks[client.name]["registered"] = @nicks[oldnick]["registered"] unless @nicks[oldnick]["registered"].nil?
      @nicks.delete(oldnick)
    end # if not
    true
  end


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
    reason = "#{reason}|#{operreason}"

    #client.host is always filled, check it for the cloak value
    if client.host == "tor-irc.dnsbl.oftc.net"
      debug(LOG_DEBUG, "Using /kill instead of AKILL for Tor user #{client.name}")
      ret = kill_user(client, reason)
    elsif client.host =~ /.*(noc|netop|netrep|chair|ombudsman|advisor).oftc.net/ # should this have an $ ending?
      debug(LOG_DEBUG, "Not issuing AKILL for #{client.name} having cloak #{client.host}, real host #{client.realhost}")
      ret = false # continue with callbacks, we haven't set any kill
    else # if host
      debug(LOG_DEBUG, "Issuing AKILL: *@#{host}, #{reason} lasting for #{@akill_duration} days")
      ret = akill_add("*@#{host}", reason, @akill_duration)
    end # if host

    if channel.length > 0 and not @channels[channel].nil?
      @channels[channel]["kills"]+=1
    end # if channel.lenght

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

    if @channels.has_key?(cname)
      if @channels[cname]["monitoronly"]
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
    if File.exists?("#{@langpath}/ganneffserv-channels.yaml")
      @channels = YAML::load( File.open( "#{@langpath}/ganneffserv-channels.yaml" ) )
    end # if File.exists

    if not @channels
      @channels = Hash.new
    else # if not @channels

      @channels.each_pair do |name, data|
        if data["kills"].nil?
          @channels[name]["kills"] = 0
        else
          @tkills += data["kills"]
        end # if data["kills"].nil?
      end # @channels.each_pair

    end # if not @channels
    debug(LOG_DEBUG, "All channel data successfully loaded")
  end # def load_data

# ------------------------------------------------------------------------

  # Save data to yaml
  def save_data()
    debug(LOG_DEBUG, "Saving channel data")
    File.open("#{@langpath}/ganneffserv-channels.yaml", 'w') do |out|
      YAML.dump(@channels, out)
    end # gs-channels
    debug(LOG_DEBUG, "All channel data successfully saved")
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
    @nicks.each_pair do |nick, data|
      diff = Time.new.to_i - data["client"].firsttime
      debug(LOG_DEBUG, "Looking at #{nick} which I know for #{diff} seconds.")
      if diff > 120
        debug(LOG_DEBUG, "Deleting knowledge of #{nick}, known for #{diff} seconds now.")
        @nicks.delete(nick)
      end # if diff
    end # @nicks.each_pair
    debug(LOG_DEBUG, "Done with all nicks")
    true
  end # def clean

end # class GanneffServ
