#ifndef __GENERAL_H__
#define __GENERAL_H__

#include <libavformat/avformat.h>

void die(char *str);

AVFormatContext * open_input_source(char *source);

int get_video_stream(AVFormatContext * fmt_ctx);

int get_audio_stream(AVFormatContext * fmt_ctx);

static AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                  uint64_t channel_layout,
                                  int sample_rate, int nb_samples, int channels);

#endif __GENERAL_H__
