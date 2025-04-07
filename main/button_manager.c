#include "button_manager.h"

static const char *TAG = "button_interrupt";

static QueueHandle_t gpio_evt_queue = NULL;
static int64_t press_start_time = 0; 
static int64_t last_isr_time = 0;  // Timestamp of the last ISR call
static TimerHandle_t long_press_timer = NULL;
static bool long_press_detected = false;

// Interrupt service routine (ISR)
static void IRAM_ATTR gpio_isr_handler(void *arg) {
    uint32_t gpio_num = (uint32_t)arg;
    int64_t now = esp_timer_get_time();  // Get current time in microseconds

    // Debounce: Ignore interrupts within X ms
    if (now - last_isr_time > BUTTON_DEBOUNCE_TIME_US) {
        last_isr_time = now;
        xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
    }
}

// Timer callback to check for long press
static void long_press_timer_callback(TimerHandle_t xTimer) {
    int64_t now = esp_timer_get_time();
    int64_t press_duration = now - press_start_time;

    if (long_press_detected) {
        return;
    }

    if (press_duration >= BUTTON_LONG_PRESS_TIME_US && gpio_get_level(BUTTON_GPIO) == 0) {
        ESP_LOGD(TAG, "Long press detected in advance on GPIO %d", BUTTON_GPIO);
        long_press_detected = true;

        // status_led_set(LED_THREE_BLINK);
        // Handle long press action here
        events_post(EVENT_BUTTON_LONG_PRESS, NULL);  // Post long press event
    }
}

// Task to handle button press events
static void button_task(void *arg) {
    uint32_t io_num;
    while (1) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            // Check the button state
            if (gpio_get_level(io_num) == BUTTON_GPIO) {
                press_start_time = esp_timer_get_time();
                long_press_detected = false;

                // Start the long press timer
                xTimerStart(long_press_timer, 0);
            } else {
                int64_t press_duration = esp_timer_get_time() - press_start_time;

                xTimerStop(long_press_timer, 0);

                if (!long_press_detected) {  // If long press wasn't already detected
                    if (press_duration >= BUTTON_LONG_PRESS_TIME_US) {  // Long press
                        ESP_LOGD(TAG, "Long press detected on GPIO %ld", io_num);
                        events_post(EVENT_BUTTON_LONG_PRESS, NULL);  // Post long press event

                        //status_led_set(LED_THREE_BLINK);
                    } else if (press_duration >= BUTTON_DEBOUNCE_TIME_US) {  // Short press
                        ESP_LOGD(TAG, "Short press detected on GPIO %ld", io_num);
                        events_post(EVENT_BUTTON_SHORT_PRESS, NULL);  // Post short press event
                        
                        //led_state_t current_state = status_led_get();
                        //status_led_set(current_state == LED_SLOW_BLINK ? LED_FAST_BLINK : LED_SLOW_BLINK);
                    }
                }
            }
        }
    }
}

void button_init(void) {
    // Configure the button GPIO as input with pull-up
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_ANYEDGE,  // Trigger on falling edge (button press)
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);

    // Create a queue to handle GPIO events
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    // Create the long press timer
    long_press_timer = xTimerCreate("LongPressTimer", pdMS_TO_TICKS(100), pdTRUE, NULL, long_press_timer_callback);

    // Start the button task
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);

    // Install the ISR service
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL3);

    // Attach the interrupt handler
    gpio_isr_handler_add(BUTTON_GPIO, gpio_isr_handler, (void *)BUTTON_GPIO);

    ESP_LOGI(TAG, "Button interrupt initialized on GPIO %d", BUTTON_GPIO);
}