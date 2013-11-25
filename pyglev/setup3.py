#!/usr/bin/env python3

from distutils.core import setup, Extension

pyglev_core = Extension('pyglev.core',
	libraries = ['ev'],
	sources = [
		'pyglev/core/core.c',
		'pyglev/core/evloop.c',
		'pyglev/core/cmdq.c',
		'pyglev/core/lstncmd.c',
		'pyglev/core/estsock.c',
	]
)

setup(
	name='PyGLEV',
	version='1.0',
	description='Greenlet Event Loop IO library for Python3 (fast asynchronous non-threaded networking)',
	author='Ales Teska',
	author_email='ales.teska+striga@gmail.com',
	url='https://github.com/ateska/striga2-pocs',
	packages=['pyglev'],
	ext_modules=[pyglev_core],
)
