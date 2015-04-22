
#define _GNU_SOURCE

#include "epak_private.h"

#include <fcntl.h>
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static char * extdirname(char *epakname)
{
    return strtok(epakname, ".");
}

static void extname(char *fn, char *hash)
{
    int i;
    for (i = 0; i < 20; i++)
        sprintf(&fn[i*2], "%02hhx", hash[i]);
}

int main(int argc, char *argv[])
{
    struct epak_t *pak;
    char *epakname = argv[1];
    int fd = open(epakname, O_RDONLY);
    char *dir = extdirname(epakname);
    mkdir(dir, 0777);
    int dirfd = open(dir, O_RDONLY | O_DIRECTORY);
    int i;

    epak_open(&pak, fd);

    for (i = 0; i < pak->hdr.n_docs; i++) {
        struct epak_blob_reader_t reader;
        char name[48];
        char buf[4096];
        int size;
        int fd2;

        extname(name, pak->entries[i].name);
        strcpy(&name[40], ".json.2");

        fd2 = openat(dirfd, name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        epak_read_blob(&reader, pak, &pak->entries[i].metadata, fd);
        while ((size = epak_blob_reader_read(&reader, buf, sizeof(buf))) > 0)
            write(fd2, buf, size);
        close(fd2);
        if (epak_blob_reader_finish(&reader))
            printf("csum does not match %s\n", name);

        strcpy(&name[40], ".blob.2");

        fd2 = openat(dirfd, name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        epak_read_blob(&reader, pak, &pak->entries[i].data, fd);
        while ((size = epak_blob_reader_read(&reader, buf, sizeof(buf))) > 0)
            write(fd2, buf, size);
        close(fd2);
        if (epak_blob_reader_finish(&reader))
            printf("csum does not match %s\n", name);
    }

    return 0;
}
