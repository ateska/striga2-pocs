#!/usr/bin/env python3
import pyglev

def line_protocol(listen_cmd):
	print("Protocol established")
	
	data = yield pyglev.READ

	print("1) Protocol reveiced: ", data)

	data = yield pyglev.READ
	
	print("2) Protocol reveiced: ", data)

###

class demo_app(pyglev.application):

	def on_prepare(self):
		self.listen('localhost', 7777, line_protocol)


if __name__ == '__main__':
	app = demo_app()
	app.run()
