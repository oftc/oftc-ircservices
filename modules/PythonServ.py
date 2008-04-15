from ServiceModule import Service

class PythonServ(Service):
	def __init__(self, service):
		self.cservice = service
		self.register(
			("HELP", 0, 2, 0, 0, 0, 0),
		)

	def HELP(self, client, parv):
		print self
		print client
		print parv
		if len(parv) == 1:
			self.do_help(client, "", parv)
		else:
			self.do_help(client, parv[1], parv)
