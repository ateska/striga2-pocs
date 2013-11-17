try:
	import _pysevapi
except ImportError as e:
	e.msg += ' (this module can be imported only from singlev)'
	raise

from ._app import application
