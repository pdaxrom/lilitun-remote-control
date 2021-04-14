cmake_minimum_required(VERSION 3.1)
project (sharescreen)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckIncludeFile)
include(FindPkgConfig)
include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(GNUInstallDirs)

set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH 1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake" "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Modules/")
option(WITH_GUI "Enable SDL GUI for projectors" OFF)
option(WITH_THREADS "Search for a threading library to build with multithreading support" ON)
option(PREFER_WIN32THREADS "When searching for a threading library, prefer win32 threads if they are found" OFF)
option(WITH_DEBUG "With optimization" OFF)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/simple-connection-lib/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wpedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -Wall -Wpedantic")

if(WITH_DEBUG)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -O0 -g")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O0 -g")
endif(WITH_DEBUG)

check_include_file("xcb/xcb.h"    SHARESCREEN_HAVE_XCB_XCB_H)
if(SHARESCREEN_HAVE_XCB_XCB_H)
  set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} -lxcb)
endif(SHARESCREEN_HAVE_XCB_XCB_H)

check_include_file("xcb/shm.h"    SHARESCREEN_HAVE_XCB_SHM_H)
if(SHARESCREEN_HAVE_XCB_SHM_H)
  set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} -lxcb-shm)
endif(SHARESCREEN_HAVE_XCB_SHM_H)

check_include_file("xcb/xtest.h"    SHARESCREEN_HAVE_XCB_XTEST_H)
if(SHARESCREEN_HAVE_XCB_XTEST_H)
  set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} -lxcb-xtest)
endif(SHARESCREEN_HAVE_XCB_XTEST_H)

check_include_file("xcb/xcb_keysyms.h"    SHARESCREEN_HAVE_XCB_KEYSYMS_H)
if(SHARESCREEN_HAVE_XCB_KEYSYMS_H)
  set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} -lxcb-keysyms)
endif(SHARESCREEN_HAVE_XCB_KEYSYMS_H)

find_package(OpenSSL)
if(OPENSSL_FOUND)
  include_directories(${OPENSSL_INCLUDE_DIR})
  set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} ${OPENSSL_LIBRARIES})
endif(OPENSSL_FOUND)

find_package(JPEG)
if(JPEG_FOUND)
  # Check whether the version of libjpeg we found was libjpeg-turbo and print a
  # warning if not.
  set(CMAKE_REQUIRED_LIBRARIES ${JPEG_LIBRARIES})
  if(JPEG_INCLUDE_DIRS) # this was not present in 3.4
    set(CMAKE_REQUIRED_INCLUDES ${JPEG_INCLUDE_DIRS})
  else()
    set(CMAKE_REQUIRED_INCLUDES ${JPEG_INCLUDE_DIR})
  endif()

  set(JPEG_TEST_SOURCE "\n
  #include <stdio.h>\n
  #include <jpeglib.h>\n
  int main(void) {\n
    struct jpeg_compress_struct cinfo;\n
    struct jpeg_error_mgr jerr;\n
    cinfo.err=jpeg_std_error(&jerr);\n
    jpeg_create_compress(&cinfo);\n
    cinfo.input_components = 3;\n
    jpeg_set_defaults(&cinfo);\n
    cinfo.in_color_space = JCS_EXT_RGB;\n
    jpeg_default_colorspace(&cinfo);\n
    return 0;\n
  }")

  if(CMAKE_CROSSCOMPILING)
    check_c_source_compiles("${JPEG_TEST_SOURCE}" FOUND_LIBJPEG_TURBO)
  else()
    check_c_source_runs("${JPEG_TEST_SOURCE}" FOUND_LIBJPEG_TURBO)
  endif()

  set(CMAKE_REQUIRED_LIBRARIES)
  set(CMAKE_REQUIRED_INCLUDES)
  set(CMAKE_REQUIRED_DEFINITIONS)

  if(NOT FOUND_LIBJPEG_TURBO)
    message(WARNING "*** The libjpeg library you are building against is not libjpeg-turbo.  Performance will be reduced.  You can obtain libjpeg-turbo from:  https://sourceforge.net/projects/libjpeg-turbo/files/ ***")
  endif()
  include_directories(${JPEG_INCLUDE_DIR})
  set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} ${JPEG_LIBRARIES})
endif(JPEG_FOUND)

#pkg_check_modules(LIBVNCSERVER REQUIRED libvncserver)
#include_directories(${LIBVNCSERVER_INCLUDE_DIRS})

find_package(SDL2)
if (SDL2_FOUND)
  if(WITH_GUI)
    add_definitions(-DSERVER_GUI)
    include_directories(${SDL2_INCLUDE_DIR})
    set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} ${SDL2_LIBRARY})
  endif(WITH_GUI)
endif(SDL2_FOUND)

pkg_check_modules(LIBVNCSERVER REQUIRED libvncserver)
include_directories(${LIBVNCSERVER_INCLUDE_DIRS})

if(WITH_THREADS)
  find_package(Threads)
endif(WITH_THREADS)

if(WITH_THREADS AND Threads_FOUND)
  set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} ${CMAKE_THREAD_LIBS_INIT})
endif(WITH_THREADS AND Threads_FOUND)

# Make sure that only one threading system is used. This happens on MinGW.
if(WITH_THREADS)
  if(CMAKE_USE_PTHREADS_INIT AND CMAKE_USE_WIN32_THREADS_INIT)
    if(PREFER_WIN32THREADS)
      unset(CMAKE_USE_PTHREADS_INIT)
    else()
      unset(CMAKE_USE_WIN32_THREADS_INIT)
    endif(PREFER_WIN32THREADS)
  endif(CMAKE_USE_PTHREADS_INIT AND CMAKE_USE_WIN32_THREADS_INIT)

  if(CMAKE_USE_PTHREADS_INIT)
    set(LIBVNCSERVER_HAVE_LIBPTHREAD 1)
    message(STATUS "Threads support is using pthreads")
  endif(CMAKE_USE_PTHREADS_INIT)

  if(CMAKE_USE_WIN32_THREADS_INIT)
    set(LIBVNCSERVER_HAVE_WIN32THREADS 1)
    message(STATUS "Threads support is using win32 threads")
  endif(CMAKE_USE_WIN32_THREADS_INIT)
endif(WITH_THREADS)

add_definitions(-DCONFIG_DIR=\"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_SYSCONFDIR}/controlserver\")
add_definitions(-DWEBROOT_DIR=\"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/controlserver/webclients\")

add_executable(sharescreen main.c jpegdec.c fbops.c translator.c list.c portctrl.c base64.c getrandom.c ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/simple-connection-lib/src/tcp.c)
target_link_libraries(sharescreen ${LIBVNCSERVER_LDFLAGS} ${ADDITIONAL_LIBS})

install(TARGETS sharescreen DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES cert/server.pem DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/controlserver)
install(DIRECTORY ../thirdparty/libvncserver/webclients DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/controlserver)
