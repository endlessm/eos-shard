
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

enum epak_blob_content_type
{
    EPAK_BLOB_CONTENT_TYPE_UNKNOWN,
    EPAK_BLOB_CONTENT_TYPE_HTML,
    EPAK_BLOB_CONTENT_TYPE_PNG,
    EPAK_BLOB_CONTENT_TYPE_JPG,
    EPAK_BLOB_CONTENT_TYPE_PDF,
};

enum epak_blob_flags
{
    EPAK_BLOB_FLAG_COMPRESSED_ZLIB = 0x01,
};

struct epak_blob_entry
{
    uint16_t content_type;
    uint16_t flags;
    uint32_t adler32;
    uint64_t offs;
    uint64_t size;
    uint64_t uncompressed_size;
};

struct epak_doc_entry
{
    char name[20];
    struct epak_blob_entry json_ld;
    struct epak_blob_entry data;
};

#pragma pack(pop)
