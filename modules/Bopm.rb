class Bopm < ServiceModule
  require 'resolv'
  require 'yaml'

  def initialize()
    service_name('Bopm')
    load_language('bopm.en')

    register([
      ['HELP', 0, 2, SFLG_NOMAXPARAM, USER_FLAG, lm('BP_HELP_SHORT'), lm('BP_HELP_LONG')],
      ['CHECK', 0, 2, 0, USER_FLAG, lm('BP_CHECK_SHORT'), lm('BP_CHECK_LONG')],
    ])

    add_hook([
      [NEWUSR_HOOK, 'newuser'],
      [QUIT_HOOK, 'client_quit'],
      ])

    File.open("#{CONFIG_PATH}/bopm.yaml", 'r') do |f|
      @config = YAML::load(f)
    end
  end

  def loaded()
    log(LOG_DEBUG, "Loaded ...")
  end

  def HELP(client, parv = [])
    do_help(client, parv[1], parv)
  end

  def CHECK(client, parv = [])
    if parv.length > 1 and not (client.is_oper? or client.is_admin?)
      reply(client, "You don't have permission for this action")
      return
    end

    if parv.length >= 2
      # check to see if this is a client first
      c = Client.find(parv[1])
      if c.nil?
        # we couldn't find the client, assume it's a hostname
        orig = parv[1]
      else
        # it is a client on the network
        reply(client, "Checking #{c.name}");
        orig = c.ip_or_hostname
      end
    else
      # no parameters just check the caller
      reply(client, "Checking your DNSBL status");
      orig = client.ip_or_hostname
    end

    # resolve a hostname, and reverse the order of the ip
    host = to_revip(orig)

    # a hostname could not be found (likely a cloak or a client that doesn't exist)
    if host.nil?
      reply(client, "#{orig} is not on the network or is an invalid hostname")
      return
    end

    if parv.length > 2
      # We were given a specific dnsbl to check against
      check = host + '.' + parv[2]
      args = Hash.new
      args['cid'] = client.id
      args['host'] = orig
      args['list'] = parv[2]
      dns_lookup(check, method(:check_user_at_cb), args)
    else
      # pass the hostname to check against our configured blacklists
      dnsbl_check(orig, false, method(:check_user_final), client.id)
    end
  end

  def client_quit(client, reason)
    return true
  end

  def check_user_at_cb(addrs, args)
    client = Client.find(args['cid'])
    if client
      if addrs.length == 0
        reply(client, "#{args['host']} was not found in #{args['list']}")
      else
        addrs.each do |addr|
          reply(client, "#{args['host']} is #{addr} according to #{args['list']}")
        end
      end
    end
  end

  def lookup_cb(addrs, args)
    entry = args['current']
    score = 0
    stop = false

    log(LOG_DEBUG, "lookup_cb fired #{args['host']} #{entry['name']}")
    # fallback score if a specific result doesn't have a score
    if entry.has_key?('score')
      default_score = entry['score']
    else
      default_score = 0
    end

    if entry.has_key?('withid')
      withid = entry['withid']
    else
      withid = false
    end

    entry_score = default_score
    entry_reason = ''
    entry_count = 0

    addrs.each do |addr|
      # the codes are optional altogether
      if entry.has_key?('codes') and entry['codes'].has_key?(addr)
        # the score for a given result is optional
        entry_score = entry['codes'][addr]['score'] if entry['codes'][addr].has_key?('score')
        # the reason is optional
        entry_reason = entry['codes'][addr]['reason'] if entry['codes'][addr].has_key?('reason')
        # stoplookups also optional
        # if this host has any result don't continue
        if entry.has_key?('stoplookups')
          stop = entry['stoplookups']
        else
          stop = false
        end
        stop = entry['codes'][addr]['stoplookups'] if entry['codes'][addr].has_key?('stoplookups')
        entry_count += 1
      end
      score += entry_score

      log(LOG_DEBUG, "#{args['host']} in #{entry['name']} (#{entry_score}) [#{entry_reason}]")

      args['results'] << [entry['name'], addr, entry_score, entry_reason, entry['cloak'], withid]
    end

    if entry_count > 0
      args['shortnames'] << "#{entry['shortname']}: #{entry_count}"
    end

    args['score'] += score

    if args['blacklists'].length == 0 or (stop and args['allow_stop'])
      log(LOG_DEBUG, "Firing final cb")
      args['final'].call(args['host'], args['score'], args['results'], args['shortnames'], args['final_data'])
    else
      log(LOG_DEBUG, "Chaining CB")
      args['current'] = args['blacklists'].shift
      dns_lookup(args['revhost'] + '.' + args['current']['name'], method(:lookup_cb), args)
    end
  end

  def newuser_final(host, score, blacklists, short_names, cid)
    client = Client.find(cid)
    if blacklists.length > 0
      name, addr, escore, reason, cloak, withid = blacklists[0]
      snames = short_names.join(", ")
      if score >= @config['kill_score']
        if client
          lmsg = "KILLING #{client.to_str} with score: #{score} [#{snames}]"
        else
          lmsg = "KILLING #{host} with score: #{score} [#{snames}]"
        end

        log(LOG_NOTICE, lmsg)

        if @config['store_kill_directly']
          if client
            mask = client.ip_or_hostname
          else
            mask = host
          end
          akill_add("*@#{mask}", @config['kill_reason'], @config['kill_duration'])
        else
          if client
            mask = client.ip_or_hostname
          else
            mask = host
          end
          msg = @config['kill_command'].dup
          msg.sub!('$HOSTNAME$', "#{mask}")
          msg.sub!('$REASON$', "#{@config['kill_reason']}")
          msg.sub!('$DURATION$', "#{@config['kill_duration']}")
          msg.sub!('$SCORE$', "#{score}")
          msg.sub!('$CLOAK$', "#{cloak}")
          send_raw(msg)
        end
      else 
        if client
          log(LOG_NOTICE, "CLOAK #{client.to_str} to #{cloak} score: #{score}")
          if withid
            client.cloak("#{client.id}.#{cloak}")
          else
            client.cloak(cloak)
          end
        else
          log(LOG_DEBUG, "client disappeared before bopm could act")
        end
      end
    end
  end

  def check_user_final(host, score, blacklists, short_names, cid)
    client = Client.find(cid)
    if client
      if blacklists.length == 0
        reply(client, "No results for #{host}")
        return
      end
    
      reply(client, "Results for #{host}")
      blacklists.each do |name,addr,escore,reason,cloak,withid|
        reply(client, "Found in #{name} (#{addr}) [#{reason}] score: #{escore}")
      end
      reply(client, "Total score: #{score}")
    end
  end

  def newuser(client)
    log(LOG_DEBUG, "Add client to pending_list: #{client.to_str}")
    if not client.is_services_client?
      host = client.ip_or_hostname
      if host.nil?
        log(LOG_DEBUG, "Failed to get reverse host for: #{client.to_str}")
      else
        dnsbl_check(host, true, method(:newuser_final), client.id)
      end
    end

    return true
  end

  def to_revip(host)
    # if the host is not an ip we need to resolve it
    if not host.match(Resolv::AddressRegex)
      begin
        host = Resolv::getaddress(host)
      rescue
        # the host could not be resolved
        host = nil
      end
    end

    # reverse the ip
    host = host.split('.').reverse.join('.') unless host.nil?

    return host
  end

  def dnsbl_check(host, allow_stop, final, final_data)
    blacklists = @config['dnsbls'].dup
    args = Hash.new
    args['host'] = host
    args['revhost'] = to_revip(host)
    args['current'] = blacklists.shift
    args['blacklists'] = blacklists
    args['score'] = 0
    args['allow_stop'] = allow_stop
    args['final'] = final
    args['final_data'] = final_data
    args['shortnames'] = []
    args['results'] = []

    log(LOG_DEBUG, "Dispatching #{args['revhost']}.#{args['current']['name']} dns_lookup")
    dns_lookup(args['revhost'] + '.' + args['current']['name'], method(:lookup_cb), args)
  end
end
