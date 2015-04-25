
AC_INIT([libepak], [0],
    [], [libepak], [http://endlessm.com])
AM_INIT_AUTOMAKE([-Wall -Wno-portability foreign 1.11 tar-ustar no-dist-gzip dist-xz subdir-objects])
AM_SILENT_RULES([yes])

PKG_PROG_PKG_CONFIG

AC_PROG_MKDIR_P
AC_CACHE_SAVE

LT_INIT

PKG_CHECK_MODULES([LIBEPAK], [gio-2.0 zlib])

GOBJECT_INTROSPECTION_REQUIRE([1.30])

AC_CONFIG_FILES([
    Makefile
])
AC_OUTPUT
