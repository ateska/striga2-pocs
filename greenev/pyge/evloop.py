import sys
import _pygeapi

class event_loop(object):

	def __init__(self):
		pass

	def run(self):
		pass

	def listen(self, hostname, port, backlog=10):
		_pygeapi.io_thread_listen(sys.pygeapi_py_thread, hostname, str(port), backlog)
