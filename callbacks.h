#ifndef __CALLBACKS_H_
#define __CALLBACKS_H_

#include <Python.h>

#include "general.h"
#include "transcode.h"

typedef int bool;

void registerHandler(PyObject *callbackFunc, TranscodingFunc * func_ptr)
{
    SWIG_PYTHON_THREAD_BEGIN_ALLOW;
    
    const bool hasCallback = 
        callbackFunc != 0 && callbackFunc != Py_None;
    
    if (register_callback(callbackFunc, func_ptr) == NULL)
        die("Registration of callback failed!");

    SWIG_PYTHON_THREAD_END_ALLOW;
    
    Py_XINCREF(callbackFunc); // to keep callback alive, don't forget to DECREF on unregiser
}

// Calls the callback (called from C code), Param is some SWIG-wrapped type
int processCallback(
    PyObject *callbackFunc,
    TranscodingContext * tctx,
    InputSource * source,
    Output * output
)
{
    SWIG_PYTHON_THREAD_BEGIN_BLOCK;

    PyObject *arglist = PyTuple_New(3);
    PyTuple_SET_ITEM(arglist, 0, 
        SWIG_NewPointerObj(SWIG_as_voidptr(tctx), SWIGTYPE_p_TranscodingContext, 0));
    PyTuple_SET_ITEM(arglist, 1, 
        SWIG_NewPointerObj(SWIG_as_voidptr(source), SWIGTYPE_p_SourceOrDestinatio, 0));
    PyTuple_SET_ITEM(arglist, 2, 
        SWIG_NewPointerObj(SWIG_as_voidptr(output), SWIGTYPE_p_SourceOrDestinatio, 0));
    
    PyObject *result = PyEval_CallObject(callbackFunc, arglist);
    
    Py_DECREF(arglist);
    Py_XDECREF(result);

    SWIG_PYTHON_THREAD_END_BLOCK;

    return 1;
}

int python_process_handler(
    TranscodingFunc func_ptr,
    TranscodingContext * tctx,
    InputSource * source,
    Output * output,
    AVPacket *pkt1_for_free,
    AVPacket *pkt2_for_free,
    AVPacket *pkt3_for_free
){

    PyObject * py_func = NULL;
    TranscodingFuncItem * item = NULL;
    int ret = 1;
    item = get_callback(func_ptr);
    if (item == NULL)
        return 1;
    else {
        py_func = item->py_func;
        if (py_func == NULL) return 1;

        ret = processCallback(
            py_func,
            tctx,
            source,
            output
        );
        if (ret <= 0){
            clean_up_packets(
                pkt1_for_free,
                pkt2_for_free,
                pkt3_for_free
            );
        }
    }
    return ret;
}


int init_processCallback(
    void
){
    return set_process_handler(&python_process_handler);
}

#endif