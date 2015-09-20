%module simple_ffmpeg

%{
 #include "general.h"	
%}

%include "general.h"

extern void die(char *str);

#swig -python simple_ffmpeg.i
#gcc -c -fpic simple_ffmpeg_wrap.c  -DHAVE_CONFIG_H  -I/usr/include/python2.7 -Itarget_usr/include
#gcc -shared simple_ffmpeg_wrap.o general.o  -o _simple_ffmpeg.so -Ltarget_usr/lib -lavformat -lavcodec