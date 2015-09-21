#include <stdio.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/frame.h>

#include "general.h"
#include "transcode.h"

int main(int argc, char ** argv){
	if (argc != 3) die("not enought arguments");

	av_register_all();
	avformat_network_init();

	InputSource * source = open_source(argv[1], 1, 1);
	Output * output = open_output(argv[2], NULL, NULL, NULL, 1, 1);

	av_dump_format(output->ctx, 0, argv[2], 1);

	avformat_write_header(output->ctx, NULL);

	TranscodingContext * tctx = build_transcoding_context(source, output);

	transcode(source, output, tctx);

	av_write_trailer(output->ctx);

	avformat_close_input(&source->ctx);
	avformat_close_input(&output->ctx);

	return 0;
}