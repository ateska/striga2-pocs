import sys
import _pygeapi

class event_loop(object):


	def __init__(self):
		pass


	def run(self):

		while True:
			cmd = _pygeapi.py_thread_next_cmd(sys.pygeapi_py_thread, 1000)
			if cmd is None: continue

			print(">", sys.pygeapi_py_thread, cmd)
			if cmd == 1: break


	def listen(self, hostname, port, backlog=10):
		_pygeapi.io_thread_listen(sys.pygeapi_py_thread, hostname, str(port), backlog)
