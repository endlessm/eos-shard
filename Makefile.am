## Copyright 2015 Endless Mobile, Inc.

## This file is part of epak.
##
## epak is free software: you can redistribute it and/or modify
## it under the terms of the GNU Lesser General Public License as published
## by the Free Software Foundation, either version 2.1 of the License, or
## (at your option) any later version.
##
## epak is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public
## License along with epak.  If not, see
## <http://www.gnu.org/licenses/>.

CLEANFILES =

AM_CFLAGS = $(LIBEPAK_CFLAGS)

INTROSPECTION_GIRS = Epak-0.gir
INTROSPECTION_SCANNER_ARGS = --warn-all --warn-error

epak_headers = \
	src/epak-pak.h \
	src/epak-record.h \
	src/epak-enums.h \
	src/epak-blob.h \
	src/epak-blob-stream.h \
	src/epak-writer.h \
	src/epak-format.h \
	src/epak-types.h \
	src/epak-utils.h \
	$(NULL)

epak_sources = \
	src/epak-pak.c \
	src/epak-record.c \
	src/epak-enums.c \
	src/epak-blob.c \
	src/epak-blob-stream.c \
	src/epak-writer.c \
	$(NULL)

pkginclude_HEADERS = $(epak_headers)

lib_LTLIBRARIES = libepak.la
libepak_la_CPPFLAGS = -Wall -Werror -I $(srcdir)
libepak_la_SOURCES = $(epak_headers) $(epak_sources)
libepak_la_LIBADD = $(LIBEPAK_LIBS)

include $(INTROSPECTION_MAKEFILE)

Epak-0.gir: libepak.la
Epak_0_gir_INCLUDES = GLib-2.0 Gio-2.0
Epak_0_gir_CFLAGS = $(AM_CFLAGS)
Epak_0_gir_LIBS = libepak.la
Epak_0_gir_FILES = $(epak_headers) $(epak_sources)
Epak_0_gir_SCANNERFLAGS = --identifier-prefix=epak_ --identifier-prefix=Epak
CLEANFILES += Epak-0.gir Epak-0.typelib

EXTRA_DIST = debian README.md

girdir = $(datadir)/gir-1.0
gir_DATA = Epak-0.gir
typelibdir = $(libdir)/girepository-1.0
typelib_DATA = Epak-0.typelib
