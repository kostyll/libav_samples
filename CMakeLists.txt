cmake_minimum_required(VERSION 2.8.1)

project (libav_samples)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -ggdb -O0 ")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w  -ggdb -O0 ")

include_directories(
    /usr/include/SDL
    /usr/include/python2.7
    target_usr/include
    ./
    ./include
)

link_directories(
    /usr/lib/i386-linux-gnu
    target_usr/lib
)

set(SOURCE_SIMPLE_FFMPEG_PLAYER "simple_ffmpeg_player.c")
set(SOURCE_SIMPLE_FFMPEG_VA_PLAYER "simple_ffmpeg_video_audio_player.c")

set(SOURCE_OUTPUT_EXAMPLE "output-example.c")

set(SOURCE_VIDEO_CONVERT "test_video_convert.c")

set(SOURCE_GENERAL "general.c")

set(SOURCE_TRANSCODE "transcode.c")

set(SOURCE_DECODE_ENCODE "de.c")

set(
    SOURCE_SOME_DECODE_CONVERT_ENCODE_AUDIO_SAMPLE
    "some_audio_decode_convert_encode_sample.c"
)

set (
    SOURCE_PRINT_PACKETS_PTS
    "print_packets_pts.c"
)

set (
    SOURCE_REMUXING
    "remuxing.c"
)

set (
    SOURCE_TRANSCODING
    "transcoding.c"
)

set(
    SOURCE_LOSELESS_OGG_FLAC
    "test_ogg_flac.c"
)

set(
    SOURCE_TEST_TRANSCODER
    "test_transcoder.c"
)

set(
    SRC_DIR
    "src"
)

set(
    TEST_DIR
    "test"
)


# add_executable(simple_ffmpeg_player ${SOURCE_SIMPLE_FFMPEG_PLAYER})
# add_executable(simple_ffmpeg_video_audio_player ${SOURCE_SIMPLE_FFMPEG_VA_PLAYER})
# add_executable(
#     some_audio_decode_convert_encode_sample
#     ${SOURCE_SOME_DECODE_CONVERT_ENCODE_AUDIO_SAMPLE}
# )

# add_executable(
#     print_pts
#     ${SOURCE_PRINT_PACKETS_PTS}
# )

# add_executable(
#     remuxing
#     ${SOURCE_REMUXING}
# )

# add_executable(
#     transcoding
#     ${SOURCE_TRANSCODING}
# )

# add_executable(
#     de
#     ${SOURCE_DECODE_ENCODE}
# )

# add_executable(output-example ${SOURCE_OUTPUT_EXAMPLE})

add_executable(
    test_video_convert
    ${TEST_DIR}/${SOURCE_VIDEO_CONVERT}
    ${SRC_DIR}/${SOURCE_GENERAL}
)
add_executable(
    test_ogg_flac
    ${TEST_DIR}/${SOURCE_LOSELESS_OGG_FLAC}
    ${SRC_DIR}/${SOURCE_GENERAL}
    ${SRC_DIR}/${SOURCE_TRANSCODE}
)

add_executable(
    test_transcoder
    ${TEST_DIR}/${SOURCE_TEST_TRANSCODER}
    ${SRC_DIR}/${SOURCE_GENERAL}
    ${SRC_DIR}/${SOURCE_TRANSCODE}
)

# target_link_libraries(
#     simple_ffmpeg_player -lSDL -lm -ldl -lrt -lpthread -lz -lavformat -lswscale -lavcodec -lavutil
# )

# target_link_libraries(
#     simple_ffmpeg_video_audio_player -lSDL -lm -ldl -lrt -lpthread -lz -lavformat -lswscale -lavcodec -lavutil
# )

target_link_libraries(
    test_video_convert 
    -lSDL -lm -ldl -lrt -lpthread -lz 
    -lavformat -lswscale -lavcodec -lavutil -lswresample
)

target_link_libraries(
    test_ogg_flac
    -lavformat -lswscale -lavcodec -lavutil -lswresample
)

target_link_libraries(
    test_transcoder
    -lavformat -lswscale -lavcodec -lavutil -lswresample
)

set_target_properties(test_video_convert PROPERTIES 
  RUNTIME_OUTPUT_DIRECTORY "${TEST_DIR}"
)

set_target_properties(test_ogg_flac PROPERTIES 
  RUNTIME_OUTPUT_DIRECTORY "${TEST_DIR}"
)

