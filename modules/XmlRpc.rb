class MyHandler
  def sumAndDifference(a, b)
    { "sum" => a + b, "difference" => a - b }
  end
end
 
class XmlRpc < ServiceModule
  require "xmlrpc/server"
  def initialize
    service_name("XmlRpc")
    begin
      foo = Thread.new {
        
        @s = XMLRPC::Server.new(8080)  
        @s.add_handler("sample", MyHandler.new)
        @s.serve
      }
    rescue Exception => ex
      puts ex
    end
  end
end
