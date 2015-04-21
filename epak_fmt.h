
#pragma once

/* EPAK file format */

#define EPAK_MAGIC ("EPK1")

#pragma pack(push,1)

struct epak_hdr
{
    char magic[4];
    uint32_t n_docs;
    uint64_t data_offs;
};

enum epak_blob_flags
{
    EPAK_BLOB_FLAG_COMPRESSED_ZLIB = 0x01,
};

struct epak_blob_entry
{
    uint32_t flags;
    uint32_t adler32;
    uint64_t offs;
    uint64_t size;
};

struct epak_doc_entry
{
    char name[20];
    struct epak_blob_entry json_ld;
    struct epak_blob_entry data;
};

#pragma pack(pop)
