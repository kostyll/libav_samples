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

set(SOURCE_OUTPUT_EXAMPLE "output-example.c")

add_executable(simple_ffmpeg_player ${SOURCE_SIMPLE_FFMPEG_PLAYER})

# add_executable(output-example ${SOURCE_OUTPUT_EXAMPLE})

target_link_libraries(
    simple_ffmpeg_player -lSDL -lm -ldl -lrt -lpthread -lz -lavformat -lswscale -lavcodec -lavutil
)

# target_link_libraries(
#     output-example -lavformat -lswscale -lavcodec -lavutil
# )