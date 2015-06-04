## Copyright 2015 Endless Mobile, Inc.

## This file is part of eos-shard.
##
## eos-shard is free software: you can redistribute it and/or modify
## it under the terms of the GNU Lesser General Public License as published
## by the Free Software Foundation, either version 2.1 of the License, or
## (at your option) any later version.
##
## eos-shard is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public
## License along with eos-shard.  If not, see
## <http://www.gnu.org/licenses/>.

CLEANFILES =

AM_CFLAGS = $(LIBEOS_SHARD_CFLAGS)

INTROSPECTION_GIRS = EosShard-0.gir
INTROSPECTION_SCANNER_ARGS = --warn-all --warn-error

eos_shard_headers = \
	src/eos-shard-shard-file.h \
	src/eos-shard-record.h \
	src/eos-shard-blob.h \
	src/eos-shard-blob-stream.h \
	src/eos-shard-writer.h \
	src/eos-shard-enums.h \
	src/eos-shard-format.h \
	src/eos-shard-types.h \
	$(NULL)

eos_shard_sources = \
	src/eos-shard-shard-file.c \
	src/eos-shard-record.c \
	src/eos-shard-blob.c \
	src/eos-shard-blob-stream.c \
	src/eos-shard-writer.c \
	src/eos-shard-enums.c \
	$(NULL)

pkginclude_HEADERS = $(eos_shard_headers)

lib_LTLIBRARIES = libeos-shard.la
libeos_shard_la_CPPFLAGS = -Wall -Werror -I $(srcdir)
libeos_shard_la_SOURCES = $(eos_shard_headers) $(eos_shard_sources)
libeos_shard_la_LIBADD = $(LIBEOS_SHARD_LIBS)

include $(INTROSPECTION_MAKEFILE)

EosShard-0.gir: libeos-shard.la
EosShard_0_gir_INCLUDES = GLib-2.0 Gio-2.0
EosShard_0_gir_CFLAGS = $(AM_CFLAGS)
EosShard_0_gir_LIBS = libeos-shard.la
EosShard_0_gir_FILES = $(eos_shard_headers) $(eos_shard_sources)
EosShard_0_gir_SCANNERFLAGS = --identifier-prefix=eos_shard_ --identifier-prefix=EosShard
CLEANFILES += EosShard-0.gir EosShard-0.typelib

EXTRA_DIST = debian README.md

girdir = $(datadir)/gir-1.0
gir_DATA = EosShard-0.gir
typelibdir = $(libdir)/girepository-1.0
typelib_DATA = EosShard-0.typelib

JS_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) $(top_srcdir)/tap-driver.sh
JS_LOG_DRIVER_FLAGS = --comments

TESTS = test/tests.js
EXTRA_DIST += $(TESTS)

EXTRA_DIST += \
	test/data/f572d396fae9206628714fb2ce00f72e94f2258f.json	\
	test/data/f572d396fae9206628714fb2ce00f72e94f2258f.blob	\
	test/data/7d97e98f8af710c7e7fe703abc8f639e0ee507c4.json	\
	test/data/7d97e98f8af710c7e7fe703abc8f639e0ee507c4.blob	\
	test/data/nul_example					\
	$(NULL)

TEST_EXTENSIONS = .js
JS_LOG_COMPILER = jasmine
AM_JS_LOG_FLAGS = --tap --no-config
AM_TESTS_ENVIRONMENT = \
	export GI_TYPELIB_PATH="$(top_builddir)$${GI_TYPELIB_PATH:+:$$GI_TYPELIB_PATH}"; \
	export LD_LIBRARY_PATH="$(top_builddir)/.libs$${LD_LIBRARY_PATH:+:$$LD_LIBRARY_PATH}"; \
	export G_TEST_SRCDIR="$(top_srcdir)/"; \
	$(NULL)
