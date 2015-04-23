
CLEANFILES =

noinst_PROGRAMS = mkepak rdepak

AM_CFLAGS = $(LIBEPAK_CFLAGS)

mkepak_SOURCES = src/mkepak.c
mkepak_LDADD = libepak.la $(LIBEPAK_LIBS)

rdepak_SOURCES = src/rdepak.c
rdepak_LDADD = libepak.la $(LIBEPAK_LIBS)

INTROSPECTION_GIRS = Epak-1.0.gir
INTROSPECTION_SCANNER_ARGS = --warn-all --warn-error

source_public = \
	src/epak.c src/epak.h \
	src/epak_entry.c src/epak_entry.h \
	src/epak_types.h \
	$(NULL)

source_private = \
	src/epak_private.c src/epak_private.h \
	src/epak_fmt.h \
	src/adler32.c src/adler32.h \
	$(NULL)

lib_LTLIBRARIES = libepak.la
libepak_la_CPPFLAGS = -I $(srcdir)
libepak_la_SOURCES = $(source_public) $(source_private)
libepak_la_LIBADD = $(LIBEPAK_LIBS)

include $(INTROSPECTION_MAKEFILE)

Epak-1.0.gir: libepak.la
Epak_1_0_gir_INCLUDES = GLib-2.0 Gio-2.0
Epak_1_0_gir_CFLAGS = $(AM_CFLAGS)
Epak_1_0_gir_LIBS = libepak.la
Epak_1_0_gir_FILES = $(source_public)
CLEANFILES += Epak-1.0.gir Epak-1.0.typelib

girdir = $(datadir)/gir-1.0
gir_DATA = Epak-1.0.gir
typelibdir = $(libdir)/girepository-1.0
typelib_DATA = Epak-1.0.typelib
