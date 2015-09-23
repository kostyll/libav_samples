%module simple_ffmpeg

%{
 #include "general.h"
 #include "transcode.h"
 #include "callbacks_table.h"
%}

%include "include/general.h"
%include "include/transcode.h"
