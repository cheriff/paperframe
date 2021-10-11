/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <ctype.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "djmWifi.h"

#include "epd/epd.h"
#include "png/png.h"

#include "logging/logging.h"
int LogLevel = LOG_INFO;

int DO_EPD = 1;

/* Borrowed from:
 * https://stackoverflow.com/questions/7775991/how-to-get-hexdump-of-a-structure-data
 */
void hexDump (const void * addr, const int len) {
    int i;
    unsigned char buff[17];
    const unsigned char * pc = (const unsigned char *)addr;

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        if ((i % 16) == 0) {
            // Don't print ASCII buffer for the "zeroth" line.
            if (i != 0) printf ("  %s\n", buff);
            printf ("  %04x ", i); // Byte offset
        }

        // Now the hex code for the specific character.
        printf (" 0x%02x", pc[i]);

        // And buffer a printable ASCII character for later.
        buff[i%16] = isprint(pc[i])? pc[i] : '.';
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }

    // And print the final ASCII buffer.
    printf ("  %s\n", buff);
}

pngStream_t the_stream;

int sz = 0;
int 
my_callback(int len, uint8_t *bytes, void *user)
{
    process_sector( user, len, bytes );
    return 0;
}

static int
imageStart( pngStream_t *stream)
{
    printf("IMAGE START!\n");
    return 0;
}
static int
imageEnd( pngStream_t* stream)
{
    printf("IMAGE END\n");
    return 0;
}

static int
imageRow( pngStream_t *stream, int rowY, const uint8_t *buff, size_t byteSize )
{
    epd_t *epd = stream->callbackPriv;
    if (DO_EPD)
        epdPush(epd, buff, byteSize);
    return 0;
}

void app_main(void)
{
    printf("Hello world!\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is %d CPU cores, WiFi%s%s, ",
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    printf("silicon revision %d, ", chip_info.revision);

    epd_t epaper = {
        .pins = {
            .BUSY = 23,
            .RST = 18,
            .DC = 17,
            .CS = 15,
            .MOSI = 13,
            .SCK = 14,
        }
    };
    if (DO_EPD) {
        ESP_ERROR_CHECK( createEpd(&epaper) );
        printf("Opened Epaper\n");
        initEpd(&epaper);
        epdClear(&epaper);
        //epd_demo(&epaper);
    }

    memset(&the_stream, 0, sizeof( the_stream ));
    the_stream.callbacks.imageStart = imageStart;
    the_stream.callbacks.imageRow = imageRow;
    the_stream.callbacks.imageEnd = imageEnd;
    the_stream.callbackPriv = &epaper;


    // WIFIPASS is set in private.cmake, if/when that exists.
    djm_wifi_init("WiFi-535E", WIFIPASS);

    {
        if(DO_EPD)
            epdBegin(&epaper);
        djm_download("http://192.168.1.101:8099/out.png", my_callback, &the_stream);
        if(DO_EPD)
            epdEnd(&epaper);
    }

die:
    printf("Sleeping\n");
    if(DO_EPD)
        epdSleep(&epaper);
    printf("Finished: %d\n", sz);

    printf("Done\n");
    for (int i = 0; ; i++) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        printf("Tick %d    \r", i);
        fflush(stdout);
    }
    goto die;
}
