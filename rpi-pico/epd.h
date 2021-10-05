#ifndef __EPD_H__
#define __EPD_H__

#include <stdint.h>

#include "hardware/spi.h"
#include "hardware/gpio.h"

#define EPD_BLACK  (0x000000) //   0000  BGR
#define EPD_WHITE  (0xffffff) //   0001
#define EPD_GREEN  (0x00ff00) //   0010
#define EPD_BLUE   (0xff0000) //   0011
#define EPD_RED    (0x0000ff) //   0100
#define EPD_YELLOW (0x00ffff) //   0101
#define EPD_ORANGE (0x0080ff) //   0110

extern const int EPD_WIDTH; // 600;
extern const int EPD_HEIGHT; // 448;

struct EPD {
    int RST;
    int DC;
    int BUSY;
    int CS;

    spi_inst_t *spi;
    int MOSI;
    int SCK;

    int numPushed;
};

int createEpd( struct EPD *epd, spi_inst_t * spi);
int initEpd( struct EPD *epd );

/* Drawing:
 * Begin()
 * Push exactly 600*448/2 bytes
 * End()
 */
void epdBegin( struct EPD *epd );
void epdPush( struct EPD *epd, uint8_t *data, size_t len );
void epdEnd( struct EPD *epd );

/* Clear is just drawing white 0x44 pixels */
void epdClear( struct EPD *epd );

void epdSleep( struct EPD *epd );



#endif // __EPD_H__
