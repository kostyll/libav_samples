#include <stdio.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>

#include "general.h"


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
	ifmt = ifmt_ctx->iformat;

	//Creating output
	AVFormatContext * ofmt_ctx = NULL;
	AVOutputFormat * ofmt = NULL;

	return 0;
}