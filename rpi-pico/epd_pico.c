#include "epd/epd.h"

#include <stdio.h>
#include <string.h>

#include "hardware/spi.h"
#include "hardware/gpio.h"

void epdReadBusyHigh( epd_t *epd );

static int
pico_pin_set( epd_t* epd, int pin, int val )
{
    (void) epd;
    gpio_put( pin, val );
    return 0;
}

static int
pico_pin_get( epd_t *epd, int pin )
{
    (void) epd;
    return gpio_get( pin );
}

static int
pico_spi_write( epd_t* epd, const uint8_t *buff, size_t sz )
{
    spi_inst_t *spi = epd->priv;
    pico_pin_set( epd, epd->pins.CS, 0);
    spi_write_blocking( spi, buff, sz );
    pico_pin_set( epd, epd->pins.CS, 1);
    return 0;
}
static void
pico_sleep_ms( epd_t* epd, int ms )
{
    (void) epd;
    sleep_ms(ms);
}


int
createEpd( epd_t *epd )
{
    epd->pins.RST = 21;
    epd->pins.DC = 20;
    epd->pins.BUSY = 19;
    epd->pins.CS = 18;
    epd->pins.MOSI = 7;
    epd->pins.SCK = 6;

    epd->priv = spi0;

    epd->callbacks.pin_set = pico_pin_set;
    epd->callbacks.pin_get = pico_pin_get;
    epd->callbacks.spi_write = pico_spi_write;
    epd->callbacks.sleep_ms = pico_sleep_ms;

    gpio_set_function(epd->pins.SCK, GPIO_FUNC_SPI);
    gpio_set_function(epd->pins.MOSI, GPIO_FUNC_SPI);

    spi_inst_t *spi = epd->priv;
    uint actual = spi_init(spi, 4000000 );
    printf("Enabled SPI (SCK: %d, MOSI: %d) @%dHz\n",
            epd->pins.SCK, epd->pins.MOSI, actual);

    gpio_init( epd->pins.RST );
    gpio_set_dir( epd->pins.RST, GPIO_OUT);
    printf( "Enabled RST( %d )\n", epd->pins.RST);
    gpio_init( epd->pins.DC );
    gpio_set_dir( epd->pins.DC, GPIO_OUT);
    printf( "Enabled DC( %d )\n", epd->pins.DC);
    gpio_init( epd->pins.CS );
    gpio_set_dir( epd->pins.CS, GPIO_OUT);
    printf( "Enabled CS( %d )\n", epd->pins.CS);

    gpio_init( epd->pins.BUSY );
    gpio_set_dir( epd->pins.BUSY, GPIO_IN);
    printf( "Enabled BUSY( %d )\n", epd->pins.RST);

    return 0;
}
