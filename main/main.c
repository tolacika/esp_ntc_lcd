#include <stdio.h>
#include <math.h>
#include <string.h>
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "status_led.h"
#include "button_manager.h"
#include "ntc_adc.h"
#include "lcd.h"
#include "nvs_manager.h"
#include "events.h"
#include "wifi_manager.h"
#include "state_manager.h"

void app_main() {
    esp_log_level_set("wifi", ESP_LOG_VERBOSE);
    // Initialize the state manager
    state_initialize(); // Initialize state manager
    nvs_initialize(); // Initialize NVS
    read_running_config(); // Read running configuration
    events_init(); // Initialize event system
    wifi_initialize(); // Initialize WiFi
    
    // Initialize Event Manager
    vTaskDelay(pdMS_TO_TICKS(100)); // Delay to allow event system to stabilize

    // Initialize status LED
    status_led_init();
    status_led_set(LED_OK); // Set LED to OK state
    vTaskDelay(pdMS_TO_TICKS(100)); // Delay to allow LED to stabilize

    // Initialize button
    button_init();
    vTaskDelay(pdMS_TO_TICKS(100)); // Delay to allow button to stabilize

    // Initialize NVS and read configuration
    //nvs_initialize(); // Initialize NVS
    vTaskDelay(pdMS_TO_TICKS(100)); // Delay to allow NVS to stabilize
    //read_running_config(); // Read running configuration
    vTaskDelay(pdMS_TO_TICKS(100)); // Delay to allow config to stabilize

    //return;
    // Initialize thread safety in NTC ADC
    ntc_init_mutex(); // Initialize mutex for thread safety
    vTaskDelay(pdMS_TO_TICKS(100)); // Delay to allow mutex to stabilize

    // Initialize i2c and LCD
    i2c_initialize();
    lcd_initialize();
    vTaskDelay(pdMS_TO_TICKS(100)); // Delay to allow LCD to stabilize

    // Initialize NTC on ADC channels
    ntc_adc_initialize(); // Initialize ADC
    vTaskDelay(pdMS_TO_TICKS(2000)); // Delay to allow ADC to stabilize


    lcd_set_screen_state(LCD_SCREEN_TEMP_AND_STATUS); // Set initial screen state
    
    // Create task to report temperature data
    // xTaskCreatePinnedToCore(report_temperature_task, "report_temperature_task", 4096, NULL, 5, NULL, 1);

    vTaskDelay(pdMS_TO_TICKS(4000)); 

    //wifi_initialize(); // Initialize WiFi
    vTaskDelay(pdMS_TO_TICKS(100)); // Delay to allow WiFi to stabilize
}