#ifndef __TRANSCODE__H_
#define __TRANSCODE__H_

#include <libavformat/avformat.h>

#include "general.h"

int process_video_packet(
    InputSource * source,
    Output * output,
    TranscodingContext * tctx
);

int process_audio_packet(
    InputSource * source,
    Output * output,
    TranscodingContext * tctx
);

int transcode(
	InputSource * source,
	Output * output,
	TranscodingContext * tctx
);

#endif __TRANSCODE__H_