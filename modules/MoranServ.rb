class MoranServ < ServiceModule

    def initialize()
        service_name("MoranServ")
        load_language("moranserv.en")
        
        @debug = false

        register_commands

        @server_conns = Hash.new
        init_server_conns(30, 10)
        init_server_conns(300, 100)
        init_server_conns(3600, 1000)
    end
    
    def loaded()
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
        ])
    end

    def init_server_conns(time, threshold)
        @server_conns[time] = Hash.new
        @server_conns[time]['threshold'] = threshold
        @server_conns[time]['counts'] = Hash.new
        @server_conns[time]['counts'].default = 0
        @server_conns[time]['event'] = add_event("check_servers", time, time)
        debug("Monitoring for an increase in #{threshold} new connections every #{time} seconds")
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
end
