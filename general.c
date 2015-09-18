#include <libavformat/avformat.h>

#include "general.h"


void die(char *str) {
    fprintf(stderr, "%s\n", str);
    exit(1);
}


AVFormatContext * open_input_source(char *source) {
    AVFormatContext * result = NULL;
    int err;
    
    // Open video file
    result = avformat_alloc_context();
    err = avformat_open_input(&result, source, NULL, NULL);
    if (err < 0) {
        fprintf(stderr, "ffmpeg: Unable to open input source%s\n", source);
    }

    // Retrieve stream information
    err = avformat_find_stream_info(result, NULL);
    if (err < 0) {
        fprintf(stderr, "ffmpeg: Unable to find stream info\n");
    }

    // Dump information about file onto standard error


    av_dump_format(result, 0, source, 0);

    return result;
}

int get_video_stream(AVFormatContext * fmt_ctx) {
    int video_stream;
    for (video_stream = 0; video_stream < fmt_ctx->nb_streams; ++video_stream){
        if (fmt_ctx->streams[video_stream]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            break;
        }
    }
    if (video_stream == fmt_ctx->nb_streams) {
        fprintf(stderr, "ffmpeg: Unable to find video stream\n");
        video_stream = -1;
    }
    return video_stream;
}

int get_audio_stream(AVFormatContext * fmt_ctx) {
    int audio_stream;
    for (audio_stream = 0; audio_stream < fmt_ctx->nb_streams; ++audio_stream){
        if (fmt_ctx->streams[audio_stream]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            break;
        }
    }
    if (audio_stream == fmt_ctx->nb_streams) {
        fprintf(stderr, "ffmpeg: Unable to find audio stream\n");
        audio_stream = -1;
    }
    return audio_stream;
}

AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                  uint64_t channel_layout,
                                  int sample_rate, int nb_samples, int channels)
{
    AVFrame *frame = av_frame_alloc();
    int ret;
    if (!frame) {
        fprintf(stderr, "Error allocating an audio frame\n");
        exit(1);
    }
    frame->format = sample_fmt; 
    frame->channel_layout = channel_layout;
    frame->channels = channels;
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;
    if (nb_samples) {
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            fprintf(stderr, "Error allocating an audio buffer\n");
            exit(1);
        }
    }
    return frame;
}

struct SwrContext * build_audio_swr(AVCodecContext * in_ctx, AVCodecContext * out_ctx){
    struct SwrContext * swr_ctx = NULL;
    if ((swr_ctx = swr_alloc()) == NULL)
        die("Cannot allocate audio swr context\n");


