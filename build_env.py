#!/usr/bin/env python
import os
import clime
import sh

"""


MXE experiments: https://github.com/mxe/mxe
make MXE_TARGETS='i686-w64-mingw32.shared i686-w64-mingw32.static' qt ffmpeg
need to add http://sourceforge.net/projects/dirac/ dirac codec

env_shared.sh
#!/bin/bash

export PATH=/cross/mxe/usr/bin:$PATH
export PKG_CONFIG_PATH_i686_w64_mingw32_shared=/cross/mxe/usr/i686-w64-mingw32.shared
#cmake -DCMAKE_TOOLCHAIN_FILE=/cross/mxe/usr/i686-w64-mingw32.shared/share/cmake/mxe-conf.cmake .
bash

env_static.sh
#!/bin/bash

export PATH=/cross/mxe/usr/bin:$PATH
export PKG_CONFIG_PATH_i686_w64_mingw32_static=/cross/mxe/usr/i686-w64-mingw32.static
#cmake -DCMAKE_TOOLCHAIN_FILE=/cross/mxe/usr/i686-w64-mingw32.static/static/cmake/mxe-conf.cmake .
bash

"""

WIN32_STATIC = "i686-w64-mingw32.static"
WIN32_SHARED = "i686-w64-mingw32.shared"

CROSS = '/cross'
CROSS_MXE = None

SEARCH_PATHS = [
    CROSS,
    os.environ['HOME'],
]


def get_MXE(paths=None):
    if paths is None:
        paths = SEARCH_PATHS
    for path in paths:
        abspath = os.path.abspath(path)
        if os.path.isdir(os.path.join(abspath, "mxe")):
            return os.path.join(abspath, "mxe")

def test_cmd():
    print "Test!"


def build_win32_mxe(case=None, libs=None):
    ret = os.chdir('/cross/mxe')
    if ret < 0:
        print "/cross dirrectory was not created!" 

    if libs is None:
        libs = []
    cmd = "make MXE_TARGETS='%s' %s" % (
        " ".join(case) if case else "i686-w64-mingw32.static",
        " ".join(libs)
    )
    return os.system(cmd)


def build_win32_mxe_static_cmd(libs=None):
    return build_win32_mxe(case="static", libs=libs)


def build_win32_mxe_shared_cmd(libs=None):
    return build_win32_mxe(case="shared", libs=libs)


def build_win32_mxe_cmd(libs=None):
    return build_win32_mxe(libs=libs)


def enter_environment(case=None):
    if case is None:
        case = "static"
    os.environ['MXE_MODE'] = case
    os.environ['PATH'] = os.path.join(CROSS_MXE, "usr", "bin")+":"+os.environ['PATH']
    PKG_CONFIG = os.path.join(CROSS_MXE, "usr", "i686-w64-mingw32.%s" % case)
    ENV_NAME = "PKG_CONFIG_PATH_i686_w64_mingw32_%s" % case
    os.environ[ENV_NAME] = PKG_CONFIG
    os.system("bash")


def enter_env_static_cmd():
    return enter_environment(case="static")


def enter_env_shared_cmd():
    return enter_environment(case="shared")


def build_cmake_project(case=None):
    if case is None:
        if os.environ['MXE_MODE'] == "":
            case = 'static'
        else:
            case = os.environ['MXE_MODE']
    cmd = "cmake -DCMAKE_TOOLCHAIN_FILE=%s/usr/i686-w64-mingw32.%s/share/cmake/mxe-conf.cmake ." % (CROSS_MXE, case)
    print "Running '%s'" % cmd
    return os.system(cmd)


def build_cmake_static_cmd():
    return build_cmake_project(case='static')


def build_cmake_shared_cmd():
    return build_cmake_project(case='shared')


def run_clean_rm_cmd():
    try:
        p = sh.make('clean')
        print "make clean was done"
    except sh.ErrorReturnCode, e:
        print e
    sh.rm(
        '-rf',
        'CMakeFiles/',
        'CMakeCache.txt',
        'Makefile'
    )
    print "removed cache..."


if __name__ == '__main__':
    import clime
    global CROSS_MXE
    CROSS_MXE = get_MXE()
    print "mxe was found at %s" % CROSS_MXE
    clime.start(white_pattern=clime.CMD_SUFFIX)
