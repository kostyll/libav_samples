#ifndef __CSHARP_CALLBACKS_H_
#define __CSHARP_CALLBACKS_H_

#include "general.h"
#include "transcode.h"

typedef int bool;

TranscodingFunc registerHandler(
	CallBackFuncObject * callbackFunc, TranscodingFunc * func_ptr
){
	const bool hasCallback = 
        callbackFunc != 0 && callbackFunc != NULL;

    if (!hasCallback) return NULL;

    if (func_ptr != NULL && func_ptr != 0 && func_ptr != Py_None){
        //
    } else {
        func_ptr = (TranscodingFunc)malloc(sizeof(int));
        *(int *)func_ptr = EXTERNAL_CALLBACK;
    }

    if (register_callback(callbackFunc, func_ptr) == NULL)
        die("Registration of callback failed");

    return func_ptr;
}

int processCallback(
	CallBackFuncObject * callbackFunc,
	TranscodingContext * tctx,
	InputSource * source,
	Output * output
){
    int result = 1;
	if (callbackFunc){
        result = (int)(*callbackFunc)(tctx, source, output);
    }

    return result;
}

int csharp_process_handler(
    TranscodingFunc func_ptr,
    TranscodingContext * tctx,
    InputSource * source,
    Output * output,
    AVPacket *pkt1_for_free,
    AVPacket *pkt2_for_free,
    AVPacket *pkt3_for_free
){
	CallBackFuncObject * func_obj = NULL;
	//...

    return 1;
}

int init_processCallback(void)
{
    return set_process_handler(csharp_process_handler);
}

#endif