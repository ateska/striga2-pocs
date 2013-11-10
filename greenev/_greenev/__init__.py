try:
	import _pygeapi
except ImportError as e:
	e.msg += ' (this module can be imported only from greenev)'
	raise
