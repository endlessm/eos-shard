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

DISTCLEANFILES =
CLEANFILES =
EXTRA_DIST = debian README.md autogen.sh

AM_CFLAGS = $(LIBEOS_SHARD_CFLAGS)

INTROSPECTION_GIRS = EosShard-@SHARD_API_VERSION@.gir
INTROSPECTION_SCANNER_ARGS = --warn-all --warn-error

eos_shard_headers = \
	src/eos-shard-format-v1.h \
	src/eos-shard-shard-file.h \
	src/eos-shard-record.h \
	src/eos-shard-blob.h \
	src/eos-shard-blob-stream.h \
	src/eos-shard-writer.h \
	src/eos-shard-enums.h \
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

shardincludedir = $(includedir)/@SHARD_API_NAME@/@PACKAGE_NAME@
shardinclude_HEADERS = $(eos_shard_headers)

lib_LTLIBRARIES = libeos-shard-@SHARD_API_VERSION@.la
libeos_shard_@SHARD_API_VERSION@_la_CPPFLAGS = -Wall -Werror -I $(srcdir)
libeos_shard_@SHARD_API_VERSION@_la_SOURCES = \
	$(eos_shard_headers) \
	$(eos_shard_sources) \
	$(NULL)
libeos_shard_@SHARD_API_VERSION@_la_LIBADD = $(LIBEOS_SHARD_LIBS)
libeos_shard_@SHARD_API_VERSION@_la_LDFLAGS = \
	-version-info @SHARD_LT_VERSION@ \
	$(AM_LDFLAGS) \
	$(NULL)

# Note that the template file is called eos-shard.pc.in, but generates a
# versioned .pc file using some magic in AC_CONFIG_FILES, thanks to
# https://developer.gnome.org/programming-guidelines/unstable/parallel-installability.html.en#pkg-config
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = @SHARD_API_NAME@.pc

DISTCLEANFILES += $(pkgconfig_DATA)
EXTRA_DIST += eos-shard.pc.in

include $(INTROSPECTION_MAKEFILE)

EosShard-@SHARD_API_VERSION@.gir: libeos-shard-@SHARD_API_VERSION@.la
EosShard_@SHARD_API_VERSION@_gir_INCLUDES = GLib-2.0 Gio-2.0
EosShard_@SHARD_API_VERSION@_gir_CFLAGS = $(AM_CFLAGS)
EosShard_@SHARD_API_VERSION@_gir_LIBS = libeos-shard-@SHARD_API_VERSION@.la
EosShard_@SHARD_API_VERSION@_gir_FILES = \
	$(eos_shard_headers) \
	$(eos_shard_sources) \
	$(NULL)
EosShard_@SHARD_API_VERSION@_gir_SCANNERFLAGS = \
	--identifier-prefix=eos_shard_ \
	--identifier-prefix=EosShard \
	$(NULL)

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)
typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)
CLEANFILES += $(gir_DATA) $(typelib_DATA)

@EOS_COVERAGE_RULES@

if EOS_ENABLE_COVERAGE
AM_DISTCHECK_CONFIGURE_FLAGS = --enable-coverage --with-coverage-dir=@EOS_COVERAGE_DIR@
endif EOS_ENABLE_COVERAGE

JS_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) $(top_srcdir)/tap-driver.sh
JS_LOG_DRIVER_FLAGS = --comments

TESTS = \
	test/writerSpec.js \
	test/streamSpec.js \
	run_coverage.coverage \
	$(NULL)

EXTRA_DIST += $(TESTS) test/utils.js

EXTRA_DIST += \
	test/data/f572d396fae9206628714fb2ce00f72e94f2258f.json	\
	test/data/f572d396fae9206628714fb2ce00f72e94f2258f.blob	\
	test/data/7d97e98f8af710c7e7fe703abc8f639e0ee507c4.json	\
	test/data/7d97e98f8af710c7e7fe703abc8f639e0ee507c4.blob	\
	test/data/nul_example \
	$(NULL)


AM_CFLAGS += @EOS_C_COVERAGE_CFLAGS@
AM_LDFLAGS = @EOS_C_COVERAGE_LDFLAGS@

TEST_EXTENSIONS = .js .coverage
JS_LOG_COMPILER = jasmine
AM_JS_LOG_FLAGS = --tap --no-config
AM_TESTS_ENVIRONMENT = \
	export GI_TYPELIB_PATH="$(top_builddir)$${GI_TYPELIB_PATH:+:$$GI_TYPELIB_PATH}"; \
	export LD_LIBRARY_PATH="$(top_builddir)/.libs$${LD_LIBRARY_PATH:+:$$LD_LIBRARY_PATH}"; \
	export G_TEST_SRCDIR="$(top_srcdir)/"; \
	$(NULL)

clean-local: clean-coverage
