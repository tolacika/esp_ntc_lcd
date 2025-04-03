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

void format_temperature(float temp, char *buffer, size_t buffer_size) {
    // printf("Formatting temperature: %f\n", temp);
    if (buffer_size < 5) {
        return; // Buffer too small
    }
    if (temp < 0) {
        buffer[0] = '-';
        temp = -temp;
    } else {
        buffer[0] = temp >= 100 ? ((int)(temp / 100) % 10) + '0' : ' ';
    }
    buffer[1] = temp < 10 ? ' ' : (int)(temp / 10) % 10 + '0';
    buffer[2] = (int)temp % 10 + '0';
    buffer[3] = '.';
    buffer[4] = (int)(temp * 10) % 10 + '0';
}

void lcd_layout(float *temps, size_t size) {
    if (size != 6) {
        return; // Not enough space for 6 temperatures
    }
    //lcd_clear();
    char buffer[6] = {0};
    float min_temp = 200.0;
    float max_temp = -20.0;
    float avg_temp = 0.0;

    for (int i = 0; i < size; i++) {
        if (temps[i] < min_temp) {
            min_temp = temps[i];
        }
        if (temps[i] > max_temp) {
            max_temp = temps[i];
        }
        avg_temp += temps[i];
        lcd_set_cursor(i < 3 ? 0 : 11, i % 3);
        lcd_write_char('T');
        lcd_write_char((i + 1) + '0');
        lcd_write_char(':');
        format_temperature(temps[i], buffer, sizeof(buffer));
        lcd_write_string(buffer);
        lcd_write_char('C');
    }
    avg_temp /= size;
    lcd_set_cursor(0, 3);
    format_temperature(min_temp, buffer, sizeof(buffer));
    lcd_write_string(buffer);
    lcd_write_char('C');
    lcd_write_char('<');
    format_temperature(avg_temp, buffer, sizeof(buffer));
    lcd_write_string(buffer);
    lcd_write_char('C');
    lcd_write_char('<');
    format_temperature(max_temp, buffer, sizeof(buffer));
    lcd_write_string(buffer);
    lcd_write_char('C');
}

void lcd_update_task(void *pvParameter) {
    float temps[6];

    while (1)
    {
        for (int i = 0; i < 6; i++) {
            temps[i] = adc_to_centigrade(get_channel_data(i));
        }
        lcd_layout(temps, 6);
        vTaskDelay(pdMS_TO_TICKS(500)); // Update every second
    }
    
}

void report_temperature_task(void *pvParameter) {
    while (1) {
        float temp = adc_to_centigrade(get_channel_data(1));
        printf("%.2f\n", temp);
        vTaskDelay(pdMS_TO_TICKS(100)); // Report every second
    }
}

void app_main() {
    i2c_master_init();
    lcd_init();
    lcd_backlight(true);
    lcd_clear();
    float initial_temps[6] = {0, 0, 0, 0, 0, 0};
    lcd_layout(initial_temps, 6); // Initialize LCD with zeros

    adc_init(); // Initialize ADC
    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay to allow ADC to stabilize
    printf("ADC initialized\n");

    // Create temperature reading task
    xTaskCreatePinnedToCore(temperature_task, "temperature_task", 4096, NULL, 5, NULL, 1);
    // Create LCD update task
    xTaskCreatePinnedToCore(lcd_update_task, "lcd_update_task", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(report_temperature_task, "report_temperature_task", 4096, NULL, 5, NULL, 1);
}