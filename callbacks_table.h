#ifndef __CALLBACKS_TABLE_H_
#define __CALLBACKS_TABLE_H_

#include <stdlib.h>

#include "general.h"
#include "transcode.h"

#define TRANSCODING_TABLE_SIZE 100
typedef CallBackFuncObject;

#define INTERNAL_CALLBACK 0x00
#define EXTERNAL_CALLBACK 0x01

typedef struct TranscodingFuncItem{
	CallBackFuncObject * func_obj;
	TranscodingFunc func_ptr;
} TranscodingFuncItem;

struct TranscodingFuncItem * table[TRANSCODING_TABLE_SIZE];
int transcoding_count = 0;

struct TranscodingFuncItem * allocate_transcoding_func_item(void){
	struct TranscodingFuncItem * result = NULL;
	if ((result = (TranscodingFuncItem*)malloc(sizeof(TranscodingFuncItem))) == NULL)
		die("Cannot allocate transcoding func item");
	return result;
}

struct TranscodingFuncItem * get_callback(TranscodingFunc func_ptr){
	int i;
	for (i = 0; i< transcoding_count; i++){
		if (table[i]->func_ptr == func_ptr) 
			return table[i];
	}
	return NULL;
}

struct TranscodingFuncItem * register_callback(CallBackFuncObject * func_obj, TranscodingFunc func_ptr){
	struct TranscodingFuncItem *item;
	item = NULL;
	if (func_ptr == NULL){
		func_ptr = (TranscodingFunc)malloc(sizeof(int));
		if (func_ptr == NULL) die("Cannot allocate callback key func");
	}
	if (transcoding_count >= TRANSCODING_TABLE_SIZE)
		die("Transcoding table is full");
	item = get_callback(func_ptr);
	if (item == NULL) {
		item = allocate_transcoding_func_item();
		item->func_obj = func_obj;
		item->func_ptr = func_ptr;
		table[transcoding_count++] = item;
	} else item->func_obj = func_obj;
	return item;
}

#endif
