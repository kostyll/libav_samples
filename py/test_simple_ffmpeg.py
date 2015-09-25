import sys
import simple_ffmpeg

def callback(tctx, source, output):
    print source.url, output.url
    return 1

def audio_after_encode_callback(tctx, source, output):
    print "after_encode_audio callback is called!"
    return 1

def main():
    infile = sys.argv[1]
    outfile = sys.argv[2]

    simple_ffmpeg.sff_register_all()
    simple_ffmpeg.sff_network_init()

    source = simple_ffmpeg.open_source(infile, 1, 1)
    # simple_ffmpeg.sff_dump_format(source, infile)

    output = simple_ffmpeg.sff_open_output(outfile, None, None, None, 1, 1)
    # simple_ffmpeg.sff_dump_format(output, outfile)

    tctx = simple_ffmpeg.build_transcoding_context(source, output)

    simple_ffmpeg.init_processCallback()

    tctx.before_decode_video = simple_ffmpeg.registerHandler(callback, tctx.before_decode_video)
    tctx.after_encode_audio = simple_ffmpeg.registerHandler(audio_after_encode_callback, None)

    simple_ffmpeg.sff_write_header(output)

    simple_ffmpeg.transcode(source, output, tctx)

    simple_ffmpeg.sff_write_trailer(output)

if __name__ == "__main__":
    main()