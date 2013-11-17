#!/usr/bin/env python3

from distutils.core import setup, Extension

setup(
	name='PyGLEV',
	version='1.0',
	description='Greenlet Event Loop IO library for Python3 (fast asynchronous non-threaded networking)',
	author='Ales Teska',
	author_email='ales.teska+striga@gmail.com',
	url='https://github.com/ateska/striga2-pocs',
	packages=['pyglev'],
	ext_modules=[Extension('pyglev.core', ['pyglev/core/core.c'])],
)
