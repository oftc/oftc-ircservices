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
      ])

    File.open("#{CONFIG_PATH}/bopm.yaml", 'r') do |f|
      @config = YAML::load(f)
    end

    @ready = false
  end

  def loaded()
    @ready = true
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
      begin
        check = host + '.' + parv[2]
        addrs  = Resolv::getaddresses(check)
        addrs.each do |addr|
          reply(client, "#{orig} is #{addr} according to #{parv[2]}")
        end
      rescue
        reply(client, "#{orig} was not found in #{parv[2]}")
      end
    else
      # pass the hostname to check against our configured blacklists
      score, results = dnsbl_check(host)

      if results.length == 0
        reply(client, "No results for #{orig}")
        return
      end

      reply(client, "Results for #{orig}")
      results.each do |name,addr,escore,reason,cloak,withid|
        reply(client, "Found in #{name} (#{addr}) [#{reason}] score: #{escore}")
      end
      reply(client, "Total score: #{score}")
    end
  end

  def newuser(client)
    return true unless @ready

    host = to_revip(client.ip_or_hostname)

    if host.nil?
      log(LOG_DEBUG, "Failed to get reverse host for: #{client.to_str}")
      return true
    end

    score, blacklists = dnsbl_check(host)

    if blacklists.length > 0
      name,addr,escore,reason,cloak,withid = blacklists[0]
      log(LOG_NOTICE, "CLOAK #{client.to_str} to #{cloak} score: #{score}")
      if withid
        client.cloak("#{client.id}.#{cloak}")
      else
        client.cloak(cloak)
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

  def dnsbl_check(host)
    score = 0
    cloak = nil
    results = []
    @config.each do |entry|
      log(LOG_DEBUG, "Checking #{host} against #{entry['name']}")
      begin
        check = host + '.' + entry['name']
        addrs = Resolv::getaddresses(check)

        stop = false

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
          end

          score += entry_score

          log(LOG_DEBUG, "#{host} in #{entry['name']} (#{entry_score}) [#{entry_reason}]")

          results << [entry['name'], addr, entry_score, entry_reason, entry['cloak'], withid]
        end

        if stop
          break
        end
      rescue Exception => e
        log(LOG_DEBUG, "#{check} -- #{e.to_str}")
        # unless I'm an idiot the dnsbl doesn't have this entry
        next
      end
    end

    return score, results
  end
end
