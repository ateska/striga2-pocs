import sys
from . import core

###

class application(object):

	def __init__(self):
		self.exit_code = 0
		self.event_loop = core.event_loop()


	def run(self):
		ret = self.event_loop.run()
		print(">>run ret", ret)

		print("Application exit (exit code={})".format(self.exit_code))
		sys.exit(self.exit_code)
