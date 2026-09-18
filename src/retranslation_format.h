#ifndef RETRANSLATION_FORMAT_H
#define RETRANSLATION_FORMAT_H

#include <stdint.h>

#pragma pack(push, 1)

typedef struct {
    char magic[4];          /* "IFTR" (In Falsus Translation Resource) */
    uint32_t version;       /* Format version = 1 */
    uint32_t entry_count;   /* 18015 */
    uint32_t pool_size;     /* Size of string pool in bytes */
} IFTRHeader;

typedef struct {
    char magic[4];          /* "IFTC" (In Falsus Translation Compressed) */
    uint32_t version;       /* Format version = 1 */
    uint32_t decomp_size;   /* Uncompressed size in bytes */
    uint32_t comp_size;     /* Compressed payload size in bytes */
} IFTCHeader;

typedef struct {
    uint16_t scene_id;      /* Scene number (1 - 236) */
    uint16_t line_id;       /* Line number (1 - 279) */
    uint32_t str_offset;    /* Byte offset from start of string pool */
    uint16_t str_len;       /* Length of UTF-8 text in bytes */
    uint16_t flags;         /* Flags (bit 0: phone chat, etc.) */
} IFTREntry;

#pragma pack(pop)

#endif /* RETRANSLATION_FORMAT_H */
