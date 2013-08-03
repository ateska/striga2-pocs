'''
This module demonstrates how to hook into Python import to enable .stpy modules to be loaded
'''

import sys, importlib.machinery

# Conditionally import 'enhanced' SourceFileLoader to overcome Python 3.3.2 limitations
if hasattr(importlib.machinery.SourceFileLoader, 'source_to_code'):
	SourceFileLoader = importlib.machinery.SourceFileLoader
else:
	from _srcfileldr import FutureSourceFileLoader as SourceFileLoader
###

STRIGA_SUFFIXES = ('.stpy',)

###

class StrigaFileLoader(SourceFileLoader):

	def source_to_code(self, data, path):
		if path.lower().endswith(STRIGA_SUFFIXES):
			print("HEY!", path)
			raise NotImplementedError('Striga compiler is not implemented yet.')
			# If 'path' file extension is Striga ('.stpy') then Striga compiler needs to be invoked instead of Python default one
		return super().source_to_code(data, path)

###

# Replace original FileFinder with one that can also recognize import .stpy files
sys.path_hooks = list(filter(lambda x:getattr(x,'__name__',None) != 'path_hook_for_FileFinder', sys.path_hooks))
loader = (StrigaFileLoader, importlib.machinery.SOURCE_SUFFIXES + ['.stpy'])
sys.path_hooks.append(importlib.machinery.FileFinder.path_hook(loader))

# Flush import cache to force re-evaluation of loaders (per path)
sys.path_importer_cache.clear()