set_target_properties(test_transcoder PROPERTIES 
  RUNTIME_OUTPUT_DIRECTORY "${TEST_DIR}"
)

# target_link_libraries(
#     some_audio_decode_convert_encode_sample -lavformat -lswscale -lavcodec -lavutil -lswresample
# )

# target_link_libraries(
#     print_pts -lavformat -lavcodec
# )

# target_link_libraries(
#     remuxing -lavformat -lavcodec  -lavutil
# )

# target_link_libraries(
#     transcoding -lavformat -lavcodec -lavutil -lavfilter 
# )

# target_link_libraries(
#     de  -lavformat -lswscale -lavcodec -lavutil -lswresample
# )

# target_link_libraries(
#     output-example -lavformat -lswscale -lavcodec -lavutil
# )

#  MAKING ffmpeg-tutorial0

# set(src_dir "ffmpeg-tutorial0/")

set(list_sources 
    # "tutorial01" 
    # "tutorial02" 
    # "tutorial03" 
    # "tutorial04" 
    # # "tutorial05" 
    # "tutorial06" 
    # "tutorial07" 
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
    # "sample01_scaning"
    # "sample02_demuxing"
    # "sample03_remuxing"
    # "sample04_decoding"
    # "sample05_filtering"
    # "sample06_encoding"
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

set(
    SOURCE_SWIG_INTERFACE
    "simple_ffmpeg"
)

set(
    CS_OUTDIR
    "cs"
)

set(
    PY_OUTDIR
    "py"
)


execute_process(
    COMMAND mkdir -p ${PY_OUTDIR}
    COMMAND touch ${PY_OUTDIR}/${SOURCE_SWIG_INTERFACE}_wrap.c
)


execute_process(
    COMMAND mkdir -p ${CS_OUTDIR} 
    COMMAND touch ${CS_OUTDIR}/${SOURCE_SWIG_INTERFACE}_wrap.c
)


add_custom_target(
    python_make_outdir
    mkdir -p ${PY_OUTDIR}
)

add_custom_target(
    python_generate_swig
    swig -python -o ${PY_OUTDIR}/${SOURCE_SWIG_INTERFACE}_wrap.c -outdir ${PY_OUTDIR} ${SOURCE_SWIG_INTERFACE}.i
)

add_library(
    _simple_ffmpeg SHARED
    ${PY_OUTDIR}/${SOURCE_SWIG_INTERFACE}_wrap.c ${SRC_DIR}/${SOURCE_GENERAL} ${SRC_DIR}/${SOURCE_TRANSCODE}
)

target_link_libraries(
    _simple_ffmpeg -lavcodec -lavformat -lavutil -lswresample -lswscale
)

set_target_properties(_simple_ffmpeg PROPERTIES 
  LIBRARY_OUTPUT_DIRECTORY "${PY_OUTDIR}"
)

set_target_properties(_simple_ffmpeg PROPERTIES PREFIX "")

add_custom_target(
    python_swig
)

add_dependencies(
    python_swig
    python_make_outdir
    python_generate_swig
    _simple_ffmpeg
)



add_custom_target(
    csharp_swig
)

add_custom_target(
    csharp_make_out_dir
    mkdir -p ${CS_OUTDIR}
)

add_custom_target(
    csharp_generate_swig
    swig -csharp  -outdir ${CS_OUTDIR} -o ${CS_OUTDIR}/${SOURCE_SWIG_INTERFACE}_wrap.c ${SOURCE_SWIG_INTERFACE}.i
)

add_library(
    simple_ffmpeg SHARED
    ${CS_OUTDIR}/${SOURCE_SWIG_INTERFACE}_wrap.c ${SRC_DIR}/${SOURCE_GENERAL} ${SRC_DIR}/${SOURCE_TRANSCODE}

)

target_link_libraries(
    simple_ffmpeg -lavcodec -lavformat -lavutil -lswresample -lswscale
)

set_target_properties(simple_ffmpeg PROPERTIES PREFIX "")

set_target_properties(simple_ffmpeg PROPERTIES 
  LIBRARY_OUTPUT_DIRECTORY "${CS_OUTDIR}"
)

add_dependencies(
    csharp_swig
    csharp_make_out_dir
    csharp_generate_swig
    simple_ffmpeg
)