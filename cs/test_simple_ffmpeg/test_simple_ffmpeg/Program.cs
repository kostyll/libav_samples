using System;

namespace test_simple_ffmpeg
{
	class MainClass
	{
		public static void Main (string[] args)
		{
			Console.WriteLine ("Hello World!");
			simple_ffmpeg sff = new simple_ffmpeg();

			simple_ffmpeg.sff_register_all ();
			simple_ffmpeg.sff_network_init ();

			string infile = "big_buck_bunny_720p_2mb.mp4";

			SourceOrDestinatio source = simple_ffmpeg.open_source(infile, 1, 1);

			//simple_ffmpeg.sff_dump_format (source, infile);

		}
	}
}
