// Register a callback (called from Python code)
// callbackFunc is a Python callable accepting one argument

%{
    #include "callbacks.h"
%}

%nothread registerHandler;
void registerHandler(PyObject *callbackFunc, TranscodingFunc * func_ptr);
