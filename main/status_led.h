#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <stdint.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "events.h"

typedef enum {
    LED_OK = 0,
    LED_FAST_BLINK,
    LED_SLOW_BLINK,
    LED_OFF,
    LED_ERROR,
    LED_THREE_BLINK,
    LED_MAX_STATES
} led_state_t;

void status_led_task(void *pvParameter);
void status_led_init(void);
void status_led_set(led_state_t led_state);
void status_led_event_handler(event_t *event);

#endif // STATUS_LED_H