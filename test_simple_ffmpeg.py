import sys
import simple_ffmpeg

def main():
	infile = sys.argv[1]
	outfile = sys.argv[2]

	source = simple_ffmpeg.open_source(infile,1,1)
	output = simple_ffmpeg.open_output(outfile)


if __name__ == "__main__":
	main()