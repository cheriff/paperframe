#include <stdio.h>

#include "stb_image.h"
#include "miniz/miniz.h"

static void
write_header( FILE *fp )
{
    uint8_t hdr[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
    int w = fwrite(hdr, 1, sizeof( hdr ), fp );
    assert( w == sizeof ( hdr ) );
}

/*
IHDR must be the first chunk; it contains (in this order)
* the image's width (4 bytes);
* height (4 bytes);
* bit depth (1 byte, values 1, 2, 4, 8, or 16);
* color type (1 byte, values 0, 2, 3, 4, or 6);
* compression method (1 byte, value 0);
* filter method (1 byte, value 0);
* and interlace method (1 byte, values 0 "no interlace" or 1 "Adam7 interlace") (13 data bytes total)
* [8] As stated in the World Wide Web Consortium, bit depth is defined as "the number of bits per sample or per palette index (not per pixel)
*/

static uint8_t *
push_uint32( uint8_t *p, uint32_t val, size_t *size )
{
    assert( *size >= sizeof( val ) );
    *((uint32_t*)p) = val;
    *size -= sizeof( val );
    return p + sizeof( val );
}
static uint8_t *
push_uint8( uint8_t *p, uint8_t val, size_t *size )
{
    assert( *size >= sizeof( val ) );
    *((uint8_t*)p) = val;
    *size -= sizeof( val );
    return p + sizeof( val );
}

static void
write_chunk( FILE *fp, uint32_t len, uint32_t type, void *data )
{
    uint32_t chunk_header[2] = { htonl( len ), type };
    int r = fwrite( chunk_header, sizeof( uint32_t ), 2, fp );
    assert( r == 2 );

    r = fwrite( data, 1, len, fp );
    assert( r == len );

    uint32_t crc = MZ_CRC32_INIT;
    crc = mz_crc32(crc, (const unsigned char*)(&type), sizeof( uint32_t ) );
    crc = mz_crc32(crc, (const unsigned char*)data, len );
    crc = htonl( crc );

    r = fwrite( &crc, sizeof( crc ), 1, fp );
    assert( r == 1 );
}


static void
write_ihdr_chunk( FILE *fp, int width, int height )
{
    uint32_t type = *((uint32_t*)"IHDR");
    const size_t len = 13;
    uint8_t data[ len ];

    uint8_t *p = data;
    size_t available = len;
    p = push_uint32( p, htonl( width ), &available );
    p = push_uint32( p, htonl( height ), &available );
    p = push_uint8( p, 4, &available ); //bitDepth (2ppb)
    p = push_uint8( p, 3, &available ); //colourType (3: palletized)
    p = push_uint8( p, 0, &available ); //compressionMethod
    p = push_uint8( p, 0, &available ); //filterMethod
    p = push_uint8( p, 0, &available ); //interlaceMethod

    write_chunk( fp, len, type, data );
}

const int COL_BLACK  = 0b0000;
const int COL_WHITE  = 0b0001;
const int COL_GREEN  = 0b0010;
const int COL_BLUE   = 0b0011;
const int COL_RED    = 0b0100;
const int COL_YELLOW = 0b0101;
const int COL_ORANGE = 0b0110;

static void
write_plte_chunk( FILE *fp )
{
    uint32_t type = *((uint32_t*)"PLTE");

    /* 7 colours * 3 bytes each = 21byte pallete */
    static uint8_t colourmap[7][3] =  {
       [ COL_BLACK  ] = { 12  , 12  , 14  },
       [ COL_WHITE  ] = { 210 , 210 , 208 },
       [ COL_GREEN  ] = { 30  , 96  , 31  },
       [ COL_BLUE   ] = { 29  , 30  , 84  },
       [ COL_RED    ] = { 140 , 27  , 29  },
       [ COL_YELLOW ] = { 211 , 201 , 61  },
       [ COL_ORANGE ] = { 193 , 113 , 42  },
    };
    assert( sizeof( colourmap ) == 21 );
    write_chunk( fp, 21, type, colourmap );
}

int convert_24_to_4( int width, int height, uint8_t * in, uint8_t *out )
{
    uint8_t byte = 0;
    int p = 0;
    for (int y=0; y<height; y++) {
        *out++ = 0; // row filter: unfiltered.

        for(int x=0; x<width; x++, p++) {
            uint32_t col = *in++;
            col = (col << 8) | *in++;
            col = (col << 8) | *in++;
            col = (col << 8);

            uint8_t next = 0;
            /* Accept images in one of two colourspaces:
             *
             * 1) A literal representation, where 'blue' is obtained from
             * rgb(0,0,1), etc. This is perhaps useful for non-photorealistic
             * image drawing as fully saturated colour channels (except orange,
             * it seems) is convinient and easy to remember.
             *
             * 2) The same output pallete as will be generated. This is useful
             * if external ditherers are used, a pallete as close to physically
             * real as possible should be used otherwise the dithering may have
             * strange artefacts when viewed on a real display.
             * This also allows this tool to consume its own output
             */
            if      (col == 0xFFFFFF00 || col == 0xd2d2d000) next = COL_WHITE;
            else if (col == 0x0000FF00 || col == 0x1d1e5400) next = COL_BLUE;
            else if (col == 0xFF000000 || col == 0x8c1b1d00) next = COL_RED;
            else if (col == 0x00000000 || col == 0x0c0c0e00) next = COL_BLACK;
            else if (col == 0xFF910000 || col == 0xc1712a00) next = COL_ORANGE;
            else if (col == 0x00ff0000 || col == 0x1e601f00) next = COL_GREEN;
            else if (col == 0xffff0000 || col == 0xd3c93d00) next = COL_YELLOW;
            else {
                printf("UNEXPECTED COLOUR: %08x\n", col );
                assert(!"ded");
            }

            byte = (byte<<4) | next;
            if (p&1) {
                *out++ = byte;
                byte = 0;
            }
        }
    }
    return 0;
}


static void
write_idat_chunk( FILE *fp, int width, int height, uint8_t *original )
{
    // each line is width/2 bytes (4ppb) + 1 for the filter type
    size_t raw_size = height * (width/2 + 1);
    uint8_t *raw =  malloc( raw_size );
    assert( raw != NULL );

    if (original) 
        convert_24_to_4( width, height, original, raw );
    else {
        size_t sz = raw_size;
        uint8_t *p = raw;
        const int ROW_PER_BAND=height/7;
        for( int y=0; y<height; y++ ){
            p = push_uint8( p, 0, &sz );
            uint8_t c = 6 - ( y/ROW_PER_BAND); // roughly divide into 7 bands
            c = c | (c<<4);
            for( int x=0; x<width/2; x++) {
                p = push_uint8( p, c, &sz );
            }
        }
        printf("Buff is %zu; Remainig: %zu\n", raw_size, sz);
        assert(sz==0);
    }

    size_t deflated_size;
    uint8_t *deflated = tdefl_compress_mem_to_heap(raw, raw_size,
            &deflated_size,
            0
            //| TDEFL_FORCE_ALL_RAW_BLOCKS
            | TDEFL_WRITE_ZLIB_HEADER
            | 4095
            );
    printf("DEFLATED TO: %zu @ %p\n", deflated_size, deflated);

    uint32_t type = *((uint32_t*)"IDAT");
    write_chunk( fp, deflated_size, type, deflated );
}

static void
write_iend_chunk( FILE *fp )
{
    uint32_t type = *((uint32_t*)"IEND");
    write_chunk( fp, 0, type, NULL );
}

int
main( int argc, char *argv[] )
{
    const char* in_fname = ( argc > 1 )? argv[ 1 ] : "in.png";
    FILE *fpin = fopen( in_fname, "rb" );
    assert( fpin );

    const char* out_fname = ( argc > 2 )? argv[ 2 ] : "out.png";
    FILE *fpout = fopen( out_fname, "wb" );
    assert( fpout );


    int WIDTH, HEIGHT, CHAN;
    void *img = stbi_load_from_file(fpin, &WIDTH, &HEIGHT, &CHAN, 3);

    printf( "Loaded %p: %dx%d@%dbpp\n", img, WIDTH, HEIGHT, CHAN*8);
    assert( img );

    write_header( fpout );
    write_ihdr_chunk( fpout, WIDTH, HEIGHT );
    write_plte_chunk( fpout );

    write_idat_chunk( fpout, WIDTH, HEIGHT, img);
    write_iend_chunk( fpout );

    fclose( fpout );
    printf("Wrote: %s\n", out_fname );
    return 0;
}
