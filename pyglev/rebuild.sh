#!/bin/sh

rm -f ./pyglev/core.so ./pyglev/core/*.o
rm -rf ./pyglev/__pycache__

python3 setup3.py build_ext --inplace

exit $?
