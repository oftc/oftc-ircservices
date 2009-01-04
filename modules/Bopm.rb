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
        orig = get_realhost(c)
      end
    else
      # no parameters just check the caller
      reply(client, "Checking your DNSBL status");
      orig = get_realhost(client)
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
        addr  = Resolv::getaddress(check)
        reply(client, "#{orig} is #{addr} according to #{parv[2]}")
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
      results.each do |entry,addr|
        reply(client, "Found in #{entry['name']} (#{addr}) [#{entry['codes'][addr]}] score: #{entry['score']}")
      end
      reply(client, "Total score: #{score}")
    end
  end

  def newuser(client)
    return true unless @ready

    orig = get_realhost(client)
    return if orig.nil?
    host = to_revip(orig)
    score, blacklists = dnsbl_check(host)

    if blacklists.length > 0
      entry,addr = blacklists[0]
      cloak = entry['cloak']
      log(LOG_NOTICE, "CLOAK #{client.to_str} to #{cloak} score: #{score}")
      client.cloak(cloak)
    end

    return true
  end

  def get_realhost(client)
    # if realhost is nil there is no cloak
    if client.realhost.nil?
      host = client.host
    else # we have a cloak
      host = client.realhost
    end

    return host
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
      begin
        check = host + '.' + entry['name']
        addr = Resolv::getaddress(check)
        score += entry['score'].to_int if entry['codes'].has_key?(addr)
        log(LOG_DEBUG, "#{host} in #{entry['name']} (#{entry['score']}) [#{entry['codes'][addr]}]")
        cloak = entry['cloak']
        results << [entry, addr]
      rescue Exception => e
        #log(LOG_DEBUG, "#{check} -- #{e.to_str}")
        next
      end
    end

    return score, results
  end
end
