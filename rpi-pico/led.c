#include "led.h"

#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "pico/time.h"
#include "bsp/board.h"

static uint32_t start_ms = 0;
static bool led_state = false;
static uint32_t blink_interval_ms = BLINK_SUSPENDED;

void on_pwm_wrap() {
    static int fade = 0;
    static bool going_up = true;
    static uint32_t last_update = 0;
    // Clear the interrupt flag that brought us here
    pwm_clear_irq(pwm_gpio_to_slice_num(PICO_DEFAULT_LED_PIN));

    const uint32_t now = board_millis();
    if ( now < last_update + blink_interval_ms ) {
        // too soon nothing to do;
        return;
    }

    if (going_up) {
        ++fade;
        if (fade > 255) {
            fade = 255;
            going_up = false;
        }
    } else {
        --fade;
        if (fade < 0) {
            fade = 0;
            going_up = true;
        }
    }
    // Square the fade value to make the LED's brightness appear more linear
    // Note this range matches with the wrap value
    pwm_set_gpio_level(PICO_DEFAULT_LED_PIN, fade * fade);
    last_update = now;
}



int
led_init( void )
{
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;

    // Tell the LED pin that the PWM is in charge of its value.
    gpio_set_function(LED_PIN, GPIO_FUNC_PWM);

    // Figure out which slice we just connected to the LED pin
    uint slice_num = pwm_gpio_to_slice_num(LED_PIN);
    printf(" GPIO %d is slice %d\n", LED_PIN, slice_num);

    // Mask our slice's IRQ output into the PWM block's single interrupt line,
    // and register our interrupt handler
    pwm_clear_irq(slice_num);
    pwm_set_irq_enabled(slice_num, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, on_pwm_wrap);
    irq_set_enabled(PWM_IRQ_WRAP, true);

    // Get some sensible defaults for the slice configuration. By default, the
    // counter is allowed to wrap over its maximum range (0 to 2**16-1)
    pwm_config config = pwm_get_default_config();
    // Set divider, reduces counter clock to sysclock/this value
    pwm_config_set_clkdiv(&config, 1.f);
    // Load the configuration into our PWM slice, and set it running.
    pwm_init(slice_num, &config, true);

    return 0;
}

void
led_task( void )
{
    return;
  // Blink every interval ms
  if ( board_millis() - start_ms < blink_interval_ms) return; // not enough time
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}

uint32_t
led_set_interval( uint32_t new_interval)
{
    printf("INTERVAL: %d\n", new_interval );
    uint32_t prev = blink_interval_ms;
    blink_interval_ms = new_interval;
    return prev;
}
