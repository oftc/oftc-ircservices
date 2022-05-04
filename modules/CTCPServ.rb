# CTCPServ - monitor various CTCP responses
class CTCPServ < ServiceModule

  require "yaml"
  require "time"

  def initialize()
    service_name("CTCPServ")
    load_language("ctcpserv.en")

    register([
      ['HELP', 0, 2, SFLG_NOMAXPARAM, ADMIN_FLAG, lm('CC_HELP_SHORT'), lm('CC_HELP_LONG')],
      #['ADD', 3, 3, SFLG_NOMAXPARAM, ADMIN_FLAG, lm('CC_HELP_ADD_SHORT'), lm('CC_HELP_ADD_LONG')],
      #['DEL', 1, 1, 0, ADMIN_FLAG, lm('CC_HELP_DEL_SHORT'), lm('CC_HELP_DEL_LONG')],
      ['LIST', 0, 0, 0, ADMIN_FLAG, lm('CC_HELP_LIST_SHORT'), lm('CC_HELP_LIST_LONG')],
      ['CHECK', 1, 1, 0, ADMIN_FLAG, lm('CC_HELP_CHECK_SHORT'), lm('CC_HELP_CHECK_LONG')],
      #['ENFORCE', 0, 0, 0, ADMIN_FLAG, lm('CC_HELP_ENFORCE_SHORT'), lm('CC_HELP_ENFORCE_LONG')],
      ['VERBOSE', 0, 0, 0, ADMIN_FLAG, lm('CC_HELP_VERBOSE_SHORT'), lm('CC_HELP_VERBOSE_LONG')],
      ['RELOAD', 0, 0, 0, ADMIN_FLAG, lm('CC_HELP_RELOAD_SHORT'), lm('CC_HELP_RELOAD_LONG')],
    ])

    @dbq = Hash.new
    @dbq['GET_ALL_VERSION_PATTERNS'] = DB.prepare('
      SELECT ctcpserv_bad_versions.id, pattern, nick, time, monitor_only, kills, reason, active
      FROM ctcpserv_bad_versions, account, nickname
      WHERE ctcpserv_bad_versions.setter = account.id
      AND account.primary_nick = nickname.id
    ')
    @dbq['INSERT_VERSION_PATTERN'] = DB.prepare('
      INSERT INTO ctcpserv_bad_versions(setter, time, pattern, reason, monitor_only)
      VALUES($1, $2, $3, $4, $5)
    ')
    @dbq['DELETE_VERSION_PATTERN'] = DB.prepare('
      DELETE FROM ctcpserv_bad_versions
      WHERE id = $1
    ')
    @dbq['INCREASE_VERSION_PATTERN_KILLS'] = DB.prepare('
      UPDATE ctcpserv_bad_versions
      SET kills = kills + 1
      WHERE id = $1
    ')

    add_hook([
      [NEWUSR_HOOK,     'client_new'],
      [CTCP_REPLY_HOOK, 'ctcp_reply'],
      [QUIT_HOOK,       'client_quit'],
    ])

    load_config

    load_version_patterns

    @nicks = Hash.new
  end

  def load_config()
    File.open("#{CONFIG_PATH}/ctcpserv.yaml", 'r') do |f|
      @config = YAML::load(f)
    end

    if not @config.has_key?('verbose')
      @config['verbose'] = false
    end

    if not @config.has_key?('channel')
      @config['channel'] = '#test'
    end

    if not @config.has_key?('akill_duration')
      @config['akill_duration'] = 30*24*60*60
    end
  end

  def load_version_patterns()
    @version_patterns = []
    @active_version_patterns = []

    result = DB.execute(@dbq['GET_ALL_VERSION_PATTERNS'], '')
    result.row_each do |row|
      SELECT id, pattern, nick, time, monitor_only, kills, reason
      pattern = Hash.new
      pattern['id'] = row[0].to_i
      pattern['pattern'] = row[1]
      pattern['setter'] = row[2]
      pattern['time'] = row[3].to_i
      pattern['monitor_only'] = row[4].to_i.nonzero?
      pattern['kills'] = row[5].to_i
      pattern['reason'] = row[6]
      pattern['active'] = row[7].to_i.nonzero?

      @version_patterns << pattern

      next if not pattern['active']
      begin
        pattern['regexp'] = Regexp.new(pattern['pattern'], Regexp::IGNORECASE)
      rescue Exception => e
        log(LOG_ERROR, "Bad pattern \"#{pattern['pattern']}\"! Error: #{e.to_s}")
        next
      end
      @active_version_patterns << pattern
    end
    result.free
  end

  def HELP(client, parv = [])
    do_help(client, parv[1], parv)

    return true
  end

  def ADD(client, parv = [])
    # NOTIMP
    return true
  end

  def DEL(client, parv = [])
    # NOTIMP
    return true
  end

  def LIST(client, parv = [])
    reply(client, "VERSION patterns\n\n")
    reply(client, "%-4s %-16s %-19s %-3s %-3s %-7s %s" % [ "ID", "By", "When", "Act", "Mon", "Matches", "Reason" ])
    reply(client, " Pattern (quoted)")

    @version_patterns.each do |pattern|
      id = pattern['id']
      setter = pattern['setter']
      time = Time.at(pattern['time']).strftime('%Y-%m-%d %H:%M:%S')
      active = pattern['active']
      mon = if pattern['monitor_only'] then 'T' else 'F' end
      matches = pattern['kills']
      reason = pattern['reason']
      regex = pattern['pattern']

      reply(client, "%-4d %-16s %-19s %-3s %-3s %-7d %s" % [ id, setter, time, active, mon, matches, reason ])
      reply(client, " \"#{regex}\"")
    end

    return true
  end

  def CHECK(client, parv = [])
    c = Client.find(parv[1])
    if c.nil?
      reply("#{parv[1]} not found!")
      return true
    elsif not @nicks.has_key?(c.id)
      reply("#{parv[1]} not known!")
      return true
    end

    sent = Time.at(@nicks[c.id][:request]).strftime("%Y-%m-%d %H:%M:%S")
    received = Time.at(@nicks[c.id][:reply]).strftime("%Y-%m-%d %H:%M:%S")
    matched = @nicks[c.id][:matches].join(', ')

    # TODO: check current matches

    reply(client, "Nick:      #{c.to_str}")
    reply(client, "Sent:      #{sent}")
    reply(client, "Received:  #{received}")
    reply(client, "VERSION:   #{c.ctcp_version}")
    reply(client, "Matched:   #{matched}")
    return true
  end

  def VERBOSE(client, parv = [])
    @config["verbose"] = if @config["verbose"] then false else true end
    log(LOG_NOTICE, "#{client.to_str} toggled VERBOSE; new value: #{@config["verbose"].to_s}")
    reply(client, "Toggled VERBOSE; new value: #{@config["verbose"].to_s}")
    return true
  end

  def RELOAD(client, parv = [])
    load_config
    load_version_patterns
    log(LOG_NOTICE, "#{client.to_str} ran RELOAD")
    reply(client, "RELOAD completed")

    return true
  end

  def snoop(line)
    # This is really gross, but it's the only way to do it during bursting
    send_raw("#{self.client.id} PRIVMSG #{@config['channel']} :#{line}")
  end

  def client_new(client)
    return true if client.is_services_client?

    @nicks[client.id] = Hash.new
    @nicks[:request] = Time.new.to_i
    @nicks[:reply] = 0
    @nicks[:matches] = []

    ctcp_user(client, "VERSION")

    return true
  end

  def ctcp_reply(service, client, command, arg)
    return true unless command == 'VERSION'
    return true unless @nicks.has_key?(client.id)
    return true unless @nicks[client.id][:reply] == 0

    matched = false
    @active_version_patterns.each do |pattern|
      next unless arg =~ pattern['regexp']
      killed = false

      if not pattern['monitor_only']
        log(LOG_NOTICE, "#{client.to_str} matched VERSION pattern #{pattern['id']}, akilling! VERSION: #{arg}")
        snoop("KILL: #{client.to_str} matched VERSION pattern #{pattern['id']}. VERSION: #{arg}")
        if not client.is_tor?
          killed = akill_add("*@#{client.ip_or_hostname}", pattern['reason'], @config['akill_duration'])
        else
          reason, *remainder = pattern['reason'].split('|')
          killed = kill_user(client, reason)
        end
      else
        @nicks[client.id][:matches] << pattern['id']
      end

      pattern['kills'] += 1
      DB.execute_nonquery(@dbq['INCREASE_VERSION_PATTERN_KILLS'], 'i', pattern['id'])

      if killed
        return false
      else
        matched = true
      end
    end

    if matched
      matches = @nicks[client.id][:matches].join(', ')
      log(LOG_NOTICE, "#{client.to_str} matched VERSION patterns #{matches}; VERSION: #{arg}")
      snoop("MONITOR: #{client.to_str} matched VERSION patterns #{matches}; VERSION: #{arg}")
    elsif @config['verbose']
      snoop("VERBOSE: #{client.to_str} VERSION: #{arg}")
    end
    client.ctcp_version = arg
    @nicks[client.id][:reply] = Time.new.to_i
    return true
  end

  def client_quit(client, reason)
    if @nicks.has_key?(client.id)
      @nicks.delete(client.id)
    end
    return true
  end
end
