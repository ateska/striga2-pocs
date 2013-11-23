import sys
from . import core

###

def _on_error(subject, error_type, error_code, error_msg):
	print("Error callback called ({} {}) on".format(error_type, error_code), subject, error_msg)


class application(object):

	# Events
	on_prepare = None # def on_prepare(self):
	on_exit = None # def on_exit(self):

	#TODO: Following callback
	on_break = None # def on_break(self):


	def __init__(self):
		self.exit_code = 0
		self.event_loop = core.event_loop()
		self.event_loop.on_error = _on_error

		if (self.on_prepare is not None): self.on_prepare()


	def run(self):
		try:
			self.event_loop.run()

		finally:
			if (self.on_exit is not None): self.on_exit()

		print("Application exit (exit code={})".format(self.exit_code))
		sys.exit(self.exit_code)


	def listen(self, host, port, protocol, backlog=10):
		cmd = core.listen_cmd(host, str(port), protocol, backlog=backlog)
		self.event_loop._xschedule(cmd)
		return cmd
