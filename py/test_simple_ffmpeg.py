import sys
import simple_ffmpeg

def main():
    infile = sys.argv[1]
    outfile = sys.argv[2]

    simple_ffmpeg.sff_register_all()

    print "opening input"
    source = simple_ffmpeg.open_source(infile, 1, 1)
    print "opened"
    print "dumping format"
    # simple_ffmpeg.sff_dump_format(source, infile)

    print "making output"
    output = simple_ffmpeg.open_output(outfile, 0, 0, 0, 1, 1)
    print "done"
    simple_ffmpeg.sff_dump_format(output, outfile)

    #tctx = simple_ffmpeg.build_transcoding_context(source, output)

    simple_ffmpeg.sff_write_header(output)

    simple_ffmpeg.sff_write_trailer(output)

if __name__ == "__main__":
    main()