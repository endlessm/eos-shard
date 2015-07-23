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

dnl Version numbers
dnl ---------------
dnl API name and version for pkg-config; increment when breaking API/ABI.
dnl This version number is also the major version number.
m4_define([_SHARD_API_VERSION_MACRO], [0])
dnl Before making a release, the three macros below should be modified.
dnl Minor and micro versions: increment micro version when making a release.
dnl Minor version is even for a stable release and odd for a development release.
dnl When making any release, if the API changes, set the interface age to 0.
m4_define([_SHARD_MINOR_VERSION_MACRO], [0])
m4_define([_SHARD_MICRO_VERSION_MACRO], [0])
m4_define([_SHARD_INTERFACE_AGE_MACRO], [0])
dnl Full version, for use in AC_INIT
m4_define([_SHARD_VERSION_MACRO],
    [_SHARD_API_VERSION_MACRO._SHARD_MINOR_VERSION_MACRO._SHARD_MICRO_VERSION_MACRO])

AC_INIT([eos-shard], [_SHARD_VERSION_MACRO],
    [], [eos-shard], [http://endlessm.com])
AM_INIT_AUTOMAKE([-Wall -Wno-portability foreign 1.11 tar-ustar no-dist-gzip dist-xz subdir-objects])
AM_SILENT_RULES([yes])

# Variables to define
# -------------------
# Version numbers and associated macros
AC_SUBST([SHARD_API_VERSION], [_SHARD_API_VERSION_MACRO])
AC_DEFINE([SHARD_MAJOR_VERSION], [_SHARD_API_VERSION_MACRO], [Major (API) version])
AC_SUBST([SHARD_API_NAME], ["eos-shard-$SHARD_API_VERSION"])
AC_DEFINE([SHARD_MINOR_VERSION], [_SHARD_MINOR_VERSION_MACRO], [Minor version])
AC_DEFINE([SHARD_MICRO_VERSION], [_SHARD_MICRO_VERSION_MACRO], [Micro version])

# Shared library versioning; calculated automatically.
# See http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
SHARD_LT_CURRENT=m4_eval(100 * _SHARD_MINOR_VERSION_MACRO + _SHARD_MICRO_VERSION_MACRO - _SHARD_INTERFACE_AGE_MACRO)
SHARD_LT_REVISION=_SHARD_INTERFACE_AGE_MACRO
SHARD_LT_AGE=$SHARD_LT_CURRENT
AC_SUBST([SHARD_LT_VERSION], ["$SHARD_LT_CURRENT:$SHARD_LT_REVISION:$SHARD_LT_AGE"])

PKG_PROG_PKG_CONFIG

AC_PROG_MKDIR_P
AC_CACHE_SAVE

AC_PROG_AWK
AC_REQUIRE_AUX_FILE([tap-driver.sh])

AC_SYS_LARGEFILE

LT_INIT

AC_SUBST([SHARD_REQUIRED_MODULES_PUBLIC], [gio-2.0])
PKG_CHECK_MODULES([LIBEOS_SHARD], [
    $SHARD_REQUIRED_MODULES_PUBLIC
])

GOBJECT_INTROSPECTION_REQUIRE([1.30])

AC_CONFIG_FILES([
    eos-shard-$SHARD_API_VERSION.pc:eos-shard.pc.in
    Makefile
], [], [SHARD_API_VERSION='$SHARD_API_VERSION'])
AC_OUTPUT
