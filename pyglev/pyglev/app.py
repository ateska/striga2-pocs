import sys
from . import core

###

class application(object):

	# Events
	on_init = None # def on_init(self):
	on_exit = None # def on_exit(self):

	#TODO: Following callback
	on_break = None # def on_break(self):


	def __init__(self):
		self.exit_code = 0
		self.event_loop = core.event_loop()
		self.event_loop.on_error = self.on_error

		if (self.on_init is not None): self.on_init()


	def run(self):
		self.event_loop.run()

		if (self.on_exit is not None): self.on_exit()

		print("Application exit (exit code={})".format(self.exit_code))
		sys.exit(self.exit_code)


	def listen(self, host, port, backlog=10):
		cmd = core.listen_cmd(host, str(port), backlog=backlog)
		self.event_loop._xschedule(cmd)


	def on_error(self, subject, error_type, error_code, error_msg):
		print("Error callback called:", subject, error_type, error_code, error_msg)
