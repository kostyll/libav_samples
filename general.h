#ifndef __GENERAL_H__
#define __GENERAL_H__

#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>

void die(char *str);

typedef struct {
    AVStream * video_st;
    AVStream * audio_st;

    int video;
    int audio;

    char * url;

    AVCodec * vc;
    AVCodec * ac;

    AVCodecContext * vctx;
    AVCodecContext * actx;

    AVFormatContext * ctx;

} SourceOrDestinatio;

typedef SourceOrDestinatio InputSource;
typedef SourceOrDestinatio Output;

AVFormatContext * open_input_source(char *source);

int get_video_stream(AVFormatContext * fmt_ctx);

int get_audio_stream(AVFormatContext * fmt_ctx);

AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                  uint64_t channel_layout,
                                  int sample_rate, int nb_samples, int channels);

struct SwrContext * build_audio_swr(AVCodecContext * in_ctx, AVCodecContext * out_ctx);

InputSource * open_source(char * url, int video, int audio);

Output * open_output(
    char * outfile,
    AVCodecContext*(*make_video)(AVFormatContext *, AVCodecContext *, char*),
    AVCodecContext*(*make_audio)(AVFormatContext *, AVCodecContext *, char*),
    InputSource * source,
    int video,
    int audio
);

#endif __GENERAL_H__
