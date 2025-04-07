#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "events.h"

#define BUTTON_GPIO GPIO_NUM_0  // IO0 button
#define BUTTON_DEBOUNCE_TIME_US 90000  // 90ms debounce time
#define BUTTON_LONG_PRESS_TIME_US 3000000  // 3 seconds long press time

void button_init(void);

#endif // BUTTON_MANAGER_H