class Bopm < ServiceModule
  require 'resolv'
  require 'yaml'

  def initialize()
    service_name('Bopm')
    load_language('bopm.en')

    register([
      ['HELP', 0, 2, SFLG_UNREGOK|SFLG_NOMAXPARAM, ADMIN_FLAG, lm('BP_HELP_SHORT'), lm('BP_HELP_LONG')],
    ])

    add_hook([[NEWUSR_HOOK, 'newuser']])

    File.open("#{CONFIG_PATH}/bopm.yaml", 'r') do |f|
      @config = YAML::load(f)
    end
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

    score = 0
    cloak = nil
    @config.each do |entry|
      begin
        check = host + '.' + entry['name']
        addr = Resolv::getaddress(check)
        score += entry['score'].to_int if entry['codes'].has_key?(addr)
        log(LOG_DEBUG, "#{client.host} in #{entry['name']} (#{entry['score']}) [#{entry['codes'][addr]}]")
        cloak = entry['cloak']
      rescue Exception => e
        log(LOG_DEBUG, "#{check} -- #{e.to_str}")
        next
      end
    end

    client.cloak(cloak) unless cloak.nil?

    return score
  end
end
