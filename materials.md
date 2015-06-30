http://miphol.com/muse/2014/03/custom-io-with-ffmpeg.html

http://www.codeproject.com/Tips/489450/Creating-Custom-FFmpeg-IO-Context
https://cdry.wordpress.com/2009/09/09/using-custom-io-callbacks-with-ffmpeg/

http://public.hudl.com/bits/archives/2014/08/15/announcing-hudlffmpeg-a-c-framework-to-make-ffmpeg-interaction-simple/
http://stackoverflow.com/questions/9604633/reading-a-file-located-in-memory-with-libavformat

http://wiki.multimedia.cx/index.php?title=FFmpeg_filter_HOWTO

curl http://ger1.peers.tv/streaming/5kanal/16/tvrec/playlist.m3u8
#EXTM3U
#EXT-X-VERSION:3
#EXT-X-TARGETDURATION:13
#EXT-X-MEDIA-SEQUENCE:2040802
#EXT-X-PROGRAM-DATE-TIME:2015-06-23T14:44:54Z
#EXTINF:8.990611,no desc
segment-1435054549-2040802.ts
#EXT-X-PROGRAM-DATE-TIME:2015-06-23T14:45:03Z
#EXTINF:8.999955,no desc
segment-1435054549-2040803.ts
#EXT-X-PROGRAM-DATE-TIME:2015-06-23T14:45:12Z
#EXTINF:11.981456,no desc
segment-1435054549-2040804.ts
#EXT-X-PROGRAM-DATE-TIME:2015-06-23T14:45:23Z
#EXTINF:9.463967,no desc
segment-1435054549-2040805.ts

https://github.com/mikeboers/PyAV.git
http://code.google.com/p/pyffmpeg/
http://www.kaij.org/blog/?p=98
http://www.dranger.com/ffmpeg/tutorial01.html
http://leixiaohua1020.github.io/
http://habrahabr.ru/post/137793/
http://habrahabr.ru/post/138426/

./simple_ffmpeg_video_audio_player http://ger1.peers.tv/streaming/5kanal/16/tvrec/playlist.m3u8

*1**** FFMPEG INSTALL **********
https://gist.github.com/kostyll/6b2da8c008b8e0e21056

*2*** FFMPEG compile ***********
https://kdenlive.org/download-source#dependencies

./configure --prefix=/usr --enable-shared --enable-libmp3lame --enable-gpl --enable-libfaac --enable-libvorbis --enable-pthreads --enable-libfaac --enable-libxvid --enable-x11grab --enable-libgsm --enable-libx264 --enable-libtheora --enable-libdc1394 --enable-nonfree --disable-stripping --enable-avfilter --enable-libschroedinger --enable-libopencore-amrnb --enable-libopencore-amrwb --enable-version3 --enable-libvpx

make -j3

sudo make install
****************************************

gcc -o simple_ffmpeg_player simple_ffmpeg_player.c -I/usr/include/SDL -lSDL -I ffmpeg-2.7.1/libavutil/ -I ffmpeg-2.7.1/libavcodec/ -I ffmpeg-2.7.1/libswscale/ -I ffmpeg-2.7.1/libavformat/ -L ffmpeg-2.7.1/libavutil/ -L ffmpeg-2.7.1/libavcodec/ -L ffmpeg-2.7.1/libavformat/ -L ffmpeg-2.7.1/libswscale/ -lavutil -lavutil -lavformat -lrtmp -lvpx -lswscale
./simple_ffmpeg_player http://ger1.peers.tv/streaming/5kanal/16/tvrec/playlist.m3u8

****************
http://www.iakovlev.org/index.html?p=1403
https://mafayyaz.wordpress.com/2013/09/05/ffmpeg-sample-to-decode-video-using-libavformat/
http://www.cyberforum.ru/cpp/thread938475.html
http://dranger.com/ffmpeg/functions.html#av_read_frame
