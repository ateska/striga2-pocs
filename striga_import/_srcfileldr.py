import sys, marshal, importlib.machinery
from importlib._bootstrap import cache_from_source, _verbose_message, _code_type, _call_with_frames_removed, _w_long, _MAGIC_BYTES, _imp

class FutureSourceFileLoader(importlib.machinery.SourceFileLoader):
	'''
	This class implements source_to_code() from Python 3.4 which is missing in 3.3.2
	When method source_to_code() is available in Python importlib.machinery.SourceFileLoader class, then this class is not needed anymore.
	'''

	def get_code(self, fullname):
		"""Concrete implementation of InspectLoader.get_code.

		Reading of bytecode requires path_stats to be implemented. To write
		bytecode, set_data must also be implemented.

		"""
		source_path = self.get_filename(fullname)
		source_mtime = None
		try:
			bytecode_path = cache_from_source(source_path)
		except NotImplementedError:
			bytecode_path = None
		else:
			try:
				st = self.path_stats(source_path)
			except NotImplementedError:
				pass
			else:
				source_mtime = int(st['mtime'])
				try:
					data = self.get_data(bytecode_path)
				except IOError:
					pass
				else:
					try:
						bytes_data = self._bytes_from_bytecode(fullname, data,
															   bytecode_path,
															   st)
					except (ImportError, EOFError):
						pass
					else:
						_verbose_message('{} matches {}', bytecode_path,
										source_path)
						found = marshal.loads(bytes_data)
						if isinstance(found, _code_type):
							_imp._fix_co_filename(found, source_path)
							_verbose_message('code object from {}',
											bytecode_path)
							return found
						else:
							msg = "Non-code object in {}"
							raise ImportError(msg.format(bytecode_path),
											  name=fullname, path=bytecode_path)
		source_bytes = self.get_data(source_path)
		code_object = self.source_to_code(source_bytes, source_path)
		_verbose_message('code object from {}', source_path)
		if (not sys.dont_write_bytecode and bytecode_path is not None and
			source_mtime is not None):
			data = bytearray(_MAGIC_BYTES)
			data.extend(_w_long(source_mtime))
			data.extend(_w_long(len(source_bytes)))
			data.extend(marshal.dumps(code_object))
			try:
				self._cache_bytecode(source_path, bytecode_path, data)
				_verbose_message('wrote {!r}', bytecode_path)
			except NotImplementedError:
				pass
		return code_object


	def source_to_code(self, data, path):
		"""Return the code object compiled from source.

		The 'data' argument can be any object type that compile() supports.
		"""
		return _call_with_frames_removed(compile, data, path, 'exec', dont_inherit=True)
