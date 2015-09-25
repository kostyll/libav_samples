%module simple_ffmpeg

#ifdef SWIGPYTHON

	typedef PyObject CallBackFuncObject;

#endif

#ifdef SWIGCSHARP

    typedef int (* CallBackFuncObject)(TranscodingContext *, InputSource *, Output *);

#endif

%{
 #include "general.h"
 #include "transcode.h"
 #include "callbacks_table.h"
%}

%include "callbacks.i"

%include "include/general.h"
%include "include/transcode.h"
