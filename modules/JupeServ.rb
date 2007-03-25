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

	def jupe(c,s)
		jupe = Jupe.new
		jupe.server = s
		jupe.client = c
		jupe.datetime = Time.new
		@jupes[s] = jupe
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

class JupeServ < ServiceModule
  def initialize
    service_name("JupeServ")
    register(["HELP", "JUPE", "LIST"])
    @jupes = Jupe.new
  end
  
  def HELP(client, parv = [])
    log(ServiceModule::LOG_DEBUG, "JupeServ::Help")
    reply_user(client, "HELP | JUPE | LIST")
  end
  def JUPE(client, parv = [])
    parv.shift
    if @jupes.find(parv[0])
    	reply_user(client, "Server " + parv[0] + "already juped")
	return
    end
    introduce_server(parv[0], "Jupitered")
    @jupes.jupe(client, parv[0])
    reply_user(client, "Jupitered " + parv[0])
  end

  def LIST(client, parv = [])
    if @jupes.size == 0
      reply_user(client, "No Jupe currently installed.")
      return
    end
    
    reply_user(client, "Currently #{@jupes.size} Jupes active:")
    @jupes.each do |jupe|
      reply_user(client, "  - #{jupe.server} by #{jupe.client} on #{jupe.datetime}")
    end 
    reply_user(client, "End of List.")
  end
end
