AC_INIT([conferenceroom], [1.0])

AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign])
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL

AC_CHECK_PROGS([RC],[${ac_tool_prefix}windres])

AC_PATH_PROG([PKG_CONFIG], [pkg-config])

PKG_CHECK_MODULES_STATIC(CONFERENCEROOM, simple-connection libjpeg libvncserver)

ADDONS_LIBS=""

CONFERENCEROOM_CFLAGS="$CONFERENCEROOM_CFLAGS -DCONFIG_DIR=\\\"${sysconfdir}/controlserver\\\" -DWEBROOT_DIR=\\\"${datadir}/controlserver/wwwroot\\\""

LIBVNCSERVER_LIBDIR=$(pkg-config libvncserver --variable=prefix)/lib
LIBVNCSERVER_VERSION=$(pkg-config libvncserver --modversion)

AC_SUBST(LIBTOOL_DEPS)
AC_SUBST([CONFERENCEROOM_CFLAGS])
AC_SUBST([CONFERENCEROOM_LIBS])
AC_SUBST([LIBVNCSERVER_LIBDIR])
AC_SUBST([LIBVNCSERVER_VERSION])
AC_LTDL_DLLIB
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
