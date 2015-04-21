
#include <assert.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "epak.h"
#include "adler32.h"

/* */

#define ALIGN(n) (((n) + 0x3f) & ~0x3f)

static off_t lalign(int fd)
{
    off_t off = lseek(fd, 0, SEEK_CUR);
    off = ALIGN(off);
    lseek(fd, off, SEEK_SET);
    return off;
}

static struct epak_t * epak_alloc(int n_docs)
{
    return malloc(sizeof(struct epak_t) + sizeof(struct epak_doc_entry)*n_docs);
}

int epak_open(struct epak_t **pak_out, int fd)
{
    struct epak_hdr hdr;
    read(fd, &hdr, sizeof(hdr));

    if (memcmp(hdr.magic, EPAK_MAGIC, sizeof(hdr.magic)) != 0)
        return -EINVAL;

    struct epak_t *pak = epak_alloc(hdr.n_docs);
    if (pak == NULL)
        return -ENOMEM;

    pak->hdr = hdr;
    lalign(fd);

    int i;
    for (i = 0; i < hdr.n_docs; i++)
        read(fd, &pak->entries[i], sizeof(struct epak_doc_entry));

    lalign(fd);

    *pak_out = pak;
    return 0;
}

void epak_read_blob(struct epak_blob_reader_t *reader,
                    struct epak_t *pak,
                    struct epak_blob_entry *blob,
                    int fd)
{
    reader->fd = fd;
    reader->base_offs = pak->hdr.data_offs + blob->offs;
    reader->offs = 0;
    reader->blob = blob;
    reader->adler32 = ADLER32_INIT;
}

static int min(int a, int b) { return a < b ? a : b; }

int epak_blob_reader_read(struct epak_blob_reader_t *reader,
                          char *buf, int size)
{
    int end = min(reader->offs + size, reader->blob->size);
    size = end - reader->offs;

    lseek(reader->fd, reader->base_offs + reader->offs, SEEK_SET);
    read(reader->fd, buf, size);
    reader->adler32 = adler32(reader->adler32, (uint8_t *) buf, size);
    reader->offs += size;

    return size;
}

int epak_blob_reader_finish(struct epak_blob_reader_t *reader)
{
    return (reader->adler32 == reader->blob->adler32) ? 0 : -1;
}

struct epak_t * epak_new(int n_docs)
{
    struct epak_t *pak = epak_alloc(n_docs);

    memcpy(pak->hdr.magic, EPAK_MAGIC, sizeof(pak->hdr.magic));
    pak->hdr.n_docs = n_docs;
    pak->hdr.data_offs = ALIGN(ALIGN(sizeof(pak->hdr)) + sizeof(*pak->entries)*pak->hdr.n_docs);

    return pak;
}

int epak_write(struct epak_writer_t *writer, int fd, struct epak_t *pak)
{
    int i;

    writer->fd = fd;
    writer->pak = pak;

    write(fd, &pak->hdr, sizeof(pak->hdr));

    writer->entry_offs = lalign(fd);

    for (i = 0; i < pak->hdr.n_docs; i++)
        write(fd, &pak->entries[i], sizeof(*pak->entries));

    lalign(fd);
    assert(lseek(fd, 0, SEEK_CUR) == pak->hdr.data_offs);

    return 0;
}

int epak_blob_writer_write(struct epak_blob_writer_t *blob_writer,
                           char *data, int size)
{
    struct epak_blob_entry *blob = blob_writer->blob;
    blob->adler32 = adler32(blob->adler32, (uint8_t *) data, size);
    write(blob_writer->writer->fd, data, size);
    blob->size += size;
    return 0;
}

int epak_write_blob(struct epak_blob_writer_t *blob_writer,
                    struct epak_writer_t *writer,
                    struct epak_blob_entry *blob,
                    enum epak_blob_writer_flags flags)
{
    blob_writer->writer = writer;
    blob_writer->blob = blob;

    blob->flags = 0;
    blob->adler32 = ADLER32_INIT;
    blob->offs = lalign(writer->fd) - writer->pak->hdr.data_offs;
    blob->size = 0;

    return 0;
}

int epak_write_finish(struct epak_writer_t *writer)
{
    lseek(writer->fd, writer->entry_offs, SEEK_SET);

    int i;
    for (i = 0; i < writer->pak->hdr.n_docs; i++)
        write(writer->fd, &writer->pak->entries[i], sizeof(struct epak_doc_entry));

    return 0;
}
