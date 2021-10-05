#ifndef __PNG_H__
#define __PNG_H__

#include <stdint.h>
#include "miniz/miniz.h"

#define FOURCC(s)  (uint32_t)(s[3]<<24 | s[2]<<16 | s[1]<<8 | s[0])

// pi-pick compiler doensn't like the FOURCC() macro constructing
// constants for switch cases. So we do manually
#define FOURCC_IHDR 0x52444849
#define FOURCC_PLTE 0x45544c50
#define FOURCC_IDAT 0x54414449
#define FOURCC_IEND 0x444e4549

typedef struct {
    uint32_t length;
    uint32_t fourcc;
} __attribute__((__packed__)) chunkHdr;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t bitDepth;
    uint8_t colourType;
    uint8_t compressionMethod;
    uint8_t filterMethod;
    uint8_t interlaceMethod;
} __attribute__((__packed__)) IHDR_t;

typedef struct pngStream pngStream_t;
typedef void (*andThen_f)(pngStream_t *stream, size_t size, void *data);

typedef struct {
    tinfl_decompressor inflator;

    size_t avail_in;
    uint8_t *in_buff;

    size_t out_buff_size;;
    size_t avail_out;
    uint8_t out_buff[TINFL_LZ_DICT_SIZE];

    uint8_t *next_in;
    uint8_t *next_out;
} deflateContext_t;

struct pngStream {
    int valid;
    int action;
    IHDR_t imageHeader;

#if defined (VIZ_DECODE )
    uint8_t *pallete;
    itermImage_t *image;
#endif

    union {
        uint8_t signature[8];
        chunkHdr chunkHeader;
        deflateContext_t dc;
    };

    struct {
        int currentBytes;
        int desiredBytes;
        uint8_t *destination;
        andThen_f next;
    } accumulator;
};

/* Entry point! */
int process_sector( pngStream_t *stream, size_t size, uint8_t *buff);

#endif // __PNG_H__
