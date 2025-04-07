#ifndef EVENTS_H
#define EVENTS_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "nvs_manager.h"

// Event group bits
#define WIFI_CONNECTED_BIT   (1 << 0)

// Event types
typedef enum {
    EVENT_WIFI_CONNECTED,
    EVENT_WIFI_DISCONNECTED,
    EVENT_BUTTON_LONG_PRESS,
    EVENT_BUTTON_SHORT_PRESS,
} event_type_t;

// Event structure
typedef struct {
    event_type_t type;
    void *data; // Optional data associated with the event
} event_t;

// Forward declaration of event_subscription_t
typedef struct event_subscription_t event_subscription_t;

struct event_subscription_t{
    event_type_t type;
    void (*callback)(event_t *event);
    event_subscription_t *next; // Pointer to the next subscription in the list
};

// Function prototypes
void events_init(void);
void events_post(event_type_t type, void *data);
EventGroupHandle_t get_event_group(void);
void events_subscribe(event_type_t type, void (*callback)(event_t *event));
void events_unsubscribe(event_type_t type, void (*callback)(event_t *event));

#endif // EVENTS_H