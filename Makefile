windows:
	gcc -o simple_ffmpeg_player simple_ffmpeg_player.c -I/usr/include/SDL -lSDL -lavformat -lswscale
	gcc -o simple_ffmpeg_video_audio_player simple_ffmpeg_video_audio_player.c -I/usr/include/SDL -lSDL -lavformat -lswscale
linux:
	gcc -c simple_ffmpeg_player.c -o simple_ffmpeg_player.o -I/usr/include/SDL -Itarget_usr/include 
	gcc -o simple_ffmpeg_player  \
	 simple_ffmpeg_player.o \
	 -L/usr/lib/i386-linux-gnu/  -lSDL \
	 -lm -ldl -lrt -lpthread -lz \
	 -Ltarget_usr/lib -lavformat -lswscale -lavcodec -lavutil
