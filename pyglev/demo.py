#!/usr/bin/env python3
import pyglev


def line_protocol(listen_cmd):
	print("Line protocol!", listen_cmd)
	yield pyglev.READ


###

class demo_app(pyglev.application):

	def on_prepare(self):
		self.listen('localhost', 7777, line_protocol)


if __name__ == '__main__':
	app = demo_app()
	app.run()
