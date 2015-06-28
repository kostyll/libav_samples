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
https://gist.github.com/xlphs/9895065

*****************************************************************

wine ~/.wine/drive_c/windows/system32/cmd.exe

set PATH=%PATH%;z:\media\andrew\9A50D0DD50D0C165\mingw_2\bin

H:\workspace\libav_samples>mingw32-gcc simple_ffmpeg_player.c -Iz:\opt\SDL-1.2.13\include\SDL -I ffmpeg-20150627-git-7728d23-win32-dev\include -L ffmpeg-20150627-git-7728d23-win32-dev\lib -lavformat -lavcodec  -L z:\opt\SDL-1.2.13\lib -lswscale -lavutil -lmingw32 -lSDLMain -lSDL
