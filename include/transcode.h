#ifndef __TRANSCODE__H_
#define __TRANSCODE__H_

#include <libavformat/avformat.h>

#include "general.h"

typedef int (*ProcessHandler)(
    TranscodingFunc * func_ptr,
    TranscodingContext * tctx,
    InputSource * source,
    Output * output,
    AVPacket *pkt1_for_free,
    AVPacket *pkt2_for_free,
    AVPacket *pkt3_for_free
);

int set_process_handler(
    ProcessHandler func_ptr
);

int native_process_handler(
    TranscodingFunc * func,
    TranscodingContext * tctx,
    InputSource * source,
    Output * output,
    AVPacket *pkt1_for_free,
    AVPacket *pkt2_for_free,
    AVPacket *pkt3_for_free
);

int process_video_packet(
    InputSource * source,
    Output * output,
    TranscodingContext * tctx
);

void process_audio_packet(
    InputSource * source,
    Output * output,
    TranscodingContext * tctx
);

int transcode(
	InputSource * source,
	Output * output,
	TranscodingContext * tctx
);

#endif