#include <stdio.h>
#include <string.h>

#include <libavformat/avformat.h>

#define true 1
#define false 0


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




int main(int argc, char **argv){
    AVPacket inPacket;
    AVFormatContext *ifmt_ctx = NULL;
    int as_csv = false;
    int v = false;
    int a = false;
    char *infile = NULL;
    int ret;
    int video_stream = -1;
    int audio_stream = -1;
    if (argc < 2){
        fprintf(stdout, "USAGE: %s <infile>\n", argv[0]);
        return 0;
    }
    if (argc == 3) {
        as_csv = true;
        if (strcmp(argv[2], "all") == 0) {
            v = true;
            a = true;
        }
        if (strcmp(argv[2], "video") == 0) {
            v = true;
        }
        if (strcmp(argv[2], "audio") == 0) {
            a = true;
        }
    }

    av_register_all();
    avformat_network_init();

    infile = argv[1];
    if (!as_csv) fprintf(stdout, "File to analize = %s\n", infile);
    ifmt_ctx = avformat_alloc_context();
    ret =  avformat_open_input(&ifmt_ctx, infile, NULL, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error while opening file %s\n", infile);
        return 0;
    }

    video_stream = get_video_stream(ifmt_ctx);
    audio_stream = get_audio_stream(ifmt_ctx);
    if (!as_csv) av_dump_format(ifmt_ctx, 0, infile, 0);
    av_init_packet(&inPacket);
    inPacket.data = NULL;
    inPacket.size = 0;

    while (av_read_frame(ifmt_ctx, &inPacket) >= 0) {
        if (!as_csv) fprintf(stdout, "Packet.pts = %d ", inPacket.pts); 
        if (inPacket.stream_index == video_stream) {
            if (!as_csv) fprintf(stdout, "VIDEO packet\n");
            else if (v) fprintf(stdout, "V, %d,\n", inPacket.pts);
        } else if (inPacket.stream_index == audio_stream) {
            if (!as_csv) fprintf(stdout, "AUDIO packet\n");
            else if (a) fprintf(stdout, "A, %d,\n", inPacket.pts);
        } else fprintf(stdout, "***** packet\n");
    }
}