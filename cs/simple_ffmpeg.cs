/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 2.0.12
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */


using System;
using System.Runtime.InteropServices;

public class simple_ffmpeg {
  public static void die(string str) {
    simple_ffmpegPINVOKE.die(str);
  }

  public static SWIGTYPE_p_AVFormatContext open_input_source(string source) {
    IntPtr cPtr = simple_ffmpegPINVOKE.open_input_source(source);
    SWIGTYPE_p_AVFormatContext ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_AVFormatContext(cPtr, false);
    return ret;
  }

  public static int get_video_stream(SWIGTYPE_p_AVFormatContext fmt_ctx) {
    int ret = simple_ffmpegPINVOKE.get_video_stream(SWIGTYPE_p_AVFormatContext.getCPtr(fmt_ctx));
    return ret;
  }

  public static int get_audio_stream(SWIGTYPE_p_AVFormatContext fmt_ctx) {
    int ret = simple_ffmpegPINVOKE.get_audio_stream(SWIGTYPE_p_AVFormatContext.getCPtr(fmt_ctx));
    return ret;
  }

  public static SWIGTYPE_p_AVFrame alloc_audio_frame(int sample_fmt, SWIGTYPE_p_uint64_t channel_layout, int sample_rate, int nb_samples, int channels) {
    IntPtr cPtr = simple_ffmpegPINVOKE.alloc_audio_frame((int)sample_fmt, SWIGTYPE_p_uint64_t.getCPtr(channel_layout), sample_rate, nb_samples, channels);
    SWIGTYPE_p_AVFrame ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_AVFrame(cPtr, false);
    if (simple_ffmpegPINVOKE.SWIGPendingException.Pending) throw simple_ffmpegPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public static SWIGTYPE_p_SwrContext build_audio_swr(SWIGTYPE_p_AVCodecContext in_ctx, SWIGTYPE_p_AVCodecContext out_ctx) {
    IntPtr cPtr = simple_ffmpegPINVOKE.build_audio_swr(SWIGTYPE_p_AVCodecContext.getCPtr(in_ctx), SWIGTYPE_p_AVCodecContext.getCPtr(out_ctx));
    SWIGTYPE_p_SwrContext ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_SwrContext(cPtr, false);
    return ret;
  }

  public static SourceOrDestinatio open_source(string url, int video, int audio) {
    IntPtr cPtr = simple_ffmpegPINVOKE.open_source(url, video, audio);
    SourceOrDestinatio ret = (cPtr == IntPtr.Zero) ? null : new SourceOrDestinatio(cPtr, false);
    return ret;
  }

  public static SourceOrDestinatio open_output(string outfile, SWIGTYPE_p_f_p_AVFormatContext_p_AVCodecContext_p_char__p_AVStream make_video, SWIGTYPE_p_f_p_AVFormatContext_p_AVCodecContext_p_char__p_AVStream make_audio, SourceOrDestinatio source, int video, int audio) {
    IntPtr cPtr = simple_ffmpegPINVOKE.open_output(outfile, SWIGTYPE_p_f_p_AVFormatContext_p_AVCodecContext_p_char__p_AVStream.getCPtr(make_video), SWIGTYPE_p_f_p_AVFormatContext_p_AVCodecContext_p_char__p_AVStream.getCPtr(make_audio), SourceOrDestinatio.getCPtr(source), video, audio);
    SourceOrDestinatio ret = (cPtr == IntPtr.Zero) ? null : new SourceOrDestinatio(cPtr, false);
    return ret;
  }

  public static SWIGTYPE_p_TranscodingContext build_transcoding_context(SourceOrDestinatio source, SourceOrDestinatio output) {
    IntPtr cPtr = simple_ffmpegPINVOKE.build_transcoding_context(SourceOrDestinatio.getCPtr(source), SourceOrDestinatio.getCPtr(output));
    SWIGTYPE_p_TranscodingContext ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_TranscodingContext(cPtr, false);
    return ret;
  }

  public static int close_source(SourceOrDestinatio arg0) {
    int ret = simple_ffmpegPINVOKE.close_source(SourceOrDestinatio.getCPtr(arg0));
    return ret;
  }

  public static int close_output(SourceOrDestinatio arg0) {
    int ret = simple_ffmpegPINVOKE.close_output(SourceOrDestinatio.getCPtr(arg0));
    return ret;
  }

  public static int close_transcoding_context(SWIGTYPE_p_TranscodingContext arg0) {
    int ret = simple_ffmpegPINVOKE.close_transcoding_context(SWIGTYPE_p_TranscodingContext.getCPtr(arg0));
    return ret;
  }

  public static void sff_register_all() {
    simple_ffmpegPINVOKE.sff_register_all();
  }

  public static int sff_network_init() {
    int ret = simple_ffmpegPINVOKE.sff_network_init();
    return ret;
  }

  public static void sff_dump_format(SWIGTYPE_p_void arg0, string arg1) {
    simple_ffmpegPINVOKE.sff_dump_format(SWIGTYPE_p_void.getCPtr(arg0), arg1);
  }

  public static int sff_write_header(SourceOrDestinatio arg0) {
    int ret = simple_ffmpegPINVOKE.sff_write_header(SourceOrDestinatio.getCPtr(arg0));
    return ret;
  }

  public static int sff_write_trailer(SourceOrDestinatio arg0) {
    int ret = simple_ffmpegPINVOKE.sff_write_trailer(SourceOrDestinatio.getCPtr(arg0));
    return ret;
  }

}
