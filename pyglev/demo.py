#!/usr/bin/env python3

import pyglev


class demo_app(pyglev.application):

	def on_init(self):
		self.listen('localhost', 7777)


if __name__ == '__main__':
	app = demo_app()
	app.run()
