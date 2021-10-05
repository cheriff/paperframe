/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "logging/logging.h"

int LogLevel = LOG_INFO;


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/bootrom.h"

#include "bsp/board.h"
#include "tusb.h"

#include "led.h"
#include "epd.h"
#include "serial.h"


volatile int QUIT = 0;

int main(void)
{
    int r;
    board_init();
    stdio_uart_init();

    r = led_init();
    log_debug( "Did LED init: %d\n", r );

    printf("Hello on the UART test\n");
    printf("OK\n");
    tusb_init();

    while (1)
    {
        tud_task(); // tinyusb device task
        cdc_task();
        led_task();
        if (QUIT) break;
    }
    printf("FINISHED\n");

finish:
    printf("REBOOT TIME, <disabling usb mas storage>\n");
    reset_usb_boot( 1<<PICO_DEFAULT_LED_PIN, 1);

    return 0;

}

// Invoked when device is mounted
void tud_mount_cb(void)
{
  led_set_interval( BLINK_STANDBY );
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  led_set_interval( BLINK_SUSPENDED );
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
    printf("suspend\n");
  (void) remote_wakeup_en;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
    printf("rsumt\n");
}




