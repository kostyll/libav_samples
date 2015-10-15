cmake_minimum_required(VERSION 2.8.1)

#successfuly compiled in SHARED ENVIRONMENT 
#cmake . -DCMAKE_TOOLCHAIN_FILE=save_sss/mxe/usr/i686-w64-mingw32.shared/share/cmake/mxe-conf.cmake

project (libav_samples)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -ggdb -O0 ")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w  -ggdb -O0 ")


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "./")
find_package(FFMPEG REQUIRED)
find_package(PythonLibs)

include_directories(
    ./
    ./include
    ${FFMPEG_INCLUDE_DIR}
)

if(UNIX)
    include_directories(target_usr/include)
    link_directories(target_usr/lib)
endif(UNIX)


link_directories(
    ${FFMPEG_LIBRARIES}
)

if (PYTHONLIBS_FOUND)
    include_directories(${PYTHON_INCLUDE_DIRS})
    link_directories(${PYTHON_LIBRARIES})
endif(PYTHONLIBS_FOUND)


set(SOURCE_VIDEO_CONVERT "test_video_convert.c")

set(SOURCE_GENERAL "general.c")

set(SOURCE_TRANSCODE "transcode.c")

set(SOURCE_LOSELESS_OGG_FLAC "test_ogg_flac.c")

set(SOURCE_TEST_TRANSCODER "test_transcoder.c")

set(SRC_DIR "src")

set(TEST_DIR "test")

add_executable(
    test_video_convert
    ${TEST_DIR}/${SOURCE_VIDEO_CONVERT}
    ${SRC_DIR}/${SOURCE_GENERAL}
)

add_executable(
    test_transcoder
    ${TEST_DIR}/${SOURCE_TEST_TRANSCODER}
    ${SRC_DIR}/${SOURCE_GENERAL}
    ${SRC_DIR}/${SOURCE_TRANSCODE}
)

target_link_libraries(
    test_video_convert 
    ${FFMPEG_LIBRARIES} swresample swscale 
)

target_link_libraries(
    test_transcoder
    ${FFMPEG_LIBRARIES} swresample swscale 
)

set_target_properties(test_video_convert PROPERTIES 
  RUNTIME_OUTPUT_DIRECTORY "${TEST_DIR}"
)

set_target_properties(test_transcoder PROPERTIES 
  RUNTIME_OUTPUT_DIRECTORY "${TEST_DIR}"
)

set(SOURCE_SWIG_INTERFACE "simple_ffmpeg")

set(CS_OUTDIR "cs")

set(PY_OUTDIR "py")


execute_process(
    COMMAND mkdir -p ${PY_OUTDIR}
    COMMAND touch ${PY_OUTDIR}/${SOURCE_SWIG_INTERFACE}_wrap.c
)


execute_process(
    COMMAND mkdir -p ${CS_OUTDIR} 
    COMMAND touch ${CS_OUTDIR}/${SOURCE_SWIG_INTERFACE}_wrap.c
)

add_library(
    sff SHARED
    ${SRC_DIR}/${SOURCE_GENERAL} ${SRC_DIR}/${SOURCE_TRANSCODE}
)

target_link_libraries(sff ${FFMPEG_LIBRARIES} swresample swscale )

if (PYTHONLIBS_FOUND)
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
        _simple_ffmpeg
        ${FFMPEG_LIBRARIES} swresample swscale 
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

    target_link_libraries(
        _simple_ffmpeg
        ${FFMPEG_LIBRARIES} swresample swscale 
    )

endif(PYTHONLIBS_FOUND)


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
    simple_ffmpeg
    ${FFMPEG_LIBRARIES} swresample swscale 
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