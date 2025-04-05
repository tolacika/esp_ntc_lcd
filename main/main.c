#include <stdio.h>
#include <math.h>
#include <string.h>
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ntc_adc.h"
#include "lcd.h"

void app_main() {
    vTaskDelay(pdMS_TO_TICKS(100)); // Delay to allow system to stabilize
    i2c_initialize();
    lcd_initialize();
    lcd_toggle_backlight(true);
    lcd_clear_buffer();
    lcd_render();
    float initial_temps[6] = {0, 0, 0, 0, 0, 0};
    lcd_display_temperatures(initial_temps, 6); // Initialize LCD with zeros

    ntc_adc_initialize(); // Initialize ADC
    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay to allow ADC to stabilize
    printf("ADC initialized\n");

    // Create temperature reading task
    xTaskCreatePinnedToCore(ntc_temperature_task, "temperature_task", 4096, NULL, 5, NULL, 1);
    // Create LCD update task
    xTaskCreatePinnedToCore(lcd_update_task, "lcd_update_task", 4096, NULL, 5, NULL, 0);
    // Create task to report temperature data
    // xTaskCreatePinnedToCore(report_temperature_task, "report_temperature_task", 4096, NULL, 5, NULL, 1);
}