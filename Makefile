
CFLAGS = -Wall -Werror

all: mkepak rdepak

mkepak: epak.o adler32.o mkepak.o
rdepak: epak.o adler32.o rdepak.o
