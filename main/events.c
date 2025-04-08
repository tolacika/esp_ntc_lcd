#include "events.h"
#include "esp_log.h"

static const char *TAG = "events";
static esp_event_loop_handle_t custom_event_loop = NULL; // Custom event loop handle

ESP_EVENT_DEFINE_BASE(CUSTOM_EVENTS); // Define the event base for custom events

static void application_task(void* args)
{
    // Wait to be started by the main task
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    while(1) {
        esp_event_loop_run(custom_event_loop, 100);
        vTaskDelay(10);
    }
}

// Initialize the event system
void events_init(void) {
    esp_event_loop_args_t loop_args = {
        .queue_size = 10, // Adjust the queue size as needed
        .task_name = "custom_evt_loop", // Name of the event loop task
        .task_stack_size = 3072, // Stack size for the event loop task
        .task_priority = uxTaskPriorityGet(NULL), // Priority for the event loop task
        .task_core_id = tskNO_AFFINITY // Core to run the event loop task
    };

    esp_err_t err = esp_event_loop_create(&loop_args, &custom_event_loop);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create custom event loop: %s", esp_err_to_name(err));
    }

    // Create the application task
    TaskHandle_t task_handle;
    ESP_LOGI(TAG, "starting application task");
    xTaskCreate(application_task, "application_task", 3072, NULL, uxTaskPriorityGet(NULL) + 1, &task_handle);

    // Start the application task to run the event handlers
    xTaskNotifyGive(task_handle);
}

// Post an event to the queue
void events_post(int32_t event_id, const void* event_data, size_t event_data_size) {
    if (custom_event_loop == NULL) {
        ESP_LOGE(TAG, "Custom event loop not initialized");
        return;
    }

    esp_err_t err = esp_event_post_to(custom_event_loop, CUSTOM_EVENTS, event_id, event_data, event_data_size, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to post event: %s", esp_err_to_name(err));
    }
}

void events_subscribe(int32_t event_id, esp_event_handler_t event_handler, void* event_handler_arg) {
    if (custom_event_loop == NULL) {
        ESP_LOGE(TAG, "Custom event loop not initialized");
        return;
    }

    esp_err_t err = esp_event_handler_instance_register_with(custom_event_loop, CUSTOM_EVENTS, event_id,
        event_handler, event_handler_arg, NULL);
    if (err != ESP_OK) {    
        ESP_LOGE(TAG, "Failed to subscribe to event: %s", esp_err_to_name(err));
    }
}
