cmake_minimum_required(VERSION 3.0)

project("PrBoom-Plus" VERSION 2.5.1.7)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(TargetArch)

# Automated dependencies discovery, mostly needed for MSVC
target_architecture(TARGET_ARCH)
if(${TARGET_ARCH} MATCHES "i386")
    set(DEPENDENCY_SUFFIX x86)
elseif(${TARGET_ARCH} MATCHES "x86_64")
    set(DEPENDENCY_SUFFIX x64)
endif()
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../dependencies_${DEPENDENCY_SUFFIX}")

set(PACKAGE_NAME "${PROJECT_NAME}")
set(PACKAGE_TARNAME "prboom-plus")
set(PACKAGE_VERSION "${PROJECT_VERSION}")
set(PACKAGE_STRING "${PROJECT_NAME} ${PROJECT_VERSION}")

include(CheckSymbolExists)

check_symbol_exists(stricmp "string.h" HAVE_STRICMP)
if(NOT HAVE_STRICMP)
    add_definitions("-Dstricmp=strcasecmp")
endif()
check_symbol_exists(strnicmp "string.h" HAVE_STRNICMP)
if(NOT HAVE_STRNICMP)
    add_definitions("-Dstrnicmp=strncasecmp")
endif()

check_symbol_exists(getopt "unistd.h" HAVE_GETOPT)
check_symbol_exists(mmap "sys/mman.h" HAVE_MMAP)
if(NOT WIN32)
    set(CMAKE_REQUIRED_DEFINITIONS_PREV ${CMAKE_REQUIRED_DEFINITIONS})
    set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS} -D_GNU_SOURCE)
    check_symbol_exists(sched_setaffinity "sched.h" HAVE_SCHED_SETAFFINITY)
    set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS_PREV})
    if(HAVE_SCHED_SETAFFINITY)
        add_definitions(-D_GNU_SOURCE)
    endif()
endif()
check_symbol_exists(usleep "unistd.h" HAVE_USLEEP)

include(CheckIncludeFile)

check_include_file("sys/wait.h" HAVE_SYS_WAIT_H)
check_include_file("unistd.h" HAVE_UNISTD_H)
check_include_file("asm/byteorder.h" HAVE_ASM_BYTEORDER_H)

option(BUILD_GL "Enable OpenGL rendering code" ON)
if(BUILD_GL)
    find_package(OpenGL)
endif()

find_package(SDL2 REQUIRED)

option(WITH_IMAGE "Use SDL2_image if available" ON)
if(WITH_IMAGE)
    find_package(SDL2_image)
    if(SDL2_IMAGE_FOUND)
        set(HAVE_LIBSDL2_IMAGE TRUE)
    endif()
endif()

option(WITH_MIXER "Use SDL2_mixer if available" ON)
if(WITH_MIXER)
    find_package(SDL2_mixer)
    if(SDL2_MIXER_FOUND)
        set(HAVE_LIBSDL2_MIXER TRUE)
    endif()
endif()

option(WITH_NET "Use SDL2_net if available" ON)
if(WITH_NET)
    find_package(SDL2_net)
    if(SDL2_NET_FOUND)
        set(HAVE_NET TRUE)
        set(USE_SDL_NET TRUE)
    endif()
endif()

option(WITH_PCRE "Use PCRE if available" ON)
if(WITH_PCRE)
    find_package(PCREPOSIX)
    if(PCREPOSIX_FOUND)
        set(HAVE_LIBPCREPOSIX TRUE)
    endif()
endif()

option(WITH_ZLIB "Use ZLIB if available" ON)
if(WITH_ZLIB)
    find_package(ZLIB)
    if(ZLIB_FOUND)
        set(HAVE_LIBZ TRUE)
    endif()
endif()

option(WITH_MAD "Use libmad if available" ON)
if(WITH_MAD)
    find_package(LibMad)
    if(LIBMAD_FOUND)
        set(HAVE_LIBMAD TRUE)
    endif()
endif()

option(WITH_FLUIDSYNTH "Use FluidSynth if available" ON)
if(WITH_FLUIDSYNTH)
    find_package(FluidSynth)
    if(FLUIDSYNTH_FOUND)
        set(HAVE_LIBFLUIDSYNTH TRUE)
    endif()
endif()

option(WITH_DUMB "Use DUMB if available" ON)
if(WITH_DUMB)
    find_package(DUMB)
    if(DUMB_FOUND)
        set(HAVE_LIBDUMB TRUE)
    endif()
endif()

option(WITH_VORBISFILE "Use vorbisfile if available" ON)
if(WITH_VORBISFILE)
    find_package(Vorbis)
    if(VORBIS_FOUND)
        set(HAVE_LIBVORBISFILE TRUE)
    endif()
endif()

option(WITH_PORTMIDI "Use PortMidi if available" ON)
if(WITH_PORTMIDI)
    find_package(PortMidi)
    if(PortMidi_FOUND)
        set(HAVE_LIBPORTMIDI TRUE)
    endif()
endif()

set(CMAKE_REQUIRED_INCLUDES_PREV ${CMAKE_REQUIRED_INCLUDES})
set(CMAKE_REQUIRED_LIBRARIES_PREV ${CMAKE_REQUIRED_LIBRARIES})
set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${SDL2_INCLUDE_DIRS})
set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${SDL2_LIBRARIES})
check_symbol_exists(SDL_JoystickGetAxis "SDL.h" HAVE_SDL_JOYSTICKGETAXIS)
set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES_PREV})
set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_PREV})

set(DOOMWADDIR "${CMAKE_INSTALL_PREFIX}/share/games/doom" CACHE PATH "Path to install prboom.wad and look for other WAD files")

option(SIMPLECHECKS "Enable checks which only impose significant overhead if a posible error is detected" ON)
option(ZONEIDCHECK "Enable id checks on zone blocks, to detect corrupted and illegally freed blocks" ON)

# Debug options, disabled by default
option(RANGECHECK "Enable internal range checking" OFF)
option(INSTRUMENTED "Enable real-time memory allocation statistics, and extra debugging features" OFF)
option(TIMEDIAG "Enable creation of time stamps each time a lump is locked, and reporting of lumps locked for long periods of time" OFF)
option(HEAPCHECK "Turn on continuous heap checking (very slow)" OFF)
option(HEAPDUMP "Turn on dumping the heap state for debugging" OFF)

configure_file(cmake/config.h.cin config.h)

add_definitions(-DHAVE_CONFIG_H)

if (MSVC)
    add_definitions("/D_CRT_SECURE_NO_WARNINGS")
else()
    set(CMAKE_C_FLAGS
        ${CMAKE_C_FLAGS} "\
        -Wall -Wextra -Wno-missing-field-initializers -Wwrite-strings -Wundef \
        -Wbad-function-cast -Wcast-align -Wcast-qual -Wdeclaration-after-statement \
        -Wpointer-arith -Wno-unused -Wno-switch -Wno-sign-compare -Wno-pointer-sign \
        -ffast-math"
    )
endif()

set(PRBOOM_OUTPUT_PATH ${CMAKE_BINARY_DIR})

set(WAD_DATA prboom-plus.wad)
set(WAD_DATA_PATH "${PRBOOM_OUTPUT_PATH}/${WAD_DATA}")

add_subdirectory(data)
add_subdirectory(src)
