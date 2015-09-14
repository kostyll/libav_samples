cmake_minimum_required(VERSION 2.8.1)

project (libav_samples)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")

include_directories(
    /usr/include/SDL
    target_usr/include
)

link_directories(
    /usr/lib/i386-linux-gnu
    target_usr/lib
)

set(SOURCE_SIMPLE_FFMPEG_PLAYER "simple_ffmpeg_player.c")
set(SOURCE_SIMPLE_FFMPEG_VA_PLAYER "simple_ffmpeg_video_audio_player.c")

set(SOURCE_OUTPUT_EXAMPLE "output-example.c")

set(SOURCE_VIDEO_CONVERT "test_video_convert.c")

set(SOURCE_DECODE_ENCODE "de.c")

set(
    SOURCE_SOME_DECODE_CONVERT_ENCODE_AUDIO_SAMPLE
    "some_audio_decode_convert_encode_sample.c"
)

add_executable(simple_ffmpeg_player ${SOURCE_SIMPLE_FFMPEG_PLAYER})
add_executable(simple_ffmpeg_video_audio_player ${SOURCE_SIMPLE_FFMPEG_VA_PLAYER})
add_executable(
    some_audio_decode_convert_encode_sample
    ${SOURCE_SOME_DECODE_CONVERT_ENCODE_AUDIO_SAMPLE}
)

# add_executable(
#     de
#     ${SOURCE_DECODE_ENCODE}
# )

# add_executable(output-example ${SOURCE_OUTPUT_EXAMPLE})

add_executable(test_video_convert ${SOURCE_VIDEO_CONVERT})

target_link_libraries(
    simple_ffmpeg_player -lSDL -lm -ldl -lrt -lpthread -lz -lavformat -lswscale -lavcodec -lavutil
)

target_link_libraries(
    simple_ffmpeg_video_audio_player -lSDL -lm -ldl -lrt -lpthread -lz -lavformat -lswscale -lavcodec -lavutil
)

target_link_libraries(
    test_video_convert 
    -lSDL -lm -ldl -lrt -lpthread -lz 
    -lavformat -lswscale -lavcodec -lavutil -lswresample
)

target_link_libraries(
    some_audio_decode_convert_encode_sample -lavformat -lswscale -lavcodec -lavutil -lswresample
)

# target_link_libraries(
#     de  -lavformat -lswscale -lavcodec -lavutil -lswresample
# )

# target_link_libraries(
#     output-example -lavformat -lswscale -lavcodec -lavutil
# )

#  MAKING ffmpeg-tutorial0

set(src_dir "ffmpeg-tutorial0/")

set(list_sources 
    "tutorial01" 
    "tutorial02" 
    "tutorial03" 
    "tutorial04" 
    # "tutorial05" 
    "tutorial06" 
    "tutorial07" 
)

foreach(source ${list_sources})
    set(src_file "${src_dir}${source}.c")
    set(dst_file "${src_dir}${source}")
    # message(STATUS "${source}")
    add_executable(
            ${dst_file}
            ${src_file}
        )
    target_link_libraries(
        ${dst_file} -lSDL -lm -ldl -lrt -lpthread -lz -lavformat -lswscale -lavcodec -lavutil 
    )
endforeach(source list_sources)

set(src_dir "FFmpegTutorial/")

set(list_sources 
    "sample01_scaning"
    "sample02_demuxing"
    "sample03_remuxing"
    "sample04_decoding"
    "sample05_filtering"
    "sample06_encoding"
)

foreach(source ${list_sources})
    set(src_file "${src_dir}${source}.c")
    set(dst_file "${src_dir}${source}")
    # message(STATUS "${source}")
    add_executable(
            ${dst_file}
            ${src_file}
        )
    target_link_libraries(
        ${dst_file} -lSDL -lm -ldl -lrt -lpthread -lz -lavfilter -lavformat -lswscale -lavcodec -lavutil 
    )
endforeach(source list_sources)

add_custom_target(
    runcb
    test_video_convert big_buck_bunny_720p_2mb.mp4 out.avi 
)


add_custom_target(
    runvb
    vlc out.avi
)

add_custom_target(
    runcr
    ./test_video_convert http://ger1.peers.tv/streaming/5kanal/16/tvrec/playlist.m3u8 test.avi 
)


add_custom_target(
    runvr
    vlc test.avi
)