    av_opt_set_int       (swr_ctx, "in_channel_count",   in_ctx->channels,          0);
    av_opt_set_int       (swr_ctx, "in_sample_rate",     in_ctx->sample_rate,       0);
    av_opt_set_int       (swr_ctx, "in_ch_layout",       in_ctx->channel_layout,    0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt",      in_ctx->sample_fmt,        0);
    av_opt_set_int       (swr_ctx, "out_channel_count",  out_ctx->channels,         0);
    av_opt_set_int       (swr_ctx, "out_sample_rate",    out_ctx->sample_rate,      0);
    av_opt_set_int       (swr_ctx, "out_ch_layout",      out_ctx->channel_layout,   0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt",     out_ctx->sample_fmt,       0);
    return swr_ctx;
}

InputSource * open_source(char * url, int video, int audio){
    AVFormatContext * ctx = NULL;
    InputSource * source = av_malloc(sizeof(InputSource));
    if (source == NULL) die("Cannot alloc source\n");
    memset((void*)source, 0, sizeof(InputSource));

    source->video = -1;
    source->audio = -1;

    source->url = av_malloc(strlen(url)+1);
    strcpy(source->url, url);

    source->ctx = open_input_source(source->url);
    ctx = source->ctx;

    if (video != 0){
        source->video = get_video_stream(ctx);
        source->video_st = ctx->streams[source->video];
        source->vctx = source->video_st->codec;
        source->vc = avcodec_find_decoder(source->vctx->codec_id);
        if (source->vc == NULL) die("Cannot find video decoder\n");
        if (avcodec_open2(source->vctx, source->vc, NULL) < 0) die("Cannot open video decoder\n");
    }
    if (audio != 0){
        source->audio = get_audio_stream(ctx);
        source->audio_st = ctx->streams[source->audio];
        source->actx = source->audio_st->codec;
        source->ac = avcodec_find_decoder(source->actx->codec_id);
        if (source->ac == NULL) die("Cannot find audio decoder\n");
        if (avcodec_open2(source->actx, source->ac, NULL) < 0) die("Cannot open audio decoder\n");
    }
    return source;
}


AVStream * general_make_video(
    AVFormatContext *ctx,
    AVCodecContext * cctx,
    char * outfile
){
    AVOutputFormat * fmt = NULL;;
    AVStream * stream = NULL;
    AVCodec * codec = NULL;
    AVCodecContext * codec_ctx = NULL;
    int vcodec_id;

    fmt = ctx->oformat;

    vcodec_id = av_guess_codec(fmt, NULL, outfile, NULL, AVMEDIA_TYPE_VIDEO);
    codec = avcodec_find_encoder(vcodec_id);
    if (codec == NULL) die("Cannot find encoder for video codec\n");
    stream = avformat_new_stream(ctx, codec);
    if (stream == NULL) die("Cannot add video steam to output file\n");

    codec_ctx = stream->codec;

    codec_ctx->codec_id = vcodec_id;
    codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_ctx->bit_rate = 512*1014;
    codec_ctx->width = 800;
    codec_ctx->height = 600;
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_ctx->time_base = (AVRational){1, 25};

    if (avcodec_open2(codec_ctx, codec, NULL) < 0) die("Cannot open video codec\n");

    return stream;
}


AVStream * general_make_audio(
    AVFormatContext *ctx,
    AVCodecContext *cctx,
    char * outfile
){
    AVOutputFormat * fmt = NULL;
    AVCodec * codec = NULL;
    AVCodecContext * codec_ctx = NULL;
    AVStream * stream = NULL;

    int acodec_id;

    fmt = ctx->oformat;

    acodec_id = av_guess_codec(fmt, NULL, outfile, NULL, AVMEDIA_TYPE_AUDIO);

    codec = avcodec_find_encoder(acodec_id);
    if (codec == NULL) die ("Cannot find encoder for video codec\n");

    stream = avformat_new_stream(ctx, codec);
    if (stream == NULL) die("Cannot add audio stream to output file\n");

    codec_ctx = stream->codec;

    codec_ctx->sample_fmt = AV_SAMPLE_FMT_S32;
    codec_ctx->codec_type = AVMEDIA_TYPE_AUDIO;
    codec_ctx->codec_id = acodec_id;
    codec_ctx->bit_rate = 64000;
    codec_ctx->sample_rate = 48000;
    codec_ctx->channels = 2;

    if (avcodec_open2(codec_ctx, codec, NULL) < 0) die("Cannot open audio encoder\n");

    return stream;
}


void duplicate_video_context_params(
    AVCodecContext *out,
    AVCodecContext *in
){
    out->codec_type = in->codec_type;
    out->bit_rate = in->bit_rate;
    out->width = in->width;
    out->height = in->height;
    out->time_base = in->time_base;
    out->gop_size = in->gop_size;
    out->pix_fmt = in->pix_fmt;

    if (out->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
        /* just for testing, we also add B frames */
        out->max_b_frames = in->max_b_frames;
    }
    if (out->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
        /* Needed to avoid using macroblocks in which some coeffs overflow.
         * This does not happen with normal video, it just happens here as
         * the motion of the chroma plane does not match the luma plane. */
        out->mb_decision = 2;
    }
}

void duplicate_audio_context_params(
    AVCodecContext *out,
    AVCodecContext *in
){
    out->sample_fmt = in->sample_fmt;
    out->codec_type = in->codec_type;
    out->bit_rate = in->bit_rate;
    out->sample_rate = in->sample_rate;
    out->channels = in->channels;
    out->channel_layout = in->channel_layout;
    out->time_base = (AVRational){1, out->sample_rate};
}


Output * open_output(
    char * outfile,
    AVStream*(*make_video)(AVFormatContext *, AVCodecContext *, char *),
    AVStream*(*make_audio)(AVFormatContext *, AVCodecContext *, char *),
    InputSource * source,
    int video,
    int audio
){
    Output * output = av_malloc(sizeof(Output));
    if (output == NULL) die("Cannot alloc output\n");
    memset((void*)output, 0, sizeof(Output));

    output->video = -1;
    output->audio = -1;

    output->url = av_malloc(strlen(outfile)+1);
    strcpy(output->url, outfile);

    avformat_alloc_output_context2(&output->ctx, NULL, NULL, output->url);
    if (output->ctx == NULL) die("Cannot allocate output format context\n");


    //Preparing output video stream
    if (video == 1){
        if (make_video == NULL){
            //Building codec and it's context
            if (source == NULL) {
                //Generating default params
                output->video_st = general_make_video(output->ctx, NULL, NULL); 
                output->vctx = output->video_st->codec;
            } else {
                //Duplicating params
                AVCodec * codec = NULL;;
                AVStream * stream = NULL;

                codec = avcodec_find_encoder(source->vctx->codec_id);
                if (codec == NULL) die("Cannot find encoder for video\n");

                stream = avformat_new_stream(output->ctx, codec);
                if (stream == NULL) die("Cannot append output video stream\n");
                output->video_st = stream;

                output->vctx = stream->codec;
                output->vctx->codec_id = stream->codec->codec_id;
                duplicate_video_context_params(output->vctx, source->vctx);

                if (avcodec_open2(output->vctx, output->vc, NULL) < 0)
                    die("Cannot open video encoder\n");
            }

        } else {
            output->video_st = make_video(output->ctx, source, output->url);
            output->vctx = output->video_st->codec;
        }
        output->vc = output->vctx->codec;
        output->video = get_video_stream(output->ctx);
    }

    //Preparing output audio stream
    if (audio == 1){
        if (make_audio == NULL){
            //Building codec and it's context
            if (source == NULL) {
                //Generating with default format params
                output->audio_st = general_make_audio(output->ctx, NULL, NULL);
                output->actx = output->audio_st->codec;
            } else {
                //Duplicating params
                AVCodec * codec = NULL;
                AVStream * stream = NULL;

                codec = avcodec_find_encoder(source->actx->codec_id);
                if (codec == NULL) die("Cannot find encoder for audio\n");

                stream = avformat_new_stream(output->ctx, codec);
                if (stream == NULL) die("Cannot append output audio stream\n");

                output->actx = stream->codec;
                output->actx->codec_id = stream->codec->codec_id;
                duplicate_video_context_params(output->actx, source->actx);

                if (avcodec_open2(output->actx, output->ac, NULL) < 0)
                    die("Cannot open audio encoder\n");
            }
        } else {
            //Making with make_audio helper
            output->audio_st = make_audio(output->ctx, source, output->url);
            output->actx = output->audio_st->codec;
        }
        output->ac = output->actx->codec;
        output->audio = get_audio_stream(output->ctx);
    }

    if (!(output->ctx->flags & AVFMT_NOFILE)) {
        if (avio_open(&output->ctx->pb, output->url, AVIO_FLAG_WRITE) < 0)
            die("Cannot open output file\n");
    }    

    return output;
}
