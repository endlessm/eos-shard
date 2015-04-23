
#ifndef EPAK_PRIVATE_H
#define EPAK_PRIVATE_H

#include <stdint.h>
#include <unistd.h>
#include "epak_fmt.h"

struct epak_t
{
    struct epak_hdr hdr;
    struct epak_doc_entry entries[];
};


/* reader API */

struct epak_blob_reader_t {
    int fd;
    off_t base_offs;
    off_t offs;
    struct epak_blob_entry *blob;
    uint32_t adler32;
};

int epak_open(struct epak_t **pak_out, int fd);
void epak_read_blob(struct epak_blob_reader_t *reader,
                    struct epak_t *pak,
                    struct epak_blob_entry *entry,
                    int fd);
int epak_blob_reader_read(struct epak_blob_reader_t *reader,
                          char *buf, int size);
int epak_blob_reader_finish(struct epak_blob_reader_t *reader);


/* writer API */

struct epak_writer_t {
    int fd;
    off_t entry_offs;
    struct epak_t *pak;
};

enum epak_blob_writer_flags {
    EPAK_BLOB_WRITER_FLAG_COMPRESS = 0x01,
};

struct epak_blob_writer_t {
    struct epak_writer_t *writer;
    struct epak_blob_entry *blob;
};

struct epak_t * epak_new(int n_docs);
int epak_write(struct epak_writer_t *writer, int fd, struct epak_t *pak);
int epak_blob_writer_write(struct epak_blob_writer_t *blob_writer,
                           char *data, int size);
int epak_write_blob(struct epak_blob_writer_t *blob_writer,
                    struct epak_writer_t *writer,
                    struct epak_blob_entry *blob,
                    enum epak_blob_writer_flags flags);
int epak_write_finish(struct epak_writer_t *writer);

#endif /* EPAK_PRIVATE_H */

