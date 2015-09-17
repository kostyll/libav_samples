#ifndef __GENERAL_H__
#define __GENERAL_H__

#include <libavformat/avformat.h>

AVFormatContext * open_input_source(char *source);

int get_video_stream(AVFormatContext * fmt_ctx);

int get_audio_stream(AVFormatContext * fmt_ctx);

#endif __GENERAL_H__
