#ifndef __CSHARP_CALLBACKS_H_
#define __CSHARP_CALLBACKS_H_

#include "general.h"
#include "transcode.h"

typedef int bool;

void registerHandler(
	CallBackFuncObject * callbackFunc, TranscodingFunc * func_ptr
){
	//
}

int processCallback(
	CallBackFuncObject * callbackFunc,
	TranscodingContext * tctx,
	InputSource * source,
	Output * output
){
	//
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
}

int init_processCallback(void)
{
    return set_process_handler(csharp_process_handler);
}

#endif