class Bopm < ServiceModule
  require 'resolv'
  require 'yaml'

  def initialize()
    service_name('Bopm')
    load_language('bopm.en')

    register([
      ['HELP', 0, 2, SFLG_NOMAXPARAM, USER_FLAG, lm('BP_HELP_SHORT'), lm('BP_HELP_LONG')],
      ['CHECK', 0, 2, 0, USER_FLAG, lm('BP_HELP_SHORT'), lm('BP_HELP_LONG')],
    ])

    add_hook([
      [NEWUSR_HOOK, 'newuser'],
      [EOB_HOOK, 'eob'],
      ])

    File.open("#{CONFIG_PATH}/bopm.yaml", 'r') do |f|
      @config = YAML::load(f)
    end

    @ready = false
  end

  def loaded()
  end

  def HELP(client, parv = [])
    do_help(client, parv[1], parv)
  end

  def CHECK(client, parv = [])
    if parv.length >= 2
      c = Client.find(parv[1])
      if c.nil?
        reply(client, "Could not find #{parv[1]}")
        orig = parv[1]
      else
        reply(client, "Checking #{c.name}");
        orig = get_realhost(c)
      end
    else
      reply(client, "Checking your DNSBL status");
      orig = get_realhost(client)
    end

    host = to_revip(orig)

    if host.nil?
      reply(client, "#{orig} could not be reversed")
      return
    end

    if parv.length > 2
      begin
        check = host + '.' + parv[2]
        addr  = Resolv::getaddress(check)
        reply(client, "#{orig} is #{addr} according to #{parv[2]}")
      rescue
        reply(client, "#{orig} was not found in #{parv[2]}")
      end
    else
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

  def eob()
    @ready = true
  end

  def newuser(client)
    return true unless @ready

    host = get_realhost(client)
    host = to_revip(host)
    score, blacklists = dnsbl_check(host) unless host.nil?

    if blacklists.length > 0
      cloak = blacklists[0][0]['cloak']
      #client.cloak(blacklists[0][0]['cloak'])
      log(LOG_DEBUG, "CLOAK #{client.name} to #{cloak} score: #{score}")
    end

    return true
  end

  def get_realhost(client)
    if client.realhost.nil?
      host = client.host
    else
      host = client.realhost
    end

    return host
  end

  def to_revip(host)
    if not host.match(/\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}/)
      begin
        host = Resolv::getaddress(host)
      rescue
        host = nil
      end
    end

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
        log(LOG_DEBUG, "#{check} -- #{e.to_str}")
        next
      end
    end

    return score, results
  end
end
