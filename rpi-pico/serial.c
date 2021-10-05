#include <stdint.h>
#include "serial.h"
#include "bsp/board.h"

#include "tusb.h"
#include "paperframe.h"

extern void do_dump(void);

#include "png/png.h"
pngStream_t stream = { 0 };

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
    memset(&stream, 0, sizeof( stream ));
    buffPos = 0;
    failed = 0;
  } else {
    // Terminal disconnected
    int rx = board_millis();
    if (failed)
        printf("Was failed\n");
    else {
        //hexdump( 0, buffPos, fileBuffer);
        process_sector( &stream, buffPos, fileBuffer );
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
