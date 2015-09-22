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
    AVStream*(*make_video)(AVFormatContext *, AVCodecContext *, char*),
    AVStream*(*make_audio)(AVFormatContext *, AVCodecContext *, char*),
    InputSource * source,
    int video,
    int audio
);

Output * sff_open_output(
    char * outfile,
    void * make_video,
    void * make_audio,
    InputSource * source,
    int video,
    int audio
);


typedef struct TranscodingContext TranscodingContext;

typedef int (*TranscodingFunc)(InputSource *, Output *, TranscodingContext *);

struct TranscodingContext{

    struct SwrContext * swr_ctx;
    struct SwsContext * sws_ctx;

    AVPacket curr_packet;
    AVPacket video_packet;
    AVPacket copy_current_packet;
    AVPacket audio_packet;

    AVFrame * ivframe;
    AVFrame * ovframe;
    AVFrame * iaframe;
    AVFrame * oaframe;

    int dest_pict_buffer_size;
    uint8_t * dest_pict_buffer;
    void * internal_ptr;

    TranscodingFunc * before_decode_video;
    TranscodingFunc * after_decode_video;
    TranscodingFunc * after_convert_video;
    TranscodingFunc * before_encode_video;
    TranscodingFunc * after_encode_video;

    TranscodingFunc * before_decode_audio;
    TranscodingFunc * after_decode_audio;
    TranscodingFunc * after_convert_audio;
    TranscodingFunc * before_encode_audio;
    TranscodingFunc * after_encode_audio;

    int first_vpts;
    int first_apts;

};

TranscodingContext * build_transcoding_context(InputSource * source, Output * output);

#endif __GENERAL_H__

//CLOSING HANDLERS

int close_source(InputSource *);

int close_output(Output *);

int close_transcoding_context(TranscodingContext *);


//INITIALIZATION FFMPEG HANDLERS

void sff_register_all(void);

int  sff_network_init(void);

//SOME OTHER UTILS

void sff_dump_format(void *, char*);

//FILE FUNCTIONS

int sff_write_header(Output *);

int sff_write_trailer(Output *);