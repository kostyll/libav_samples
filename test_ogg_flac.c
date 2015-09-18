#include <stdio.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>

#include "general.h"

AVCodecContext * add_video_output(AVFormatContext * fmt_ctx){

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

	return vcodec_ctx;
}

AVCodecContext * add_audio_output(AVFormatContext * fmt_ctx){
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

	return acodec_ctx;
}


AVFormatContext * open_output_off_flac(char * outfile){
	AVFormatContext * ctx;

	AVCodecContext * vcodec_ctx = NULL;

	AVCodecContext * acodec_ctx = NULL;

	avformat_alloc_output_context2(&ctx, NULL, NULL, outfile);
	if (ctx == NULL) die("Cannot allocate output format context\n");

	vcodec_ctx = add_video_output(ctx);

	acodec_ctx = add_audio_output(ctx);

	if (!(ctx->flags & AVFMT_NOFILE)) {
		if (avio_open(&ctx->pb, outfile, AVIO_FLAG_WRITE) < 0)
			die("Cannot open output file\n");
	}

	if ((avformat_write_header(ctx, NULL)) < 0)
		die("Cannot write header\n");

	return ctx;
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
	AVFormatContext * ifmt_ctx = NULL;
	AVInputFormat * ifmt = NULL;
	ifmt_ctx = open_input_source(argv[1]);
	// ifmt = ifmt_ctx->iformat;

	//Creating output
	AVFormatContext * ofmt_ctx = NULL;
	AVOutputFormat * ofmt = NULL;

	ofmt_ctx = open_output_off_flac(outfile);

	return 0;
}