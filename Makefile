
MKDIR_P = mkdir -p

CLEANFILES =
PKGS = gio-2.0
CFLAGS = $(shell pkg-config --cflags $(PKGS)) -Wall -Werror
LDFLAGS = $(shell pkg-config --libs $(PKGS))

FILES = src/epak.c src/epak.h

all: libepak.so Epak-1.0.typelib

INTROSPECTION_GIRS = Epak-1.0.gir
INTROSPECTION_SCANNER_ARGS = --warn-all --warn-error --no-libtool

Epak-1.0.gir: libepak.so
Epak_1_0_gir_INCLUDES = GLib-2.0
Epak_1_0_gir_CFLAGS = $(CFLAGS)
Epak_1_0_gir_LIBS = epak
Epak_1_0_gir_FILES = $(FILES)
CLEANFILES += Epak-1.0.gir Epak-1.0.typelib

include Makefile.introspection

libepak.so: CFLAGS += -fPIC -shared
libepak.so: src/epak.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
CLEANFILES += libepak.so src/epak.o

clean:
	rm -f $(CLEANFILES)
