#include <stdio.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/frame.h>

#include "general.h"

#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_PIX_FMT PIX_FMT_YUV420P /* default pix_fmt */
#define STREAM_DURATION   5.0

static void die(char *str) {
    fprintf(stderr, "%s\n", str);
    exit(1);
}


static AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
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
int main(int argc, char ** argv) {

    char * infile = NULL;
    char * outfile = NULL;
    int err;
    int ret;
    int i;

    if (argc != 3){
        fprintf(stderr, "Usage %s input_file output_file\n", argv[0]);
        return 0;
    } 

    infile = argv[1];
    outfile = argv[2];

    av_register_all();
    avformat_network_init();
    avcodec_register_all();
    av_log_set_level(AV_LOG_DEBUG);

    AVInputFormat * ifmt = NULL;
    AVFormatContext * ifmt_ctx = NULL;

    AVOutputFormat * ofmt = NULL;
    AVFormatContext * ofmt_ctx = NULL;

    ifmt_ctx = open_input_source(infile);
    ifmt = ifmt_ctx->iformat;

    int video_stream = get_video_stream(ifmt_ctx);
    int audio_stream = get_audio_stream(ifmt_ctx);

    fprintf(stdout, "VideoStream = %d, AudioStream = %d\n", video_stream, audio_stream);

    AVStream *video_st = ifmt_ctx->streams[video_stream];
    AVStream *audio_st = ifmt_ctx->streams[audio_stream];

    AVCodecContext* ivcodec_ctx = ifmt_ctx->streams[video_stream]->codec;
    AVCodec* ivcodec = avcodec_find_decoder(ivcodec_ctx->codec_id);

    AVCodecContext* iacodec_ctx = ifmt_ctx->streams[audio_stream]->codec;
    AVCodec* iacodec = avcodec_find_decoder(iacodec_ctx->codec_id);

    fprintf(stdout, 
        "Vctx->tb.den = %d, Vctx->tb.num = %d Actx->tb.den = %d, Actx->tb.num = %d\nVst->tb.den = %d, Vst->tb.num = %d Ast->tb.den = %d Ast->tb.num = %d\n",
        ivcodec_ctx->time_base.den, ivcodec_ctx->time_base.num, iacodec_ctx->time_base.den, iacodec_ctx->time_base.num,
        video_st->time_base.den, video_st->time_base.num, audio_st->time_base.den, audio_st->time_base.num
    );

    err = avcodec_open2(ivcodec_ctx, ivcodec, NULL);
    if (err < 0) {
        fprintf(stderr, "ffmpeg: Unable to open video codec\n");
        return -1;
    }

    err = avcodec_open2(iacodec_ctx, iacodec, NULL);
    if (err < 0) {
        fprintf(stderr, "ffmpeg: Unable to open audio codec\n");
        return -1;
    }    

    AVFrame* frame = av_frame_alloc();
    AVPacket packet;
    AVPacket packet_copy;
    AVPacket target_packet;

    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, outfile);
    if (!ofmt_ctx) {
        fprintf(stderr, "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        return (-1);
    };
    ofmt = ofmt_ctx->oformat;

    int vcodec_id, acodec_id;

    vcodec_id = av_guess_codec(ofmt, NULL, outfile, NULL, AVMEDIA_TYPE_VIDEO);
    acodec_id = av_guess_codec(ofmt, NULL, outfile, NULL, AVMEDIA_TYPE_AUDIO);

    fprintf(stdout, "VideoCodecID = %d, AudioCodecID = %d\n", vcodec_id, acodec_id);

    AVCodecContext* ovcodec_ctx = NULL;
    AVCodec* ovcodec = NULL;

    AVCodecContext* oacodec_ctx = NULL;
    AVCodec* oacodec = NULL;

    AVStream* ovstream = NULL;
    AVStream* oastream = NULL;

    ovcodec = avcodec_find_encoder(vcodec_id);
    oacodec = avcodec_find_encoder(acodec_id);

    ovstream = avformat_new_stream(ofmt_ctx, ovcodec);
    ovstream->duration = video_st->duration; 
    ovcodec_ctx = ovstream->codec;
    ovcodec_ctx->codec_id = vcodec_id;
    ovcodec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    ovcodec_ctx->bit_rate = ivcodec_ctx->bit_rate*4;
    /* Resolution must be a multiple of two. */
    ovcodec_ctx->width    = ivcodec_ctx->width;
    ovcodec_ctx->height   = ivcodec_ctx->height;
    /* timebase: This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identical to 1. */
    ovcodec_ctx->time_base.den = ivcodec_ctx->time_base.den;
    ovcodec_ctx->time_base.num = ivcodec_ctx->time_base.num;
    ovcodec_ctx->gop_size      = ivcodec_ctx->gop_size; /* emit one intra frame every twelve frames at most */
    ovcodec_ctx->pix_fmt       = ivcodec_ctx->pix_fmt;
    if (ovcodec_ctx->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
        /* just for testing, we also add B frames */
        ovcodec_ctx->max_b_frames = ivcodec_ctx->max_b_frames;
    }
    if (ovcodec_ctx->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
        /* Needed to avoid using macroblocks in which some coeffs overflow.
         * This does not happen with normal video, it just happens here as
         * the motion of the chroma plane does not match the luma plane. */
        ovcodec_ctx->mb_decision = 2;
    }

    oastream = avformat_new_stream(ofmt_ctx, oacodec);
    oacodec_ctx = oastream->codec;

    oacodec_ctx->sample_fmt  = iacodec_ctx->sample_fmt;
    oacodec_ctx->codec_type = AVMEDIA_TYPE_AUDIO;
    oacodec_ctx->bit_rate    = iacodec_ctx->bit_rate;
    oacodec_ctx->sample_rate = iacodec_ctx->sample_rate;
    oacodec_ctx->channels    = iacodec_ctx->channels;
    oacodec_ctx->channel_layout = iacodec_ctx->channel_layout;
    // oacodec_ctx->time_base = iacodec_ctx->time_base;
    oacodec_ctx->time_base = (AVRational){ 1, oacodec_ctx->sample_rate } ;
    // oacodec_ctx->time_base.den = iacodec_ctx->time_base.den;
    // oacodec_ctx->time_base.num = iacodec_ctx->time_base.num;

    if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        fprintf(stdout, "ASASASASASA\n");
        ovstream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
        oastream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }

    ofmt_ctx->oformat->flags |= AVFMT_NOTIMESTAMPS;

    err = avcodec_open2(ovcodec_ctx, ovcodec, NULL);
    if (err < 0) {
        fprintf(stderr, "ffmpeg: Unable to open video codec\n");
        return -1;
    }

    AVDictionary* opts = NULL;
    av_dict_set(&opts, "strict", "experimental", 2);
    err = avcodec_open2(oacodec_ctx, oacodec, &opts);
    if (err < 0) {
        fprintf(stderr, "ffmpeg: Unable to open audio codec\n");
        return -1;
    }        

    int out_video_stream = get_video_stream(ofmt_ctx);
    int out_audio_stream = get_audio_stream(ofmt_ctx);

    AVStream * ovideo_st = NULL;
    AVStream * oaudio_st = NULL;
    AVFrame* aframe = NULL;
    AVFrame* daframe = NULL;

    ovideo_st = ofmt_ctx->streams[out_video_stream];
    oaudio_st = ofmt_ctx->streams[out_audio_stream];

    // ovideo_st->time_base = ovcodec_ctx->time_base;
    // oaudio_st->time_base = oacodec_ctx->time_base;

    fprintf(stdout, "ovstream_ctx = %p\n", ovstream->codec);
    fprintf(stdout, "oastream_ctx = %p\n", oastream->codec);
    fprintf(stdout, "oaudio_st.timebase = %d\n", oaudio_st->time_base);

    av_dump_format(ofmt_ctx, 0, outfile, 1);

    fprintf(stdout, "VideoStream = %d, AudioStream = %d\n", out_video_stream, out_audio_stream);    

    float audio_pts = 0.0;
    float video_pts = 0.0;

    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, outfile, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open output file '%s'", outfile);
            return (-1);
        }
    }    
    // fprintf(stdout, "Getting default audio frame\n");
    aframe = alloc_audio_frame(
        iacodec_ctx->sample_fmt,
        iacodec_ctx->channel_layout,
        iacodec_ctx->sample_rate,
        10000,
        iacodec_ctx->channels
    );
    struct SwsContext *sws_ctx;
    struct SwrContext *swr_ctx;
    /* create resampler context */
   swr_ctx = swr_alloc();
   if (!swr_ctx) {
       fprintf(stderr, "Could not allocate resampler context\n");
       exit(1);
   }

   /* set options */
   av_opt_set_int       (swr_ctx, "in_channel_count",   iacodec_ctx->channels,       0);
   av_opt_set_int       (swr_ctx, "in_sample_rate",     iacodec_ctx->sample_rate,    0);
   av_opt_set_int       (swr_ctx, "in_ch_layout",       iacodec_ctx->channel_layout, 0);
   av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt",      iacodec_ctx->sample_fmt,     0);
   av_opt_set_int       (swr_ctx, "out_channel_count",  oacodec_ctx->channels,       0);
   av_opt_set_int       (swr_ctx, "out_sample_rate",    oacodec_ctx->sample_rate,    0);
   av_opt_set_int       (swr_ctx, "out_ch_layout",      oacodec_ctx->channel_layout, 0);
   av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt",     oacodec_ctx->sample_fmt,     0);

   /* initialize the resampling context */
   if ((ret = swr_init(swr_ctx)) < 0) {
       fprintf(stderr, "Failed to initialize the resampling context\n");
       exit(1);
   }
    fprintf(stdout, "AUDIO FRAME samples = %d \n", aframe->nb_samples);
    int counter = 0;

    int nb_samples;
    if (oacodec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE)
       nb_samples = 10000;
    else
       nb_samples = oacodec_ctx->frame_size;

    daframe = alloc_audio_frame(
        oacodec_ctx->sample_fmt,
        oacodec_ctx->channel_layout,
        oacodec_ctx->sample_rate,
        nb_samples,
        oacodec_ctx->channels
    );

    daframe->nb_samples = oacodec_ctx->frame_size;
    daframe->format = oacodec_ctx->sample_fmt;
    daframe->channel_layout = oacodec_ctx->channel_layout;
    daframe->channels = oacodec_ctx->channels;
    daframe->sample_rate = oacodec_ctx->sample_rate;

    FILE * src_pcm = fopen("src.pcm", "wb");
    FILE * scaled_pcm = fopen("scaled.pcm", "wb");

    fprintf(stdout, "Writing header...\n");
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        return (-1);
    };

    int last_vpts = -1;
    int last_apts = -1;

    while(av_read_frame(ifmt_ctx, &packet) >= 0) {

        // fprintf(stdout, "video_pts = %f, audio_pts = %f \n", video_pts, audio_pts);

        // fprintf(stdout, "packet.pts = %d, packet.dts = %d\n", packet.pts, packet.dts);
        fprintf(stdout, "\npacket.pts = %d, packet.dts = %d, packet_stream = %d vsindx = %d asindx = %d\n", packet.pts, packet.dts, packet.stream_index, video_stream, audio_stream);
        fprintf(stdout,
          "video_st.tb.den = %d, video_st.tb.num = %d, ivcctx.tb.den = %d, ivcctx.tb.num = %d\n",
          video_st->time_base.den, video_st->time_base.num, ivcodec_ctx->time_base.den, ivcodec_ctx->time_base.num
        );
        fprintf(stdout,
          "audio_st.tb.den = %d, audio_st.tb.num = %d, iacctx.tb.den = %d, iacctx.tb.num = %d\n",
          audio_st->time_base.den, audio_st->time_base.num, iacodec_ctx->time_base.den, iacodec_ctx->time_base.num
        );

        fprintf(stdout,
          "ovideo_st.tb.den = %d, ovideo_st.tb.num = %d, ovcctx.tb.den = %d, ovcctx.tb.num = %d\n",
          ovideo_st->time_base.den, ovideo_st->time_base.num, ovcodec_ctx->time_base.den, ovcodec_ctx->time_base.num
        );
        fprintf(stdout,
          "oaudio_st.tb.den = %d, oaudio_st.tb.num = %d, oacctx.tb.den = %d, oacctx.tb.num = %d\n",
          oaudio_st->time_base.den, oaudio_st->time_base.num, oacodec_ctx->time_base.den, oacodec_ctx->time_base.num
        );

        if (packet.stream_index == video_stream)
        {
            fprintf(stdout, "Video stream\n");
            int frame_finished;
            int frame_encoded;
            avcodec_decode_video2(ivcodec_ctx, frame, &frame_finished, &packet);
            fprintf(stdout, "vp decoded.frame.pts = %d\n", frame->pts);
            fprintf(stdout, "scaling vpts with:\nvideo_st.tb.den = %d, video_st.tb.num = %d, ivcctx.tb.den = %d, ivcctx.tb.num = %d\n",
                video_st->time_base.den, video_st->time_base.num,
                ivcodec_ctx->time_base.den, ivcodec_ctx->time_base.num
            );
            frame->pts = av_rescale_q(packet.pts, video_st->time_base, ivcodec_ctx->time_base);
            fprintf(stdout, "vp decoded.frame.pts = %d\n", frame->pts);

            if (frame_finished){
                av_init_packet(&target_packet);
                target_packet.size = 0;
                target_packet.data = NULL;

                frame->pts = av_rescale_q(frame->pts, ivcodec_ctx->time_base, ovcodec_ctx->time_base);

                avcodec_encode_video2(ovcodec_ctx, &target_packet, frame, &frame_encoded);
                if (frame_encoded) {
                    video_pts += ((double)ovcodec_ctx->time_base.num / (double)ovcodec_ctx-> time_base.den) ;
                    fprintf(stdout, "video_pts = %f\n", video_pts);
                    target_packet.stream_index = out_video_stream;
                    fprintf(stdout, "[V]...packet.pts = %d, packet.dts = %d \n", target_packet.pts, target_packet.dts);
                    if (last_vpts == -1){
                        fprintf(stdout, "Setting last_vpts\n");
                        last_vpts = target_packet.pts;
                        target_packet.pts = 0;
                    } else {
                        target_packet.pts -= last_vpts;
                    }                    
                    target_packet.pos = -1;
                    target_packet.pts = av_rescale_q(target_packet.pts, ovcodec_ctx->time_base, ovideo_st->time_base);
                    target_packet.dts = AV_NOPTS_VALUE;

                    fprintf(stdout, "[V]packet.pts = %d, packet.dts = %d \n", target_packet.pts, target_packet.dts);
                    if (av_interleaved_write_frame(ofmt_ctx, &target_packet) !=0)
                            die("Error while writing video frame");
                    av_free_packet(&target_packet);
                    av_free_packet(&packet);
                }
                

            }

        } else if (packet.stream_index == audio_stream)
        {
            fprintf(stdout, "Audio stream\n");
            int frame_finished;
            int frame_encoded;
            int dst_nb_samples;
            av_copy_packet(&packet_copy, &packet);
            avcodec_decode_audio4(iacodec_ctx, aframe, &frame_finished, &packet_copy);
            fprintf(stdout, "ap decoded.frame.pts = %d\n", frame->pts);
            fprintf(stdout,
                "scaling apts with:\naudio_st.tb.den = %d, audio_st.tb.num = %d, iacctx.tb.den = %d, iacctx.tb.num = %d\n",
                audio_st->time_base.den, audio_st->time_base.num,
                iacodec_ctx->time_base.den, iacodec_ctx->time_base.num
                );

            aframe->pts = av_rescale_q(packet_copy.pts, audio_st->time_base, iacodec_ctx->time_base);
            fprintf(stdout, "ap decoded.frame.pts = %d\n", frame->pts);

            if (frame_finished ) {
                uint8_t **convertedData=NULL;
                if (av_samples_alloc(&convertedData,
                             NULL,
                             iacodec_ctx->channels,
                             daframe->nb_samples,
                             oacodec_ctx->sample_fmt, 0) < 0)
                    die("Could not allocate samples");
                int outSamples = swr_convert(swr_ctx, NULL, 0,
                             (const uint8_t **)aframe->data,
                             aframe->nb_samples);
                if (outSamples < 0) die("Could not convert");

                for (;;) {

                         outSamples = swr_get_out_samples(swr_ctx, 0);
                             if (outSamples < oacodec_ctx->frame_size) break;
                             outSamples = swr_convert(swr_ctx,
                                                      &convertedData,
                                                      daframe->nb_samples, NULL, 0);

                             size_t buffer_size = av_samples_get_buffer_size(NULL,
                                            oacodec_ctx->channels,
                                            daframe->nb_samples,
                                            oacodec_ctx->sample_fmt,
                                            0);
                            if (buffer_size < 0) die("Invalid buffer size");
                            if (avcodec_fill_audio_frame(daframe,
                                     oacodec_ctx->channels,
                                     oacodec_ctx->sample_fmt,
                                     convertedData,
                                     buffer_size,
                                     0) < 0)
                                die("Could not fill frame");

                    AVPacket outPacket;
                    av_init_packet(&outPacket);
                    outPacket.data = NULL;
                    outPacket.size = 0;
                    daframe->pts = aframe->pts;
                    daframe->pts = av_rescale_q(daframe->pts, iacodec_ctx->time_base, oacodec_ctx->time_base);
                    if (avcodec_encode_audio2(oacodec_ctx, &outPacket, daframe, &frame_finished) < 0)
                        die("Error encoding audio frame");
                    if (frame_finished) {
                        outPacket.stream_index = out_audio_stream;
                        audio_pts += (double)daframe->nb_samples*((double)oacodec_ctx->time_base.num / (double)oacodec_ctx-> time_base.den);
                        fprintf(stdout, "audio_pts = %f\n", audio_pts);
                        fprintf(stdout, "[A]....outPacket.pts = %d, outPacket.dts = %d \n", outPacket.pts, outPacket.dts);
                        outPacket.pos = -1;
                        if (last_apts == -1){
                            fprintf(stdout, "Setting last_apts\n");
                            last_apts = outPacket.pts;
                            outPacket.pts = 0;
                        } else {
                            outPacket.pts -= last_apts;
                        }
                        outPacket.pts = av_rescale_q(outPacket.pts, oacodec_ctx->time_base, oaudio_st->time_base);
                        outPacket.dts = AV_NOPTS_VALUE;

                        fprintf(stdout, "[A]outPacket.pts = %d, outPacket.dts = %d \n", outPacket.pts, outPacket.dts);
                        if (av_interleaved_write_frame(ofmt_ctx, &outPacket) != 0)
                            die("Error while writing audio frame");
                        av_free_packet(&packet);
                        av_free_packet(&packet_copy);
                        av_free_packet(&outPacket);
                    }
                }
                if (convertedData) av_free(&convertedData[0]);
            }
        }

        // Free the packet that was allocated by av_read_frame
        
    }
    fclose(src_pcm);
    fclose(scaled_pcm);

    av_write_trailer(ofmt_ctx);

    // Free the YUV frame
    av_free(frame);

    // Close the codec
    avcodec_close(ivcodec_ctx);
    avcodec_close(iacodec_ctx);

    // Close the video file
    avformat_close_input(&ifmt_ctx);

    /* free the streams */
    for(i = 0; i < ofmt_ctx->nb_streams; i++) {
        av_freep(&ofmt_ctx->streams[i]->codec);
        av_freep(&ofmt_ctx->streams[i]);
    }

    swr_close(swr_ctx);
    swr_free(&swr_ctx);
    
    // av_frame_free(&frame);
    // av_frame_free(&aframe);
    // av_frame_free(&daframe);

    // av_free_packet(&packet);
    // av_free_packet(&packet_copy);
    // av_free_packet(&target_packet);
    // av_write_trailer(ofmt_ctx);
    // avio_close(ofmt_ctx->pb);

    printf("Finish!\n");
    return 0;
}