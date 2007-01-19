class JupeServ < ServiceModule
  def initialize
    service_name("JupeServ")
    register(["HELP", "ADD", "LIST"])
  end
  def HELP(client, parv = [])
    log(ServiceModule::LOG_DEBUG, "JupeServ::Help")
    reply_user(client, "You want help!?")
  end
  def ADD(client, parv = [])
    parv.shift
    reply_user(client, "You want me to jupe " + parv[0] + "?!")
  end
  def LIST(client, parv = [])
    reply_user(client, "LIST!? ARE YOU INSANE?!")
  end
end
