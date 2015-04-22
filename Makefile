
MKDIR_P = mkdir -p

CLEANFILES =
PKGS = gio-2.0
CFLAGS = $(shell pkg-config --cflags $(PKGS)) -Wall -Werror -O0 -g
LDFLAGS = $(shell pkg-config --libs $(PKGS))

source_c_public = \
	src/epak.c \
	src/epak_entry.c \
	$(NULL)

source_h_public = \
	src/epak.h \
	src/epak_entry.h \
	$(NULL)

source_c_private = \
	src/epak_private.c \
	$(NULL)

source_h_private = \
	src/epak_private.h \
	$(NULL)

all: libepak.so Epak-1.0.typelib mkepak rdepak

mkepak: src/mkepak.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -L. -lepak
rdepak: src/rdepak.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -L. -lepak
CLEANFILES += mkepak rdepak src/mkepak.o src/rdepak.o

install: libepak.so Epak-1.0.typelib
	cp Epak-1.0.typelib /home/enoch/install/lib/girepository-1.0
	cp libepak.so /home/enoch/install/lib
	cp Epak-1.0.gir /home/enoch/install/share/gir-1.0

INTROSPECTION_GIRS = Epak-1.0.gir
INTROSPECTION_SCANNER_ARGS = --warn-all --warn-error --no-libtool --identifier-prefix=Epak

Epak-1.0.gir: libepak.so
Epak_1_0_gir_INCLUDES = GLib-2.0 Gio-2.0
Epak_1_0_gir_CFLAGS = $(CFLAGS)
Epak_1_0_gir_LIBS = epak
Epak_1_0_gir_FILES = $(source_c_public) $(source_h_public)
CLEANFILES += Epak-1.0.gir Epak-1.0.typelib

include Makefile.introspection

libepak.so: CFLAGS += -fPIC -shared
libepak.so: src/epak.o src/epak_private.o src/epak_entry.o src/adler32.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
CLEANFILES += libepak.so src/epak.o src/epak_private.o src/epak_entry.o src/adler32.o

clean:
	rm -f $(CLEANFILES)
