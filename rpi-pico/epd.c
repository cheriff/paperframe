#include "epd.h"

#include <stdio.h>
#include <string.h>

const int EPD_WIDTH       = 600;
const int EPD_HEIGHT      = 448;

#define EPD_NOP 0

int
createEpd( struct EPD *epd, spi_inst_t * spi)
{
    epd->RST = 21;
    epd->DC = 20;
    epd->BUSY = 19;
    epd->CS = 18;
    epd->MOSI = 7;
    epd->SCK = 6;
    epd->spi = spi;
    epd->numPushed = 0;
    return 0;
}


static void
resetEpd( struct EPD *epd )
{
    if (EPD_NOP) return;
    gpio_put( epd->RST, 1);
    sleep_ms(600);
    gpio_put( epd->RST, 0);
    sleep_ms(2);
    gpio_put( epd->RST, 1);
    sleep_ms(200);
}


static void
epdSendCommand( struct EPD *epd, const uint8_t cmd )
{
    if (EPD_NOP) return;
    gpio_put( epd->DC, 0);
    gpio_put( epd->CS, 0);
    spi_write_blocking( epd->spi, &cmd, 1 );
    gpio_put( epd->CS, 1);
}

static void
epdSendData( struct EPD *epd, const uint8_t *src, size_t sz )
{
    if (EPD_NOP) return;
    gpio_put( epd->DC, 1);
    gpio_put( epd->CS, 0);
    spi_write_blocking( epd->spi, src, sz );
    gpio_put( epd->CS, 1);
}

static void
epdSendData1( struct EPD *epd, const uint8_t d )
{
    if (EPD_NOP) return;
    epdSendData( epd, &d, 1 );
}

static void
epdReadBusyHigh( struct EPD *epd )
{
    if (EPD_NOP) return;
    printf("Waiting BusyHigh ..\n" );
    while( gpio_get( epd->BUSY ) == 0) {
        sleep_ms(100);
    }
    printf("Waiting Done\n" );
}

static void
epdReadBusyLow( struct EPD *epd )
{
    if (EPD_NOP) return;
    printf("Waiting BusyLow..\n" );
    while( gpio_get( epd->BUSY ) == 1) {
        sleep_ms(100);
    }
    printf("Waiting Done\n" );
}

int
initEpd( struct EPD *epd )
{

    gpio_set_function(epd->SCK, GPIO_FUNC_SPI);
    gpio_set_function(epd->MOSI, GPIO_FUNC_SPI);
    uint actual = spi_init(epd->spi, 4000000 );
    printf("Enabled SPI (SCK: %d, MOSI: %d) @%dHz\n",
            epd->SCK, epd->MOSI, actual);

    gpio_init( epd->RST );
    gpio_set_dir( epd->RST, GPIO_OUT);
    printf( "Enabled RST( %d )\n", epd->RST);
    gpio_init( epd->DC );
    gpio_set_dir( epd->DC, GPIO_OUT);
    printf( "Enabled DC( %d )\n", epd->RST);
    gpio_init( epd->CS );
    gpio_set_dir( epd->CS, GPIO_OUT);
    printf( "Enabled CS( %d )\n", epd->RST);

    gpio_init( epd->BUSY );
    gpio_set_dir( epd->BUSY, GPIO_IN);
    printf( "Enabled BUSY( %d )\n", epd->RST);


    /* reset process */
    resetEpd( epd );
    epdReadBusyHigh( epd );
    epdSendCommand( epd, 0x00);
    epdSendData1( epd, 0xEF);
    epdSendData1( epd, 0x08);
    epdSendCommand( epd, 0x01);
    epdSendData1( epd, 0x37);
    epdSendData1( epd, 0x00);
    epdSendData1( epd, 0x23);
    epdSendData1( epd, 0x23);
    epdSendCommand( epd, 0x03);
    epdSendData1( epd, 0x00);
    epdSendCommand( epd, 0x06);
    epdSendData1( epd, 0xC7);
    epdSendData1( epd, 0xC7);
    epdSendData1( epd, 0x1D);
    epdSendCommand( epd, 0x30);
    epdSendData1( epd, 0x3C);
    epdSendCommand( epd, 0x40);
    epdSendData1( epd, 0x00);
    epdSendCommand( epd, 0x50);
    epdSendData1( epd, 0x37);
    epdSendCommand( epd, 0x60);
    epdSendData1( epd, 0x22);
    epdSendCommand( epd, 0x61);
    epdSendData1( epd, 0x02);
    epdSendData1( epd, 0x58);
    epdSendData1( epd, 0x01);
    epdSendData1( epd, 0xC0);
    epdSendCommand( epd, 0xE3);
    epdSendData1( epd, 0xAA);

    sleep_ms(100);
    epdSendCommand( epd, 0x50);
    epdSendData1( epd, 0x37);

    return 0;
}

void
epdBegin( struct EPD *epd )
{
    epdSendCommand( epd, 0x61 ); //Set Resolution setting
    epdSendData1( epd, 0x02);
    epdSendData1( epd, 0x58);
    epdSendData1( epd, 0x01);
    epdSendData1( epd, 0xC0);
    epdSendCommand( epd,0x10);
    epd->numPushed = 0;
}

void
epdPush( struct EPD *epd, uint8_t *data, size_t len )
{
    epdSendData( epd, data, len );
}

void epdClear( struct EPD *epd )
{
    uint8_t row[ EPD_WIDTH / 2 ];

    epdBegin( epd );

#define BLOCK (EPD_HEIGHT/7)
    for (int i=0; i<EPD_HEIGHT; i++) {
        uint8_t v;
        if      ( i < ( BLOCK*1) ) v = 0x00;
        else if ( i < ( BLOCK*2) ) v = 0x11;
        else if ( i < ( BLOCK*3) ) v = 0x22;
        else if ( i < ( BLOCK*4) ) v = 0x33;
        else if ( i < ( BLOCK*5) ) v = 0x44;
        else if ( i < ( BLOCK*6) ) v = 0x55;
        else if ( i < ( BLOCK*7) ) v = 0x66;

        v = 0x11;;
        memset( row, v, EPD_WIDTH / 2 );
        epdSendData( epd, row, EPD_WIDTH / 2 );
    }

    epdEnd( epd );
}

void epdEnd( struct EPD *epd )
{
    epdSendCommand( epd, 0x04); //0x04
    epdReadBusyHigh( epd );
    epdSendCommand( epd,0x12); //#0x12
    epdReadBusyHigh( epd );
    epdSendCommand( epd,0x02);//  #0x02
    epdReadBusyLow( epd );
    sleep_ms(500);
}

void epdSleep( struct EPD *epd )
{
    sleep_ms(500);
    epdSendCommand( epd, 0x07); //DEEP_SLEEP
    epdSendData1( epd, 0XA5);
    gpio_put( epd->RST, 0);
}
