
#define _GNU_SOURCE

#include "epak_private.h"

#include <fcntl.h>
#include <dirent.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

static int filt(const struct dirent *ent)
{
    /* 40 characters + ".json" */
    if (strlen(ent->d_name) != 45)
        return 0;
    /* check for "json" */
    if (ent->d_name[41] != 'j')
        return 0;
    /* XXX: should do extra validation probs */
    return 1;
}

static void mangle(char *v)
{
    int n = strlen(v);
    strcpy(n + v - 5, ".blob");
}

static char * strjoin(char *a, char *b)
{
    int n = strlen(a);
    char *s = malloc(n + strlen(b));
    strcpy(s, a);
    strcpy(s+n, b);
    return s;
}

static void fillname(uint8_t *hexstr, char *jsonfn)
{
    int i;
    for (i = 0; i < 20; i++)
        sscanf(&jsonfn[i*2], "%2hhx", &hexstr[i]);
}

int main(int argc, char *argv[])
{
    int i;
    int nent;
    struct dirent **ents;
    struct epak_t *pak;
    char *dirname = argv[1];
    char *epakname = strjoin(dirname, ".epak");

    int dirfd = open(dirname, O_RDONLY | O_DIRECTORY);
    nent = scandirat(dirfd, ".", &ents, filt, alphasort);
    pak = epak_new(nent);

    struct epak_writer_t writer;
    struct epak_blob_writer_t blob_writer;
    int fd = open(epakname, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    epak_write(&writer, fd, pak);

    for (i = 0; i < nent; i++) {
        printf("packing %.40s\n", ents[i]->d_name);
        int fd2;
        char buf[4096];
        int size;

        fillname(pak->entries[i].raw_name, ents[i]->d_name);
        fd2 = openat(dirfd, ents[i]->d_name, O_RDONLY);
        epak_write_blob(&blob_writer, &writer, &pak->entries[i].metadata, 0);
        while ((size = read(fd2, buf, sizeof(buf))) > 0)
            epak_blob_writer_write(&blob_writer, buf, size);
        close(fd2);

        mangle(ents[i]->d_name);

        fd2 = openat(dirfd, ents[i]->d_name, O_RDONLY);
        if (fd2 < 0) {
            fprintf(stderr, "no file about %s\n", ents[i]->d_name);
            return 1;
        }

        epak_write_blob(&blob_writer, &writer, &pak->entries[i].data, 0);
        while ((size = read(fd2, buf, sizeof(buf))) > 0)
            epak_blob_writer_write(&blob_writer, buf, size);
        close(fd2);
    }

    epak_write_finish(&writer);

    return 0;
}
