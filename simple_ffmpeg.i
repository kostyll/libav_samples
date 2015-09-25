%module simple_ffmpeg

#ifdef SWIGPYTHON

	typedef PyObject CallBackFuncObject;

#endif

#ifdef SWIGCSHARP

    typedef void (* CallBackFuncObject)(int);
    #define DLL __declspec(dllexport)

#endif

%{
 #include "general.h"
 #include "transcode.h"
 #include "callbacks_table.h"
%}

%include "callbacks.i"

%include "include/general.h"
%include "include/transcode.h"
