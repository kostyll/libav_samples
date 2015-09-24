// Register a callback (called from Python code)
// callbackFunc is a Python callable accepting one argument

%nothread registerHandler;

#ifdef SWIGPYTHON

	%{
	    #include "python_callbacks.h"
	%}

	void registerHandler(PyObject *callbackFunc, TranscodingFunc * func_ptr);
#endif

#ifdef SWIGCSHARP

	%{
		#include "csharp_callbacks.h"
	%}

	registerHandler(void *callbackFunc, TranscodingFunc * func_ptr);
#endif

%nothread init_processCallback;
int init_processCallback(
    void
);
