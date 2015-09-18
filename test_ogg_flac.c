#include <stdio.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/frame.h>

#include "general.h"

AVStream * add_video_output(AVFormatContext * fmt_ctx, AVCodecContext * cctx, char *outfile){

    AVStream * stream = NULL;

    AVCodec * vcodec = NULL;
    AVCodecContext * vcodec_ctx = NULL;

    vcodec = avcodec_find_encoder(AV_CODEC_ID_THEORA);
    if (vcodec == NULL) die("Cannot find video encoder THEORA\n");

    stream = avformat_new_stream(fmt_ctx, vcodec);

    vcodec_ctx = stream->codec;

    vcodec_ctx->codec_id = AV_CODEC_ID_THEORA;
    vcodec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    vcodec_ctx->bit_rate = 1000000;
    vcodec_ctx->width = 800;
    vcodec_ctx->height = 600;
    vcodec_ctx->time_base = (AVRational){1, 25};
    vcodec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

    if (avcodec_open2(vcodec_ctx, vcodec, NULL) < 0) die("Cannot open THEORA encoder\n");

    return stream;
}

AVStream * add_audio_output(AVFormatContext * fmt_ctx, AVCodecContext * cctx, char *outfile){
    AVStream * stream = NULL;

    AVCodec * acodec = NULL;
    AVCodecContext * acodec_ctx = NULL;

    acodec = avcodec_find_encoder(AV_CODEC_ID_FLAC);
    if (acodec == NULL) die("Cannot find video encoder FLAC\n");

    stream = avformat_new_stream(fmt_ctx, acodec);
    acodec_ctx = stream->codec;

    acodec_ctx->sample_fmt = AV_SAMPLE_FMT_S32;
    acodec_ctx->codec_type = AVMEDIA_TYPE_AUDIO;
    acodec_ctx->codec_id = AV_CODEC_ID_FLAC;
    acodec_ctx->bit_rate = 64000;
    acodec_ctx->sample_rate = 48000;
    acodec_ctx->channels = 2;

    if (avcodec_open2(acodec_ctx, acodec, NULL) < 0) die("Cannot open FLAC encoder\n");

    return stream;
}

int process_video_packet(InputSource * source, Output * output, AVPacket * packet){
    int frame_finished;
    AVFrame frame;

    avcodec_decode_video2(source->vctx, &frame, &frame_finished, packet);
    frame.pts = av_rescale_q(packet->pts, source->video_st->time_base, source->vctx->time_base);
    if (frame_finished){
        fprintf(stdout, "VFrame finished\n");
        return 0;
    }
}


int main(int argc, char ** argv){
    if (argc != 3){
        fprintf(stderr, "Usage %s <infile> <outfile>\n", argv[0]);
        return 0;
    }
    char * outfile = NULL;
    outfile = argv[2];

    // Initialization
    av_register_all();
    avformat_network_init();

    //Opening input source
    InputSource * source = NULL;
    source = open_source(argv[1], 1, 1);


    //Creating output
    Output * output;
    output = open_output(
        outfile,
        add_video_output,
        add_audio_output,
        NULL,
        1,
        1
    );

    av_dump_format(output->ctx, 0, outfile, 1);

    avformat_write_header(output->ctx, NULL);

    struct SwrContext * swr_ctx = NULL;
    swr_ctx = build_audio_swr(source->actx, output->actx);


    AVPacket packet;
    AVPacket packet_copy;
    AVPacket target_packet;

    AVFrame* frame = NULL;
    AVFrame* aframe = NULL;
    AVFrame* daframe = NULL;

    int nb_samples;

    frame = av_frame_alloc();
    if (frame == NULL) die("Cannot allocate frame\n");

    aframe = alloc_audio_frame(
        source->actx->sample_fmt,
        source->actx->channel_layout,
        source->actx->sample_rate,
        10000,
        source->actx->channels
        );
    if (aframe == NULL) die("Cannot allocate aframe\n");

    daframe = alloc_audio_frame(
        output->actx->sample_fmt,
        output->actx->channel_layout,
        output->actx->sample_rate,
        output->ac->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE ? 10000 : output->actx->frame_size,
        output->actx->channels
        );
    if (daframe == NULL) die("Cannot allocate daframe\n");

    daframe->nb_samples = output->actx->frame_size;
    daframe->format = output->actx->sample_fmt;
    daframe->channel_layout = output->actx->channel_layout;
    daframe->channels = output->actx->channels;
    daframe->sample_rate = output->actx->sample_rate;

    int last_vpts = -1;
    int last_apts = -1;
    int ret;

    while(av_read_frame(source->ctx, &packet) >= 0){
        fprintf(stdout, "packet.pts = %d\n", packet.pts);
        if (packet.stream_index == source->video){
            ret = process_video_packet(source, output, &packet);
            if (ret > 0) av_free_packet(&packet);
        }
    }

    av_write_trailer(output->ctx);

    av_free(frame);
    av_free(aframe);
    av_free(daframe);
    avformat_close_input(&source->ctx);
    avformat_close_input(&output->ctx);

    swr_close(swr_ctx);
    swr_free(&swr_ctx);

    return 0;
}
