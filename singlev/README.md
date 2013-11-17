# SinglEv

PoC to explore libev-based event loop together with Greenlets in Python.

Whole core is meant to run in single thread but support multi-threaded Python client applications.


## Findings

* DNS queries should be asynchronous
* Error reporting needs to be asynchronous, e.g. application.on_error() callback
