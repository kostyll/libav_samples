using System;
using System.Runtime.InteropServices;

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
			string outfile = "out.mp4";

			SourceOrDestinatio source= simple_ffmpeg.open_source(
											infile,
											1, 1);


			Console.WriteLine (source.url);

			SourceOrDestinatio output = simple_ffmpeg.open_output (
				                            outfile,
				                            null,
				                            null,
				                            null,
				                            1, 1);

			Console.WriteLine (output.url);

			SWIGTYPE_p_void pointer;
			IntPtr ptr;
			HandleRef href;
			href = SourceOrDestinatio.getCPtr (source);
			ptr = href.Handle;
			pointer = new SWIGTYPE_p_void(
				ptr, true
			);
			Console.WriteLine (pointer.ToString());
			simple_ffmpeg.sff_dump_format (pointer, infile);

			simple_ffmpeg.sff_dump_format (new SWIGTYPE_p_void(SourceOrDestinatio.getCPtr (output).Handle, true), outfile);


			simple_ffmpeg.sff_write_header (output);

			simple_ffmpeg.sff_write_trailer (output);

			Console.WriteLine ("Done!");

		}
	}
}
