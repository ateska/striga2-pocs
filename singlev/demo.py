import pysev

#

class inbound(object):

	def on_start(self):
		print("Yeah !")

	def on_error(self):
		pass

#

class demo_app(pysev.application):

	def on_init(self):
		self.listen(inbound, '*', 7777)

	def on_error(self):
		pass

#

if __name__ == '__main__':
	app = demo_app()
	app.run()
