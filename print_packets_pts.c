#include <stdio.h>

#include <libavformat/avformat.h>



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
    char *infile = NULL;
    int ret;
    int video_stream = -1;
    int audio_stream = -1;
    if (argc != 2){
        fprintf(stdout, "USAGE: %s <infile>\n", argv[0]);
        return 0;
    }

    av_register_all();

    infile = argv[1];
    fprintf(stdout, "File to analize = %s\n", infile);
    ifmt_ctx = avformat_alloc_context();
    ret =  avformat_open_input(&ifmt_ctx, infile, NULL, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error while opening file %s\n", infile);
        return 0;
    }

    video_stream = get_video_stream(ifmt_ctx);
    audio_stream = get_audio_stream(ifmt_ctx);
    av_dump_format(ifmt_ctx, 0, infile, 0);
    av_init_packet(&inPacket);
    inPacket.data = NULL;
    inPacket.size = 0;

    while (av_read_frame(ifmt_ctx, &inPacket) >= 0) {
        fprintf(stdout, "Packet.pts = %d ", inPacket.pts);
        if (inPacket.stream_index == video_stream) {
            fprintf(stdout, "VIDEO packet\n");
        } else if (inPacket.stream_index == audio_stream) {
            fprintf(stdout, "AUDIO packet\n");
        } else fprintf(stdout, "***** packet\n");
    }
}