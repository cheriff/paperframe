#include "png.h"
#include "logging/logging.h"
#include "epd.h"
#include "led.h"

#define htonl(_x) __builtin_bswap32(_x)

struct EPD epd;

enum StreamAction {
    ACTION_ACCUMULATE = 1,
    ACTION_DROP,
    ACTION_INFLATE,
};

void
flush_imgBuff( pngStream_t *stream, size_t size, void *data )
{
    log_debug( "FLUSHING: %d @ %p\n", size, data);
    static uint8_t line[300];
    static int currentX = 0; // in bytes (2 pixels)
    static int currentY = 0;
    uint8_t *buff = data;

    if (currentX==0 && currentY==0) {
        log_debug( "Will BEGIN epd\n");
        epdBegin( &epd );
    }

    for( int i=0; i<size; i++) {
        assert( currentX < 300 );

        if (currentX == 0 ){
            uint8_t filter = buff[i++];
            if (filter != 0){
                log_fatal("Unsupported filter on row %d: %d", currentY, filter );
            }
            assert( filter == 0 && "Only unfiltered for now" );
        }

        uint8_t byte = buff[i];
        line[ currentX ] = byte;

#if defined( VIZ_DECODE )
        int hi = (byte >> 4) & 0xf;
        int lo = (byte >> 0) & 0xf;

        int x = currentX*2; // in pixels
        itermImagePut( stream->image, x++, currentY,
                stream->pallete[hi*3 + 0],
                stream->pallete[hi*3 + 1],
                stream->pallete[hi*3 + 2]);
        itermImagePut( stream->image, x++, currentY,
                stream->pallete[lo*3 + 0],
                stream->pallete[lo*3 + 1],
                stream->pallete[lo*3 + 2]);
#endif

        if ( ++currentX == 300 ) {
            log_debug( "Pushing 300Bytes" );
            epdPush( &epd, line, 300 );
            currentY++;
            currentX = 0;
            // FLUSH ROW!
        }
    }
}

int doInflate( pngStream_t *stream, size_t size, void *data )
{
    deflateContext_t *dc = &stream->dc;

    // shouldn't be un-inflated stuff from last time
    assert( dc->avail_in == 0 );

    dc->avail_in = size;
    dc->in_buff = data;
    dc->next_in = data;

    for ( ;; ) {
        size_t in_bytes = dc->avail_in;
        size_t out_bytes = dc->avail_out;

        log_debug("Inflate     AvailableIn(%zu)\tAvailableOut(%zu)", in_bytes, out_bytes);
        tinfl_status status = tinfl_decompress( &dc->inflator,
                dc->next_in, &in_bytes,
                dc->out_buff, dc->next_out, &out_bytes,
                TINFL_FLAG_HAS_MORE_INPUT
                | TINFL_FLAG_PARSE_ZLIB_HEADER
                );
        log_debug("Inflate(%d): ConsumedIn(%zu)\tConsumedOut(%zu)", status, in_bytes, out_bytes);

        dc->avail_in -= in_bytes;
        dc->next_in += in_bytes;

        dc->avail_out -= out_bytes;
        dc->next_out += out_bytes;

        if (status == TINFL_STATUS_NEEDS_MORE_INPUT ) {
            break;
        }

        if ( status == TINFL_STATUS_DONE ) {
            log_debug( "Inflation complete: flushing remainder and fnish.");
            flush_imgBuff( stream, out_bytes, dc->out_buff );
            break;
        }

        if ( !dc->avail_in ) {
            log_debug( "InBuffer empty: flushing and wait for more input");
            flush_imgBuff( stream, out_bytes, dc->out_buff );
            break;
        }

        if ( !dc->avail_out ) {
            flush_imgBuff( stream, dc->out_buff_size, dc->out_buff );
            log_debug( "OutBuffer full: flushing");
            dc->avail_out = dc->out_buff_size;
            dc->next_out = dc->out_buff;
        }
    }
    return 0;
}

void deflateContext_init( deflateContext_t *dc )
{
    assert( dc );
    tinfl_init( &dc->inflator );
    dc->out_buff_size = TINFL_LZ_DICT_SIZE;
    dc->avail_out = dc->out_buff_size; // sizeof (dc->out_buff)
    dc->next_out = dc->out_buff;

    dc->avail_in = 0;
    log_trace( "Inflate context initialised");
}


void parseChunk(pngStream_t *stream, size_t size, void *data);

void streamAction( pngStream_t *stream, size_t size, void *buff, int action,  andThen_f cb)
{
    stream->valid = 1;
    stream->action = action;
    stream->accumulator.currentBytes = 0;
    stream->accumulator.desiredBytes = size;
    stream->accumulator.destination = buff;
    stream->accumulator.next = cb;
}

void drop( pngStream_t *stream, size_t size, andThen_f cb)
{
    streamAction( stream, size, NULL, ACTION_DROP, cb);
}

void accumulate( pngStream_t *stream, size_t size, void *buff, andThen_f cb)
{
    streamAction( stream, size, buff, ACTION_ACCUMULATE, cb);
}

void deflateBytes( pngStream_t *stream, size_t size, andThen_f cb )
{
    streamAction( stream, size, NULL, ACTION_INFLATE, cb);
}

void
nextChunk(pngStream_t *stream, size_t size, void *data)
{
    accumulate( stream, sizeof( chunkHdr ), &stream->chunkHeader, parseChunk );
}

#if defined( VIZ_DECODE )
void
havePallete(pngStream_t *stream, size_t size, void *data)
{
    uint8_t *colours = data;
    for (int i=0; i<size/3; i++) {
        log_info(" [%2d]: %02x %02x %02x", i,
            colours[0], colours[1], colours[2] );
        colours += 3;
    }
    drop( stream, 4, nextChunk );
}
#endif

