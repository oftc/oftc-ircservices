class JupeServ < ServiceModule
  def initialize
    service_name("JupeServ")
    load_language("jupeserv.en")
    register([
      ["HELP", 0, 2, SFLG_NOMAXPARAM, ADMIN_FLAG, lm('JS_HELP_SHORT'), lm('JS_HELP_LONG')],
      ["JUPE", 1, 2, SFLG_NOMAXPARAM, ADMIN_FLAG, lm('JS_HELP_JUPE_SHORT'), lm('JS_HELP_JUPE_LONG')],
      ["LIST", 0, 0, 0, ADMIN_FLAG, lm('JS_HELP_LIST_SHORT'), lm('JS_HELP_LIST_LONG')],
    ])
    add_hook([
        [QUIT_HOOK, 'squit']
    ])
    @jupes = Jupes.new
  end
  
  def squit(client, reason)
    log(LOG_DEBUG, "JupeServ SQUIT #{client.name} #{reason}")
    cjupe = @jupes.find(client.name)
    if cjupe != nil
      @jupes.remove(client.name)
      log(LOG_INFO, "Removed Jupiter on #{client.name}")
    end
  end
  
  def HELP(client, parv = [])
    do_help(client, parv[1], parv)
  end
  def JUPE(client, parv = [])
	  reason = ""
    parv.shift
    if @jupes.find(parv[0])
    	reply_user(client, "Server #{parv[0]} already juped")
	    return
    end
		if(parv.length == 1)
	    server = introduce_server(parv[0], "Jupitered")
			log(LOG_INFO, "Jupitered Server #{parv[0]}")
		else
			sname = parv.shift
			reason = parv.length > 0 ? parv.join(' ') : "No Reason"
	    server = introduce_server(sname, "Jupitered: #{reason}")
			log(LOG_INFO, "Jupitered Server #{sname} #{reason}")
		end

    @jupes.jupe(client, server, reason)
    reply_user(client, "Jupitered #{parv[0]}")
  end

  def LIST(client, parv = [])
    if @jupes.size == 0
      reply_user(client, "No Jupe currently installed.")
      return
    end
    
    reply_user(client, "Currently #{@jupes.size} Jupes active:")
    @jupes.each do |jupe|
      reply_user(client, "  - #{jupe.server.name} by #{jupe.client.name} on #{jupe.datetime}")
    end 
    reply_user(client, "End of List.")
  end
end

class Jupe
	attr_accessor :server, :client, :datetime
end

class Jupes 
	def initialize
		@jupes = Hash.new
	end
	def find(s)
		return @jupes[s]
	end
	
	def remove(s)
	  @jupes.delete(s)
	end

	def jupe(c,s, r)
		jupe = Jupe.new
		jupe.server = s
		jupe.client = c
		jupe.datetime = Time.new
		jupe.reason = r
		@jupes[s.name] = jupe
	end

	def each
		@jupes.each do  |k,v|
			yield v
		end
	end

	def size
		return @jupes.size
	end
end

