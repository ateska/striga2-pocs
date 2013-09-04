This PoC demonstrates that Python can:
1) load file
2) change (trans-compile) it
3) parse into AST using standard Python 'compile()' function
4) modify/fix AST lines and columns attributes to refer to original file
5) compile AST into code object
6) use that as a module; which refers traceback correctly into positions in original file

Currently, PoC is limited to line numbers transformation - but it can be easily extended to columns
