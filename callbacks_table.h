#ifndef __CALLBACKS_TABLE_H_
#define __CALLBACKS_TABLE_H_

#include <stdlib.h>

#include <Python.h>

#include "general.h"
#include "transcode.h"

#define TRANSCODING_TABLE_SIZE 100

typedef struct TranscodingFuncItem TranscodingFuncItem;

struct TranscodingFuncItem{
	PyObject * py_func;
	TranscodingFunc func_ptr;
};

TranscodingFuncItem * table[TRANSCODING_TABLE_SIZE];
int transcoding_count = 0;

TranscodingFuncItem * allocate_transcoding_func_item(){
	struct TranscodingFuncItem * result = NULL;
	if ((result = (TranscodingFuncItem*)malloc(sizeof(TranscodingFuncItem))) == NULL)
		die("Cannot allocate transcoding func item");
	return result;
}

TranscodingFuncItem * register_callback(
	PyObject * py_func,
	TranscodingFunc func_ptr
){
	if (transcoding_count >= TRANSCODING_TABLE_SIZE)
		die("Transcoding table is full");
	struct TranscodingFuncItem * item = NULL;
	item = get_callback(func_ptr);
	if (item == NULL) {
		item = allocate_transcoding_func_item();
		item->py_func = py_func;
		item->func_ptr = func_ptr;
		table[transcoding_count++] = item;
	}
	return item;
};

TranscodingFuncItem * get_callback(TranscodingFunc func_ptr){
	int i;
	for (i = 0; i< TRANSCODING_TABLE_SIZE; i++)
		if (table[i]->func_ptr == func_ptr)
			return table[i];
	return NULL;
}

#endif
