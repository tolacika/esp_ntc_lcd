#include <stdio.h>
#include <math.h>
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ntc_adc.h"
#include "lcd.h"

void temperature_task(void *pvParameter) {
    printf("Starting temperature task...\n");
    char buffer[5]; // Buffer to hold the temperature string

    while (1) {
        // Read temperature
        printf("Reading temperature...\n");
        int adc_raw = adc_read();
        float temperature = adc_to_centigrade(adc_raw);

        printf("T: %.2f °C\n", temperature);

        // Update LCD
        lcd_set_cursor(3, 1);
        if (temperature < 0) {
            lcd_write_string("-");
            temperature = -temperature; // Make temperature positive for display
        } else {
            lcd_write_string(" ");
        }

        // Format temperature as a string
        snprintf(buffer, sizeof(buffer), "%4.1f", temperature);
        lcd_write_string(buffer);
        lcd_write_string("C");

        vTaskDelay(pdMS_TO_TICKS(1000)); // 1s delay (10Hz sampling)
    }
}

void app_main() {
    i2c_master_init();
    lcd_init();
    lcd_backlight(true);

    lcd_set_cursor(0, 0);
    lcd_write_string("Hello, ESP32!");
    lcd_set_cursor(0, 1);
    lcd_write_string("T0: N/A C");

    adc_init(); // Initialize ADC
    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay to allow ADC to stabilize
    printf("ADC initialized\n");

    // Create temperature reading task
    xTaskCreate(temperature_task, "temperature_task", 2048, NULL, 5, NULL);
}