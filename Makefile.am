
CLEANFILES =

AM_CFLAGS = $(LIBEPAK_CFLAGS)

INTROSPECTION_GIRS = Epak-1.0.gir
INTROSPECTION_SCANNER_ARGS = --warn-all --warn-error

source_public = \
	src/epak-pak.c src/epak-pak.h \
	src/epak-entry.c src/epak-entry.h \
	src/epak-blob.c src/epak-blob.h \
	src/epak-blob-stream.c src/epak-blob-stream.h \
	src/epak-writer.c src/epak-writer.h \
	src/epak-format.h \
	src/epak-types.h \
	src/epak-utils.h \
	$(NULL)

lib_LTLIBRARIES = libepak.la
libepak_la_CPPFLAGS = -Wall -Werror -I $(srcdir)
libepak_la_SOURCES = $(source_public)
libepak_la_LIBADD = $(LIBEPAK_LIBS)

include $(INTROSPECTION_MAKEFILE)

Epak-1.0.gir: libepak.la
Epak_1_0_gir_INCLUDES = GLib-2.0 Gio-2.0
Epak_1_0_gir_CFLAGS = $(AM_CFLAGS)
Epak_1_0_gir_LIBS = libepak.la
Epak_1_0_gir_FILES = $(source_public)
Epak_1_0_gir_SCANNERFLAGS = --identifier-prefix=epak_ --identifier-prefix=Epak
CLEANFILES += Epak-1.0.gir Epak-1.0.typelib

EXTRA_DIST = debian

girdir = $(datadir)/gir-1.0
gir_DATA = Epak-1.0.gir
typelibdir = $(libdir)/girepository-1.0
typelib_DATA = Epak-1.0.typelib
