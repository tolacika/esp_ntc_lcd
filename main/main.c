#include <stdio.h>
#include <math.h>
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h" // Include for GPIO functionality

// Constants
#define ADC_CHANNEL ADC_CHANNEL_0  // ADC1 Channel 0
#define R_FIXED 10000.0            // 10kΩ fixed resistor
#define V_SUPPLY 3300.0            // Supply voltage in mV
#define NTC_BETA 3950.0            // Beta value for NTC thermistor
#define NTC_R25 100000.0            // Resistance at 25°C in ohms
#define T0_KELVIN 298.15           // 25°C in Kelvin
#define NUM_SAMPLES 10             // Number of samples for rolling average

// Variables
float temp_buffer[NUM_SAMPLES] = {0};
int buffer_index = 0;
adc_oneshot_unit_handle_t adc1_handle;

// Function to read temperature
float read_temperature() {
    int adc_raw;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL, &adc_raw));
    printf("ADC Raw: %d\n", adc_raw);

    // Convert ADC value to voltage (12-bit ADC, 0 dB attenuation = 0–1.1V range)
    float voltage_mv = (adc_raw * 1100) / 4095.0; // Convert to mV
    printf("Voltage: %.2f V\n", voltage_mv / 1000.0);

    // Calculate NTC resistance
    float R_ntc = R_FIXED * (V_SUPPLY / voltage_mv - 1.0);
    printf("NTC Resistance: %.2f Ω\n", R_ntc);

    // Convert resistance to temperature using Steinhart-Hart equation
    float t_kelvin = 1.0 / ((1.0 / T0_KELVIN) + (1.0 / NTC_BETA) * log(R_ntc / NTC_R25));
    float temperature = t_kelvin - 273.15; // Convert to Celsius
    printf("Temperature: %.2f °C\n", temperature);

    return temperature;
}

// Function to calculate rolling average
float get_rolling_average() {
    float sum = 0.0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        sum += temp_buffer[i];
    }
    return sum / NUM_SAMPLES;
}

// Task to read temperature and compute rolling average
void temperature_task(void *pvParameter) {
    while (1) {
        // Read temperature
        float temperature = read_temperature();

        printf("T: %.2f °C\n", temperature);

        vTaskDelay(pdMS_TO_TICKS(1000)); // 100ms delay (10Hz sampling)
    }
}

void app_main() {
    // Configure GPIO for ADC1 Channel 0
    gpio_num_t adc_gpio = GPIO_NUM_36;
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << adc_gpio),
        .mode = GPIO_MODE_DISABLE, // ADC pins are configured as disabled GPIOs
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // Configure ADC
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config, &adc1_handle);

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT, 

        .atten = ADC_ATTEN_DB_0, // 11 dB attenuation for 0–3.3V range
    };
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &config);

    // Create temperature reading task
    xTaskCreate(temperature_task, "temperature_task", 2048, NULL, 5, NULL);
}