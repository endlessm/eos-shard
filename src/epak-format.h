
#ifndef EPAK_FORMAT_H
#define EPAK_FORMAT_H

#include <stdint.h>

/* EPAK file format */

#pragma pack(push,1)

#define EPAK_MAGIC ("EPK1")

struct epak_hdr
{
  char magic[4];
  uint32_t n_docs;
  uint64_t data_offs;
};

typedef enum epak_blob_content_type
{
  EPAK_BLOB_CONTENT_TYPE_UNKNOWN,
  EPAK_BLOB_CONTENT_TYPE_HTML,
  EPAK_BLOB_CONTENT_TYPE_PNG,
  EPAK_BLOB_CONTENT_TYPE_JPG,
  EPAK_BLOB_CONTENT_TYPE_PDF,
  EPAK_BLOB_CONTENT_TYPE_JSON,
  EPAK_BLOB_CONTENT_TYPE_TEXT_PLAIN,
} EpakBlobContentType;

typedef enum epak_blob_flags
{
  EPAK_BLOB_FLAG_NONE,
  EPAK_BLOB_FLAG_COMPRESSED_ZLIB,
} EpakBlobFlags;

struct epak_blob_entry
{
  uint16_t content_type;
  uint16_t flags;
  uint32_t adler32;
  uint64_t offs;
  uint64_t size;
  uint64_t uncompressed_size;
};

#define EPAK_RAW_NAME_SIZE 20
#define EPAK_HEX_NAME_SIZE (EPAK_RAW_NAME_SIZE*2)

struct epak_doc_entry
{
  uint8_t raw_name[EPAK_RAW_NAME_SIZE];
  struct epak_blob_entry metadata;
  struct epak_blob_entry data;
};

#pragma pack(pop)

#endif /* EPAK_FORMAT_H */
