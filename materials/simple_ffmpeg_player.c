/**
 * FFmpeg sample player
 * Related to the article at http://habrahabr.ru/blogs/video/137793/
 */
#include <stdio.h>

#include <signal.h>     /* Signal handling */

#include <SDL.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>

FILE * f;

/* Our own SIGTERM handler */
void mysigint()
{
    printf("I caught the SIGINT signal!\n");
    fclose(f);
    exit(0);
}

/* Our own SIGKILL handler */
void mysigkill()
{
    printf("I caught the SIGKILL signal!\n");
    fclose(f);
    exit(0);
}

/* Our own SIGHUP handler */
void mysighup()
{
    printf("I caught the SIGHUP signal!\n");
    fclose(f);
    exit(0);
}

/* Our own SIGTERM handler */
void mysigterm()
{
    printf("I caught the SIGTERM signal!\n");
    fclose(f);
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s filename\n", argv[0]);
        return 0;
    }
    f = fopen("002.avi", "wb");
    if (signal(SIGINT, mysigint) == SIG_ERR)
       printf("Cannot handle SIGINT!\n");
    //if (signal(SIGHUP, mysighup) == SIG_ERR)
    //   printf("Cannot handle SIGHUP!\n");
    //if (signal(SIGTERM, mysigterm) == SIG_ERR)
    //   printf("Cannot handle SIGTERM!\n");

    /* can SIGKILL be handled by our own function? */
    //if (signal(SIGKILL, mysigkill) == SIG_ERR)
    //   printf("Cannot handle SIGKILL!\n");

    // Register all available file formats and codecs
    av_register_all();

    int err;
    // Init SDL with video support
    err = SDL_Init(SDL_INIT_VIDEO);
    if (err < 0) {
        fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
        return -1;
    }

    // Open video file
    const char* filename = argv[1];
    AVFormatContext* format_context = NULL;
    err = avformat_open_input(&format_context, filename, NULL, NULL);
    if (err < 0) {
        fprintf(stderr, "ffmpeg: Unable to open input file\n");
        return -1;
    }

    // Retrieve stream information
    err = avformat_find_stream_info(format_context, NULL);
    if (err < 0) {
        fprintf(stderr, "ffmpeg: Unable to find stream info\n");
        return -1;
    }

    // Dump information about file onto standard error
    av_dump_format(format_context, 0, argv[1], 0);

    // Find the first video stream
    int video_stream;
    for (video_stream = 0; video_stream < format_context->nb_streams; ++video_stream) {
        if (format_context->streams[video_stream]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            break;
        }
    }
    if (video_stream == format_context->nb_streams) {
        fprintf(stderr, "ffmpeg: Unable to find video stream\n");
        return -1;
    }

    AVCodecContext* codec_context = format_context->streams[video_stream]->codec;
    AVCodec* codec = avcodec_find_decoder(codec_context->codec_id);
    err = avcodec_open2(codec_context, codec, NULL);
    if (err < 0) {
        fprintf(stderr, "ffmpeg: Unable to open codec\n");
        return -1;
    }

    SDL_Surface* screen = SDL_SetVideoMode(codec_context->width, codec_context->height, 0, 0);
    if (screen == NULL) {
        fprintf(stderr, "Couldn't set video mode\n");
        return -1;
    }

    SDL_Overlay* bmp = SDL_CreateYUVOverlay(codec_context->width, codec_context->height,
                                            SDL_YV12_OVERLAY, screen);

    struct SwsContext* img_convert_context;
    img_convert_context = sws_getCachedContext(NULL,
                                                codec_context->width, codec_context->height,
                                                codec_context->pix_fmt,
                                                codec_context->width, codec_context->height,
                                                PIX_FMT_YUV420P, SWS_BICUBIC,
                                                NULL, NULL, NULL);
    if (img_convert_context == NULL) {
        fprintf(stderr, "Cannot initialize the conversion context\n");
        return -1;
    }


    AVFrame* frame = avcodec_alloc_frame();
    AVPacket packet;
    AVPacket packet_copy;

    // preparing output ...
    int i, ret;
    char * outputfile = "test.mpg";
    AVFormatContext * oformat_context = NULL;
    AVOutputFormat *ofmt = NULL;
    avformat_alloc_output_context2(&oformat_context, NULL, NULL, outputfile);
    if (!oformat_context) {
        fprintf(stderr, "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        return (-1);
    };
    ofmt = oformat_context->oformat;

    for (i = 0; i < format_context->nb_streams; i++) {
        AVStream *in_stream = format_context->streams[i];
        AVStream *out_stream = avformat_new_stream(oformat_context, in_stream->codec->codec);
        if (!out_stream) {
            fprintf(stderr, "Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            return (-1);
        }
        ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
        if (ret < 0) {
            fprintf(stderr, "Failed to copy context from input to output stream codec context\n");
            return (-1);
        }
        out_stream->codec->codec_tag = 0;
        if (oformat_context->oformat->flags & AVFMT_GLOBALHEADER)
            out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
    av_dump_format(oformat_context, 0, outputfile, 1);
    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&oformat_context->pb, outputfile, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open output file '%s'", outputfile);
            return (-1);
        }
    }
    ret = avformat_write_header(oformat_context, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        return (-1);
    };
    AVStream *in_stream, *out_stream;


    while (av_read_frame(format_context, &packet) >= 0) {
        av_copy_packet(&packet_copy, &packet);
        // in_stream  = format_context->streams[packet_copy.stream_index];
        // out_stream = oformat_context->streams[packet_copy.stream_index];

        // packet_copy.pts = av_rescale_q_rnd(packet_copy.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        // packet_copy.dts = av_rescale_q_rnd(packet_copy.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        // packet_copy.duration = av_rescale_q(packet_copy.duration, in_stream->time_base, out_stream->time_base);
        // packet_copy.pos = -1;
        ret = av_interleaved_write_frame(oformat_context, &packet_copy);
        if (ret < 0) {
            fprintf(stderr, "Error muxing packet\n");
        };

        if (packet.stream_index == video_stream) {
            // Video stream packet
            int frame_finished;

            avcodec_decode_video2(codec_context, frame, &frame_finished, &packet);

            if (frame_finished) {
                SDL_LockYUVOverlay(bmp);

                // Convert frame to YV12 pixel format for display in SDL overlay

                AVPicture pict;
                pict.data[0] = bmp->pixels[0];
                pict.data[1] = bmp->pixels[2];  // it's because YV12
                pict.data[2] = bmp->pixels[1];

                pict.linesize[0] = bmp->pitches[0];
                pict.linesize[1] = bmp->pitches[2];
                pict.linesize[2] = bmp->pitches[1];

                sws_scale(img_convert_context,
                            frame->data, frame->linesize,
                            0, codec_context->height,
                            pict.data, pict.linesize);

                SDL_UnlockYUVOverlay(bmp);

                SDL_Rect rect;
                rect.x = 0;
                rect.y = 0;
                rect.w = codec_context->width;
                rect.h = codec_context->height;
                SDL_DisplayYUVOverlay(bmp, &rect);

                printf("%d\n", packet.size);
                fwrite(packet.data, 1, packet.size, f);

            }
        }

        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);

        // Handling SDL events there
        SDL_Event event;
        if (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                break;
            }
        }
    }

    fclose(f);

    av_write_trailer(oformat_context);

    if (oformat_context && !(ofmt->flags & AVFMT_NOFILE))
        avio_closep(&oformat_context->pb);
    avformat_free_context(oformat_context);

    sws_freeContext(img_convert_context);

    // Free the YUV frame
    av_free(frame);

    // Close the codec
    avcodec_close(codec_context);

    // Close the video file
    avformat_close_input(&format_context);

    // Quit SDL
    SDL_Quit();
    return 0;
}
