#ifndef EVENTS_H
#define EVENTS_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_event.h"
#include "nvs_manager.h"
#include "state_manager.h"

// Declare an event base
ESP_EVENT_DECLARE_BASE(CUSTOM_EVENTS);        // declaration of the timer events family

// Event types
enum {
    EVENT_WIFI_CONNECTED,               // Event for WiFi connection established
    EVENT_WIFI_DISCONNECTED,            // Event for WiFi disconnection
    EVENT_BUTTON_LONG_PRESS,            // Event for button long press
    EVENT_BUTTON_SHORT_PRESS,           // Event for button short press
};

// Function prototypes
void events_init(void);
void events_post(int32_t event_id, const void* event_data, size_t event_data_size);
void events_subscribe(int32_t event_id, esp_event_handler_t event_handler, void* event_handler_arg);

#endif // EVENTS_H