#include <stdio.h>
#include <math.h>
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ntc_adc.h"
#include "lcd.h"

void temperature_task(void *pvParameter) {
    adc_start_continuous();
    adc_process_data();
}

void lcd_update_task(void *pvParameter) {
    char buffer[5];

    while (1)
    {
        int channel_1_adc_raw = get_channel_1_data();
        int channel_2_adc_raw = get_channel_2_data();
        float temp = adc_to_centigrade(channel_1_adc_raw);
        lcd_set_cursor(3, 1);
        snprintf(buffer, sizeof(buffer), "%.2f", temp);
        lcd_write_string(buffer);
        lcd_write_string(" C");
        lcd_set_cursor(3, 2);
        temp = adc_to_centigrade(channel_2_adc_raw);
        snprintf(buffer, sizeof(buffer), "%.2f", temp);
        lcd_write_string(buffer);
        lcd_write_string(" C");
        vTaskDelay(pdMS_TO_TICKS(1000)); // Update every second
    }
    
}

void app_main() {
    i2c_master_init();
    lcd_init();
    lcd_backlight(true);

    lcd_set_cursor(0, 0);
    lcd_write_string("Hello, ESP32!");
    lcd_set_cursor(0, 1);
    lcd_write_string("T1: N/A C");
    lcd_set_cursor(0, 2);
    lcd_write_string("T2: N/A C");

    adc_init(); // Initialize ADC
    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay to allow ADC to stabilize
    printf("ADC initialized\n");

    // Create temperature reading task
    xTaskCreatePinnedToCore(temperature_task, "temperature_task", 4096, NULL, 5, NULL, 1);
    // Create LCD update task
    xTaskCreatePinnedToCore(lcd_update_task, "lcd_update_task", 4096, NULL, 5, NULL, 0);
}