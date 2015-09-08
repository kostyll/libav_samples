#include <stdio.h>


#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>

AVFormatContext * open_input_source(char *source) {
    AVFormatContext * result = NULL;
    int err;
    
    // Open video file
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

    AVInputFormat * ifmt = NULL;
    AVFormatContext * ifmt_ctx = NULL;

    AVOutputFormat * ofmt = NULL;
    AVFormatContext * ofmt_ctx = NULL;

    ifmt_ctx = open_input_source(infile);
    ifmt = ifmt_ctx->iformat;

    int video_stream = get_video_stream(ifmt_ctx);
    int audio_stream = get_audio_stream(ifmt_ctx);

    fprintf(stdout, "VideoStream = %d, AudioStream = %d\n", video_stream, audio_stream);

    AVCodecContext* icodec_ctx = ifmt_ctx->streams[video_stream]->codec;
    AVCodec* icodec = avcodec_find_decoder(icodec_ctx->codec_id);

    err = avcodec_open2(icodec_ctx, icodec, NULL);
    if (err < 0) {
        fprintf(stderr, "ffmpeg: Unable to open codec\n");
        return -1;
    }

    AVFrame* frame = avcodec_alloc_frame();
    AVPacket packet;
    AVPacket packet_copy;

    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, outfile);
    if (!ofmt_ctx) {
        fprintf(stderr, "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        return (-1);
    };
    ofmt = ofmt_ctx->oformat;

    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream *in_stream = ifmt_ctx->streams[i];
        AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
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
        if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
    av_dump_format(ofmt_ctx, 0, outfile, 1);
    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, outfile, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open output file '%s'", outfile);
            return (-1);
        }
    }
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        return (-1);
    };

    while(av_read_frame(ifmt_ctx, &packet) >= 0) {
        av_copy_packet(&packet_copy, &packet);

        ret = av_interleaved_write_frame(ofmt_ctx, &packet_copy);
        if (ret < 0) {
            fprintf(stderr, "Error muxing packet\n");
        };
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }

    av_write_trailer(ofmt_ctx);

    // Free the YUV frame
    av_free(frame);

    // Close the codec
    avcodec_close(icodec_ctx);

    // Close the video file
    avformat_close_input(&ifmt_ctx);

    /* free the streams */
    for(i = 0; i < ofmt_ctx->nb_streams; i++) {
        av_freep(&ofmt_ctx->streams[i]->codec);
        av_freep(&ofmt_ctx->streams[i]);
    }

    printf("Finish!\n");
    return 0;
}