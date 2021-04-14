AC_INIT([projector], [1.0])

AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign])
AC_PROG_CC
AC_PROG_CXX
AC_PROG_OBJC
AC_PROG_LIBTOOL

AC_CHECK_PROGS([RC],[${ac_tool_prefix}windres])

AC_PATH_PROG([PKG_CONFIG], [pkg-config])

PKG_CHECK_MODULES_STATIC(PROJECTOR, simple-connection libjpeg)

PKG_CHECK_MODULES_STATIC(LIBJPEG, libjpeg)

FLTK_INCLUDE="$(${FLTK_DIR}fltk-config --use-images --cxxflags)"
FLTK_LIB="$(${FLTK_DIR}fltk-config --use-images --ldstaticflags)"

if test "$FLTK_LIB" = ""; then
    AC_MSG_ERROR([fltk-config not found])
fi

ADDONS_LIBS=""

case $host_os in
    cygwin*|mingw*)
        GRABBER_SRC=wingrabber.cxx
        build_for="windows"
        ADDONS_LIBS="-lpng -lzlibstatic -ld3d9 -pthread -all-static"
    ;;
    darwin*)
        bindir=${prefix}/LiliTun-remote-control.app/Contents/MacOS
        GRABBER_SRC=macgrabber.c
        build_for="macos"
        ADDONS_LIBS="-framework IOSurface -framework Carbon"
    ;;
    *)
        PKG_CHECK_MODULES(GRABBER, xcb xcb-shm xcb-xtest xcb-keysyms)
        GRABBER_SRC=xcbgrabber.c
        build_for="linux"
    ;;
esac

PROJECTOR_INCLUDES="$PROJECTOR_INCLUDES $FLTK_INCLUDE $LIBJPEG_INCLUDES"
PROJECTOR_LIBS="$PROJECTOR_LIBS $FLTK_LIB $LIBJPEG_LIBS $ADDONS_LIBS"

SYSTEM_ARCH=$host_cpu

AM_CONDITIONAL([TARGET_LINUX], [test "$build_for" = "linux"])
AM_CONDITIONAL([TARGET_WINDOWS], [test "$build_for" = "windows"])
AM_CONDITIONAL([TARGET_MACOS], [test "$build_for" = "macos"])
AC_SUBST(LIBTOOL_DEPS)
AC_SUBST([GRABBER_SRC])
AC_SUBST([PROJECTOR_INCLUDES])
AC_SUBST([PROJECTOR_LIBS])
AC_SUBST([GRABBER_INCLUDES])
AC_SUBST([GRABBER_LIBS])
AC_SUBST([SYSTEM_ARCH])
AC_LTDL_DLLIB
AC_CONFIG_FILES([Makefile
                osx/Info.plist
                linux/lilitun-remote-control.desktop])
AC_OUTPUT
