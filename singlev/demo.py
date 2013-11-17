import pysev

#

class demo_app(pysev.application):

	def __init__(self):
		super(demo_app, self).__init__()

#

if __name__ == '__main__':
	app = demo_app()
	app.run()