void
haveDeflatedBytes(pngStream_t *stream, size_t size, void *data)
{
    if (size == 0 && data == NULL) {
        // compression is over. Hopefully this means the IDAT chunk is too
        return drop( stream, 4, nextChunk );
    }

}

void
validateIHeader(pngStream_t *stream, size_t size, void *data)
{
    assert (data == &stream->imageHeader );
    assert( size == sizeof( IHDR_t ) );
    IHDR_t * imgHeader = data;

    imgHeader->height = htonl(imgHeader->height);
    imgHeader->width  = htonl(imgHeader->width);

    log_info("IHDR:\tImage is %dx%d @%dbpp",
            imgHeader->height, imgHeader->width,
            imgHeader->bitDepth);

    drop( stream, 4, nextChunk );

    led_set_interval( BLINK_BUSY );
    log_trace( "Creating EPD" );
    createEpd( &epd, spi0 );
    log_trace( "Initd EPD" );
    initEpd( &epd );
    log_trace( "Will clear EPD" );
    epdClear( &epd );
    log_trace( "EPD is initialised" );

    // TODO: validate items and abort if incorrect
}

void
endOfFile(pngStream_t *stream, size_t size, void *data)
{
    log_debug("End of file, marking stream as invalid");
    stream->valid = 0;
}

void
parseChunk(pngStream_t *stream, size_t size, void *data)
{
    assert (data == &stream->chunkHeader );
    assert( size == sizeof( chunkHdr ) );
    chunkHdr *chunk = data;
    uint16_t length = htonl( chunk->length );

    switch (chunk->fourcc ) {
        case FOURCC_IHDR:
            log_info("IHDR %d/%d", length, (int)sizeof( IHDR_t ));
            accumulate( stream, sizeof( IHDR_t ), &stream->imageHeader, validateIHeader );
            break;
        case FOURCC_PLTE:
            {
            int numColours = length/3;
            assert( (length % 3) == 0 );
            assert( numColours == 7 );
            log_info("PLTE\tPallete of %d colours", numColours);
#if defined( VIZ_DECODE )
            // need the pallete to show later, assumed we have the luxure of malloc
            stream->pallete = malloc( length );
            stream->image = itermImageCreate(
                    stream->imageHeader.width,
                    stream->imageHeader.height );
            accumulate( stream, length, stream->pallete, havePallete );
#else
            drop( stream, length+4, nextChunk );
#endif
            break;
            }
        case FOURCC_IDAT:
            log_info("IDAT: deflate %d bytes", length);
            deflateContext_init( &stream->dc );
            deflateBytes( stream, length, haveDeflatedBytes );
            break;
        case FOURCC_IEND:
            log_info("IEND: Image is finished");
            epdEnd( &epd );
            epdSleep( &epd );
            log_trace( "Epd sleeping" );
            led_set_interval( BLINK_STANDBY );
            assert( length == 0 );
            drop( stream, 4, endOfFile );
#if defined( VIZ_DECODE )
            itermImageEmit( stream->image );
#endif
            break;
        default:
            log_warn( "Unhandled chunk: %.4s (%u bytes)", (char*)(&chunk->fourcc), length );
            // skip the chunk payload, and also the crc32 footer, before
            // starting next chunk
            drop( stream, length+4, nextChunk );
    }

}

void
validateSignature(pngStream_t *stream, size_t size, void *data)
{
    assert( size == 8 );
    log_trace( "Have signature!");
    static uint8_t expected[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
    if ( memcmp( expected, data, size ) ) {
        stream->valid = 0;
        return;
    }

    accumulate( stream, sizeof( chunkHdr ), &stream->chunkHeader, parseChunk );
}

size_t
consumeBytes( pngStream_t *stream, size_t size, uint8_t *buff )
{
    log_trace("CONSUME: Have %d, Need %d",
            stream->accumulator.currentBytes,
            stream->accumulator.desiredBytes );
    int finalBytes = stream->accumulator.desiredBytes;
    int want = finalBytes - stream->accumulator.currentBytes;

    log_trace("Need %d more, have %d available", want, (int)size);
    if (want > size ) want = size;
    log_trace("Will actually consume  %d", want );

    if (stream->action == ACTION_INFLATE ) {
        if ( want == 0 ) {
            // we've passed all bytes to the deflator. Thats the end, so
            // invoke the callback with nil values
            stream->accumulator.next( stream, 0, NULL);
            return 0;
        }

        doInflate( stream, want, buff );
        stream->accumulator.currentBytes += want;

    } else {
        if (stream->action == ACTION_ACCUMULATE ){
            uint8_t *p = stream->accumulator.destination + stream->accumulator.currentBytes;
            memcpy(p, buff, want);
        }
        else assert (stream->action == ACTION_DROP );
        stream->accumulator.currentBytes += want;

        // If enough is dropped or accumulated, call the callback
        if ( stream->accumulator.currentBytes == finalBytes ) {
            stream->accumulator.next( stream, finalBytes, stream->accumulator.destination );
        }
    }

    return want;
}

int
process_sector( pngStream_t *stream, size_t size, uint8_t *buff)
{
    uint32_t *magic = (uint32_t*)buff;
    if (*magic == 0x474e5089 ) {
        log_debug( "Magic hit");
        accumulate( stream, 8, stream->signature, validateSignature );
    }


    while( size ) {
        if (!stream->valid) return -1;
        size_t consumed = consumeBytes( stream,  size, buff );
        log_trace("Did consume %zu/%zu", consumed, size );
        size -= consumed;
        buff += consumed;
    }
    return 0;
}

