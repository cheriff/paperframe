#ifndef __LED_H__
#define __LED_H__

#include <stdint.h>

enum  {
  BLINK_BUSY = 1,
  BLINK_STANDBY = 10,
  BLINK_SUSPENDED = 25,
};

int led_init( void );
void led_task( void );

uint32_t led_set_interval( uint32_t new_interval);


#endif // __LED_H__
