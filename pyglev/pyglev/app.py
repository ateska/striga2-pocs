import sys
from . import core

###

class application(object):

	# Events
	def on_init(self): pass
	def on_exit(self): pass


	def __init__(self):
		self.exit_code = 0
		self.event_loop = core.event_loop()

		self.on_init()


	def run(self):
		self.event_loop.run()

		self.on_exit()

		print("Application exit (exit code={})".format(self.exit_code))
		sys.exit(self.exit_code)


	def listen(self, host, port, backlog=10):
		print("Method {}.listen() not implemented".format(application.__name__))