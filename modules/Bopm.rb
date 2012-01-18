class Bopm < ServiceModule
  require 'resolv'
  require 'yaml'
  require 'ipaddr'

  def initialize()
    service_name('Bopm')
    load_language('bopm.en')

    register([
      ['HELP', 0, 2, SFLG_NOMAXPARAM, USER_FLAG, lm('BP_HELP_SHORT'), lm('BP_HELP_LONG')],
      ['CHECK', 0, 2, 0, USER_FLAG, lm('BP_CHECK_SHORT'), lm('BP_CHECK_LONG')],
      ['PENDING', 0, 2, 0, USER_FLAG, lm('BP_PENDING_SHORT'), lm('BP_PENDING_LONG')],
    ])

    add_hook([
      [NEWUSR_HOOK, 'newuser'],
      [QUIT_HOOK, 'client_quit'],
      ])

    add_event('check_pending', 10, nil)
    add_event('expire_dns_request', 10, nil)

    File.open("#{CONFIG_PATH}/bopm.yaml", 'r') do |f|
      @config = YAML::load(f)
    end

    @config['dnsbls'].each_with_index do |l,i|
      l['priority'] = i
    end

    if @config.has_key?('dnsbl_max_requests')
      @dnsbl_max_requests = @config['dnsbl_max_requests']
    else
      @dnsbl_max_requests = 100
    end

    if @config.has_key?('dnsbl_request_timeout')
      @dnsbl_request_timeout = @config['dnsbl_request_timeout']
    else
      @dnsbl_request_timeout = 300
    end

    @pending_users = Hash.new

    @outstanding_requests = Hash.new
    @requestid = 'AAAAAA'
  end

  def loaded()
    log(LOG_DEBUG, "Loaded ...")
  end

  def HELP(client, parv = [])
    do_help(client, parv[1], parv)
  end

  def PENDING(client, parv = [])
    if client.is_oper? or client.is_admin?
      reply(client, "#{@outstanding_requests.keys.length} requests dispatched")
      reply(client, "#{@pending_users.keys.length} users to check")
    end
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
    @pending_users.delete(client.id) if @pending_users.has_key?(client.id)
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

  def get_priority(results)
    results.each do |r|
      if r and r.length > 0
        return r
      end
    end

    return []
  end

  def lookup_cb(addrs, args)
    entry = args['current']
    reqid = args['reqid']
    score = 0
    stop = false
    results = nil

    if not @outstanding_requests.has_key?(reqid)
      return
    end
    
    req = @outstanding_requests[reqid]
    req['count'] -= 1

    if not req['results'][entry['priority']]
      req['results'][entry['priority']] = []
      results = req['results'][entry['priority']]
    end

    log(LOG_DEBUG, "lookup_cb fired #{req['host']} #{entry['name']} #{req['count']} lists left")
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

    if entry.has_key?('hexip')
      hexip = entry['hexip']
    else
      hexip = false
    end

    log(LOG_DEBUG, "hexip: #{hexip}")

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

      log(LOG_DEBUG, "#{req['host']} in #{entry['name']} (#{entry_score}) [#{entry_reason}]")

      results << [entry['name'], addr, entry_score, entry_reason, entry['cloak'], withid, hexip]
    end

    if entry_count > 0
      req['shortnames'] << "#{entry['shortname']}: #{entry_count}"
    end

    req['score'] += score

    really_stop = false
    if stop and req['allow_stop']
      really_stop = true
    end

    if req['count'] == 0 or really_stop
      finish_request(reqid)
      check_pending(nil)
    end
  end

  def finish_request(reqid)
    log(LOG_DEBUG, "Firing final cb request: #{reqid}")
    if @outstanding_requests.has_key?(reqid)
      req = @outstanding_requests[reqid]
      @outstanding_requests.delete(reqid)
      req['final'].call(req['host'], req['score'], req['results'], req['shortnames'], req['final_data'])
    end
  end

  def newuser_final(host, score, blacklists, short_names, cid)
    client = Client.find(cid)
    r = get_priority(blacklists)
    if r.length > 0
      name, addr, escore, reason, cloak, withid, hexip = r[0]
      snames = short_names.join(", ")
      if score >= @config['kill_score']
        if client
          channels = []
          client.channel_each do |channel|
            channels << channel.name
          end
          channels = channels.join(", ")
          lmsg = "KILLING #{client.to_str} with score: #{score} [#{snames}] [#{channels}]"
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
          do_cloak = false
          if !exempt_host(client.host) and client.host != cloak
            do_cloak = true
          end

          if @config['recloak_users'] or do_cloak
            log(LOG_NOTICE, "CLOAK #{client.to_str} to #{cloak} score: #{score}")
            cloakstr = "#{cloak}"

            if withid
              cloakstr = "#{client.id}.#{cloakstr}"
            end

            if hexip and !client.sockhost.nil?
              chex = IPAddr.new(client.sockhost).to_i.to_s(16)
              cloakstr = "#{chex}.#{cloakstr}"
            end

            client.cloak(cloakstr)
          else
            log(LOG_DEBUG, "Not cloaking #{client.to_str} to #{cloak}")
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
      blacklists.each do |b|
        b.each do |name,addr,escore,reason,cloak,withid|
          reply(client, "Found in #{name} (#{addr}) [#{reason}] score: #{escore}")
        end
      end
      reply(client, "Total score: #{score}")
    end
  end

  def newuser(client)
    if not client.is_services_client?
      if @outstanding_requests.keys.length > @dnsbl_max_requests
        @pending_users[client.id] = client
        log(LOG_DEBUG, "Add client to pending_list: #{client.to_str}")
      else
        dispatch_client(client)
      end
    end

    return true
  end

  def dispatch_client(client)
    host = client.ip_or_hostname
    if host.nil?
      log(LOG_DEBUG, "Failed to get reverse host for: #{client.to_str}")
    else
      dnsbl_check(host, true, method(:newuser_final), client.id)
    end
  end

  def check_pending(arg)
    if @outstanding_requests.keys.length < @dnsbl_max_requests and @pending_users.keys.length > 0
      id = @pending_users.keys.shift
      client = @pending_users[id]
      @pending_users.delete(id)
      dispatch_client(client)
    end
  end

  def expire_dns_request(arg)
    @outstanding_requests.keys.each do |reqid|
      req = @outstanding_requests[reqid]
      delta = req['ttl'] - Time.now.to_i
      if delta < 0
        log(LOG_DEBUG, "Request #{reqid} timed out")
        finish_request(reqid) if delta < 0
      end
    end
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

    req = Hash.new
    req['count'] = blacklists.length
    req['host'] = host
    req['revhost'] = to_revip(host)
    req['score'] = 0
    req['allow_stop'] = allow_stop
    req['final'] = final
    req['final_data'] = final_data
    req['shortnames'] = []
    req['results'] = Array.new(blacklists.length)
    req['ttl'] = Time.now.to_i + @dnsbl_request_timeout

    reqid = @requestid.succ
    @outstanding_requests[reqid] = req
    @requestid.succ!

    log(LOG_DEBUG, "Creating request #{@requestid} for #{host}")

    blacklists.each do |list|
      args = Hash.new
      args['reqid'] = reqid
      args['current'] = list
      log(LOG_DEBUG, "Dispatching #{req['revhost']}.#{list['name']} dns_lookup")
      dns_lookup(req['revhost'] + '.' + list['name'], method(:lookup_cb), args)
    end
  end

  def exempt_host(host)
    if @config.has_key?('exempt_hosts') and @config['exempt_hosts'].length > 0
      @config['exempt_hosts'].each do |h|
        r = Regexp.new(h+'$')
        if r.match(host)
          log(LOG_DEBUG, "#{host} is exempted by rule #{h}")
          return true
        end
      end
    end
    log(LOG_DEBUG, "#{host} is not exempt")
    return false
  end
end
