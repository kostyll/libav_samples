all:
	gcc -o simple_ffmpeg_player simple_ffmpeg_player.c -I/usr/include/SDL -lSDL -lavformat -lswscale
