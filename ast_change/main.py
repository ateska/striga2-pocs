#/usr/bin/env python3
import io, os, ast, sys
from importlib._bootstrap import _call_with_frames_removed

###

def get_data(path):
	'''
	Mimic importlib.abc.ResourceLoader.get_data() method.
	'''
	with io.FileIO(path, 'r') as file:
		return file.readlines()

###

class CodeLocationTransformer(ast.NodeTransformer):

	def __init__(self):
		self._linemap = {}
		return super().__init__()
		


	def add_line_mapping(self, linenofrom, linenoto):
		self._linemap[linenofrom] = linenoto

	def generic_visit(self, node):
		if not issubclass(node.__class__, (ast.expr,ast.stmt)):
			return super().generic_visit(node)

		# Use line map to translate line number to original file
		node.lineno = self._linemap[node.lineno]

		return super().generic_visit(node)

###

def main():
	# Load file
	path = os.path.dirname(os.path.realpath(__file__)) + os.path.sep + 'input.stpy'
	origdata = get_data(path)

	# 'Trans-compile' it
	transformer = CodeLocationTransformer()

	data = b''
	outlineno = 1
	for inlineno, line in enumerate(origdata, 1):
		if line == b'\n': continue
		data += line
		transformer.add_line_mapping(outlineno, inlineno)
		outlineno+=1

	# Parse into AST
	astnode = _call_with_frames_removed(compile, data, path, 'exec', flags=ast.PyCF_ONLY_AST, dont_inherit=True)

	# Modify AST code line and columns numbers
	astnode = transformer.visit(astnode)

	# Compile modified AST into code object
	codeobj = _call_with_frames_removed(compile, astnode, path, 'exec', dont_inherit=True)

	# Load as module
	module_dict = {}
	_call_with_frames_removed(exec, codeobj, module_dict)

	# Call test function
	module_dict['test']()


if __name__ == '__main__':
	main()
