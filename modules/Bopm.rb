class Bopm < ServiceModule
  require 'resolv'

  def initialize()
    service_name('Bopm')
    load_language('bopm.en')

    register([
      ['HELP', 0, 2, SFLG_UNREGOK|SFLG_NOMAXPARAM, ADMIN_FLAG, lm('BP_HELP_SHORT'), lm('BP_HELP_LONG')],
    ])

    add_hook([[NEWUSR_HOOK, 'newuser']])

    @list = [
      ['tor-irc.dnsbl.oftc.net', 'tor-irc.dnsbl.oftc.net'],
      ['dnsbl.dronebl.org', 'dronebl.dnsbl.oftc.net'],
      ['xbl.spamhaus.org', 'xbl.dnsbl.oftc.net'],
      ['web.dnsbl.sorbs.net', 'websorbs.dnsbl.oftc.net'],
      ['proxies.dnsbl.sorbs.net', 'sorbs.dnsbl.oftc.net'],
      ['dnsbl.ahbl.org', 'ahbl.dnsbl.oftc.net'],
    ]
  end

  def loaded()
  end

  def HELP(client, parv = [])
    do_help(client, parv[1], parv)
  end

  def newuser(client)
    blacklists = dnsbl_check(client)

    return true
  end

  def dnsbl_check(client)
    if client.realhost.nil?
      host = client.host
    else
      host = client.realhost
    end
    
    if not host.match(/\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}/)
      host = Resolv::getaddress(host)
    end

    host = host.split('.').reverse.join('.')

    results = []
    @list.each do |l|
      begin
        check = host + '.' + l[0]
        log(LOG_NOTICE, check)
        results << [Resolv::getaddress(check), l[1]]
      rescue Exception => e
        log(LOG_NOTICE, e.to_str)
        next
      end
    end

    if results.length > 0
      client.cloak(results[0][0])
    end

    results.each do |bl, r|
      log(LOG_NOTICE, '%s found in %s (%s)' % [client.name, bl, r])
    end

    return results
  end
end
