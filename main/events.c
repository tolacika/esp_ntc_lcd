#include "events.h"
#include "esp_log.h"

static const char *TAG = "events";

static EventGroupHandle_t event_group;
static QueueHandle_t event_queue;

static event_subscription_t *subs_head = NULL;
static event_subscription_t *subs_tail = NULL;

#define EVENT_QUEUE_SIZE 10

// Task to process events
static void event_task(void *pvParameter) {
    event_t event;
    while (1) {
        if (xQueueReceive(event_queue, &event, portMAX_DELAY)) {
            // Call type-specific callbacks
            event_subscription_t *current = subs_head;
            while (current != NULL) {
                if (current->type == event.type) {
                    current->callback(&event);
                }
                current = current->next;
            }

            switch (event.type) {
                case EVENT_WIFI_CONNECTED:
                    ESP_LOGI(TAG, "WiFi connected");
                    xEventGroupSetBits(event_group, WIFI_CONNECTED_BIT);
                    break;

                case EVENT_WIFI_DISCONNECTED:
                    ESP_LOGI(TAG, "WiFi disconnected");
                    xEventGroupClearBits(event_group, WIFI_CONNECTED_BIT);
                    break;

                case EVENT_BUTTON_LONG_PRESS:
                    ESP_LOGI(TAG, "Long button press detected");
                    break;

                case EVENT_BUTTON_SHORT_PRESS:
                    ESP_LOGI(TAG, "Short button press detected");
                    break;

                default:
                    ESP_LOGW(TAG, "Unknown event type: %d", event.type);
                    break;
            }
        }
    }
}

// Initialize the event system
void events_init(void) {
    event_group = xEventGroupCreate();
    event_queue = xQueueCreate(EVENT_QUEUE_SIZE, sizeof(event_t));
    xTaskCreate(event_task, "event_task", 4096, NULL, 5, NULL);
}

// Post an event to the queue
void events_post(event_type_t type, void *data) {
    event_t event = {
        .type = type,
        .data = data
    };
    xQueueSend(event_queue, &event, portMAX_DELAY);
}


void events_subscribe(event_type_t type, void (*callback)(event_t *event)) {
    event_subscription_t *new_sub = (event_subscription_t *)malloc(sizeof(event_subscription_t));
    if (new_sub == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for event subscription");
        return;
    }
    new_sub->type = type;
    new_sub->callback = callback;
    new_sub->next = NULL;

    if (subs_head == NULL) {
        subs_head = new_sub;
        subs_tail = new_sub;
    } else {
        subs_tail->next = new_sub;
        subs_tail = new_sub;
    }
}

void events_unsubscribe(event_type_t type, void (*callback)(event_t *event)) {
    event_subscription_t *current = subs_head;
    event_subscription_t *previous = NULL;

    while (current != NULL) {
        if (current->type == type && current->callback == callback) {
            if (previous == NULL) {
                subs_head = current->next;
            } else {
                previous->next = current->next;
            }
            free(current);
            break;
        }
        previous = current;
        current = current->next;
    }
}

// Get the event group handle
EventGroupHandle_t get_event_group(void) {
    return event_group;
}