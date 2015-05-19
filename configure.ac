dnl Copyright 2015 Endless Mobile, Inc.

dnl This file is part of eos-shard.
dnl
dnl eos-shard is free software: you can redistribute it and/or modify
dnl it under the terms of the GNU Lesser General Public License as published
dnl by the Free Software Foundation, either version 2.1 of the License, or
dnl (at your option) any later version.
dnl
dnl eos-shard is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU Lesser General Public License for more details.
dnl
dnl You should have received a copy of the GNU Lesser General Public
dnl License along with eos-shard.  If not, see
dnl <http://www.gnu.org/licenses/>.

AC_INIT([eos-shard], [0],
    [], [eos-shard], [http://endlessm.com])
AM_INIT_AUTOMAKE([-Wall -Wno-portability foreign 1.11 tar-ustar no-dist-gzip dist-xz subdir-objects])
AM_SILENT_RULES([yes])

PKG_PROG_PKG_CONFIG

AC_PROG_MKDIR_P
AC_CACHE_SAVE

LT_INIT

PKG_CHECK_MODULES([LIBEOS_SHARD], [gio-2.0 zlib])

GOBJECT_INTROSPECTION_REQUIRE([1.30])

AC_CONFIG_FILES([
    Makefile
])
AC_OUTPUT
