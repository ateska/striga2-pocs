# GreenEv

This Proof-Of-Concept is incomplete - and will likely stay as it is.
It already proved that:

1. Application & Event loop in C, Python on top of that is correct concept
2. Threads only makes things complicated - with no tangible benefits

## Conclusion

Another PoC should explore single thread (no thread) libev-based C application integrated with Python.

See:

-  singlev (embed Python interpretter with libev - aborted Poc)
-  pyglev (Python 3 module that integrates libev event loop with greenlets)
