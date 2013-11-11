from pyge  import event_loop

if __name__ == '__main__':
	
	loop = event_loop()
	loop.listen("localhost", 7777, 10)
	loop.run()
