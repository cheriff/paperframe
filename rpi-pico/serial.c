#include <stdint.h>
#include "serial.h"
#include "bsp/board.h"

#include "tusb.h"
#include "paperframe.h"
#include "logging/logging.h"

#include "epd/epd.h"
#include "led.h"

extern void do_dump(void);

#include "png/png.h"
epd_t the_epd;
pngStream_t the_stream = { 0 };

#define MAX_SIZE 128 * 1024
static uint8_t fileBuffer[MAX_SIZE];
static int buffPos = 0;
static int failed = 0;

static void hexdump( uint32_t base, size_t size, uint8_t *buff)
{
    char ascii[17];
    ascii[16] = 0;

    for (int i = 0; i < size; i++) {
        int mod = i%16;
        char c = buff[i];
        if ( mod == 0 ) {
            if (i) {
                printf("  |%s|\n", ascii);
            }
            printf("%08x ", i+base);
        }
        if ( mod == 8) printf(" ");
        printf(" %02x", c);
        if (c < ' ' || c > '~') {
            c = '.';
        }
        ascii[mod] = c;
    }
    printf("  |%s|\n", ascii);
}

static int imageStart( pngStream_t *stream)
{
    epd_t *epd = stream->callbackPriv;
    led_set_interval( BLINK_BUSY );
    log_trace( "Initd EPD" );
    initEpd( epd );
    log_trace( "Will clear EPD" );
    epdClear( epd );
    log_trace( "EPD is initialised" );
    return 0;
}

void init_serial( void )
{
    log_trace( "Creating EPD" );
    createEpd( &the_epd );
}



static int
imageRow( pngStream_t *stream, int rowY, const uint8_t *buff, size_t byteSize )
{
    epd_t *epd = stream->callbackPriv;
    if (rowY == 0) {
        epdBegin( epd );
    }
    epdPush( epd, buff, byteSize );
}

static int
imageEnd( pngStream_t* stream)
{
    epd_t *epd = stream->callbackPriv;
    epdEnd( epd );
    epdSleep( epd );
    log_trace( "Epd sleeping" );
    led_set_interval( BLINK_STANDBY );
}


// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
  (void) itf; (void) rts;

  static int starttime = 0;

  if ( dtr )
  {
    // Terminal connected
    printf("UART CONNECTED\n");
    starttime = board_millis();
    memset(&the_stream, 0, sizeof( the_stream ));
    the_stream.callbacks.imageStart = imageStart;
    the_stream.callbacks.imageRow = imageRow;
    the_stream.callbacks.imageEnd = imageEnd;
    the_stream.callbackPriv = &the_epd;
    buffPos = 0;
    failed = 0;
  } else {
    // Terminal disconnected
    int rx = board_millis();
    if (failed)
        printf("Was failed\n");
    else {
        //hexdump( 0, buffPos, fileBuffer);
        process_sector( &the_stream, buffPos, fileBuffer );
    }
    int now = board_millis();
    printf("COMPLETE: Transfer: %d ms\n", rx - starttime);
    printf("Display: %d ms\n", now - rx );
  }
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf){}
void cdc_task(void)
{
    while( tud_cdc_available() ) {
        int available = MAX_SIZE - buffPos;
        if (available > 32) available = 32;

        if (available < 0 ) {
            printf("Buffer full: dropping\n");
            failed = 1;
            tud_cdc_read( fileBuffer, available);
            continue;
        }
        uint32_t count = tud_cdc_read( fileBuffer+buffPos, available);
        //printf("Received:: (%d) %d / %d\n", buffPos, count, available);
        buffPos += count;
    }
}
