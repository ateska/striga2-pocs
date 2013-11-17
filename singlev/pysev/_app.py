import _pysevapi

class application(object):

	# Callbacks
	def on_init(self): pass


	def __init__(self):
		super(application, self).__init__()
		self.on_init()


	def run(self):
		_pysevapi.application_run()


	def listen(self, greenlet, host, port, backlog=10, concurent=20):
		for i in range(10):
			_pysevapi.application_listen(host, str(port), backlog, concurent)
