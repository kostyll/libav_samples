// Register a callback (called from Python code)
// callbackFunc is a Python callable accepting one argument

%nothread registerHandler;

#ifdef SWIGPYTHON

	%{
	    #include "python_callbacks.h"
	%}

	TranscodingFunc registerHandler(PyObject *callbackFunc, TranscodingFunc func_ptr);
	%nothread init_processCallback;

#endif

#ifdef SWIGCSHARP

	%{
		#include "csharp_callbacks.h"
	%}
	// http://www.codeproject.com/Tips/318140/How-to-make-a-callback-to-Csharp-from-C-Cplusplus

	TranscodingFunc registerHandler(CallBackFuncObject *callbackFunc, TranscodingFunc func_ptr);

#endif


int init_processCallback(
    void
);
