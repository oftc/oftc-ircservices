class MoranServ < ServiceModule
    require 'yaml'
    require 'ipaddr'

    def initialize()
        service_name("MoranServ")
        load_language("moranserv.en")
        
        @debug = false

        register_commands

        File.open("#{CONFIG_PATH}/moran.yaml", 'r') do |f|
            @config = YAML::load(f)
        end

        @server_conns = Hash.new
        @config['times'].each do |entry|
            init_server_conns(entry['time'], entry['warn'])
        end

        @track = []
        @track_ids = Hash.new

        @track_notices = Hash.new

        add_hook([
          [NEWUSR_HOOK, 'client_new'],
          [QUIT_HOOK, 'client_quit'],
          [JOIN_HOOK, 'client_join'],
          [PART_HOOK, 'client_part'],
        ])

        add_event('drain_notices', 10, nil)
    end

    def loaded
        @server_conns.each_key{|time|
            init_server_load(time)
        }
    end

    def check_servers(time)
        debug("Checking server list for interval #{time}")
        Client.all_servers_each {|server|
            count = server.client_length
            old_count = @server_conns[time]['counts'][server.name]
            delta = count - old_count
            threshold = @server_conns[time]['threshold']
            debug("! #{server.name}: was #{old_count}, now #{count}, threshold #{threshold}")
            if delta.abs > threshold
                dir = delta > 0 ? '+' : '-'
                notice("#{server.name} had #{dir}#{delta.abs} connections in #{time} seconds (total clients: #{count}; threshold: |#{@server_conns[time]['threshold']}|)")
            end
            @server_conns[time]['counts'][server.name] = count
        }
    end
    
    def HELP(client, parv = [])
        debug("#{client.name} called HELP")
        do_help(client, parv[1], parv)
        return true
    end

    def DEBUG(client, parv = [])
        debug("#{client.name} called DEBUG")
        toggle_debug
        reply(client, "Debug mode is now #{@debug}")
        notice("#{client.name} set DEBUG to #{@debug}")
        return true
    end

    def THRESHOLD(client, parv = [])
        debug("#{client.name} called THRESHOLD")
        if parv[1].downcase == 'list'
            return list_threshold(client)
        elsif parv.length < 3
            do_help(client, 'threshold', parv)
            return true
        end
        begin
            time = Integer(parv[1])
        rescue
            reply(client, "Could not parse an integer from the time argument")
            return true
        end
        begin
            new_threshold = Integer(parv[2])
        rescue
            reply(client, "Could not parse an integer from the threshold argument")
            return true
        end
        if not @server_conns.has_key?(time)
            reply(client, "#{time} seconds is not a registered interval")
            return true
        end
        if new_threshold < 0
            reply(client, "#{new_threshold} is negative. Don't do that")
            return true
        end
        @server_conns[time]['threshold'] = new_threshold
        reply(client, "#{time} second THRESHOLD is now #{new_threshold}")
        notice("#{client.name} set #{time} second THRESHOLD to #{new_threshold}")
    end

    def TRACK(client, parv = [])
      # TRACK <nick>
      # TRACK <regexp>
      # TRACK <ip>
      # TRACK
      if parv.length == 1
        reply(client, "Track list")
        @track.each do |t|
          reply(client, "#{t['type']} -- #{t['value']}")
        end
        reply(client, "End of track list")
      else
        arg = parv[1]
        track_client = Client.find(arg)
        type = 'regexp'
        value = arg
        if track_client
          type = 'client'
          value = track_client.id
          @track_ids[track_client.id] = true
        else
          begin
            addr = IPAddr.new(arg)
            type = 'ip'
            value = arg
          rescue Exception => e
          end
        end
        t = Hash.new
        t['type'] = type
        t['value'] = value
        @track << t
        reply(client, "Added track of #{type} for #{value}")
        notice("#{client.name} Added track of #{type} for #{value}")
      end
    end

    def UNTRACK(client, parv = [])
      arg = parv[1]
      if arg == '-'
        @track = []
        @track_ids = Hash.new
        reply(client, "No longer tracking any thing")
        notice("#{client.name} has removed all tracking")
      else
        @track.length.times do |i|
          if @track[i]['value'] == arg
            if @track[i]['type'] == 'client'
              @track_ids.delete(@track[i]['value'])
            end
            @track.delete_at(i)
            reply(client, "No longer tracking #{arg}")
            notice("#{client.name} has removed tracking for #{arg}")
            return
          end
        end
        reply(client, "Not currently tracking #{arg}")
      end
    end

    private

    def list_threshold(client)
        debug("#{client.name} called list_threshold")
        reply(client, "Current threshold list")
        @server_conns.keys.sort.each do |time|
            reply(client, "#{time} seconds: #{@server_conns[time]['threshold']} connections")
        end
        reply(client, "End of list")
        return true
    end

    def toggle_debug
        if @debug
            @debug = false
        else
            @debug = true
        end
    end

    def register_commands
        register([
                 ["HELP", 0, 2, SFLG_UNREGOK|SFLG_NOMAXPARAM, ADMIN_FLAG, lm('MS_HELP_SHORT'), lm('MS_HELP_LONG')],
                 ["DEBUG", 0, 0, 0, ADMIN_FLAG, lm('MS_HELP_DEBUG_SHORT'), lm('MS_HELP_DEBUG_LONG')],
                 ["THRESHOLD", 1, 2, 0, ADMIN_FLAG, lm('MS_HELP_THRESHOLD_SHORT'), lm('MS_HELP_THRESHOLD_LONG')],
                 ["TRACK", 0, 1, 0, ADMIN_FLAG, lm('MS_HELP_TRACK_SHORT'), lm('MS_HELP_TRACK_LONG')],
                 ["UNTRACK", 0, 1, 0, ADMIN_FLAG, lm('MS_HELP_UNTRACK_SHORT'), lm('MS_HELP_UNTRACK_LONG')],
        ])
    end

    def init_server_conns(time, threshold)
        @server_conns[time] = Hash.new
        @server_conns[time]['threshold'] = threshold
        @server_conns[time]['counts'] = Hash.new
        @server_conns[time]['counts'].default = 0
        @server_conns[time]['event'] = add_event("check_servers", time, time)
        debug("Monitoring for a change of #{threshold} connections every #{time} seconds")
    end

    def init_server_load(time)
        debug("Getting initial server loading for #{time} second period")
        Client.all_servers_each {|server|
            count = server.client_length
            debug("#{server.name} has #{count} clients")
            @server_conns[time]['counts'][server.name] = count
        }
    end

    def debug(message)
        if @debug
            log(LOG_NOTICE, message)
        else
            log(LOG_DEBUG, message)
        end
    end

    def notice(message)
        log(LOG_NOTICE, message)
    end

    def track_event(client)
      if @track_ids.has_key?(client.id)
        t = Hash.new
        t['type'] = 'client'
        t['value'] = client.id
        return true, t
      else
        @track.each do |t|
          case t['type']
            when 'ip'
              begin
                addr = IPAddr.new(t['value'])
                if addr.include?(client.ip_or_hostname)
                  return true, t
                end
              rescue Exception => e
              end
            when 'regexp'
              begin
                rxp = Regexp.new(t['value'])
                if rxp.match(client.host) or rxp.match(client.realhost) or rxp.match(client.name)
                  return true, t
                end
              rescue
              end
          end
        end
      end
      return false, nil
    end

    def add_notice(track, message)
      key = track['value']
      @track_notices[key] = [] unless @track_notices.has_key?(key)
      @track_notices[key] << message
    end

    def client_new(client)
      tevent, t = track_event(client)
      if tevent
        add_notice(t, "C:#{client.name}")
      end
      return true
    end

    def client_quit(client, reason)
      tevent, t = track_event(client)
      if tevent
        add_notice(t, "Q:#{client.name}:#{reason}")
      end
      if @track_ids.has_key?(client.id)
        @track_ids.delete(client.id)
        @track.length.times do |i|
          if @track['type'] == 'client' and @track['value'] == client.id
            @track.delete_at(i)
          end
        end
      end
      return true
    end

    def client_join(client, channel)
      tevent, t = track_event(client)
      if tevent
        add_notice(t, "J:#{client.name}:#{channel}")
      end
      return true
    end

    def client_part(source, client, channel, reason)
      tevent, t = track_event(client)
      if tevent
        add_notice(t, "P:#{client.name}:#{channel.name}:#{reason}")
      end
      return true
    end

    def drain_notices(data)
      @track_notices.each do |key,value|
        msg = value.join(", ")[0,400]
        notice("#{key} => #{msg}")
      end
      @track_notices = Hash.new
    end
end
