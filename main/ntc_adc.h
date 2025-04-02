#ifndef NTC_ADC_H
#define NTC_ADC_H

#include <stdio.h>
#include <math.h>
#include "esp_adc/adc_continuous.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h" // Include for GPIO functionality

#define ADC_USED_CHANNEL_1  ADC_CHANNEL_0  // ADC1 Channel 0
#define ADC_USED_CHANNEL_2  ADC_CHANNEL_3  // ADC1 Channel 3

#define R_FIXED 10000.0            // 10kΩ fixed resistor
#define V_SUPPLY 3300.0            // Supply voltage in mV
#define NTC_BETA 3950.0            // Beta value for NTC thermistor
#define NTC_R25 100000.0            // Resistance at 25°C in ohms
#define T0_KELVIN 298.15           // 25°C in Kelvin

float adc_to_centigrade(int adc_raw); // Changed from static to extern
esp_err_t adc_init();
void adc_start_continuous();
void adc_stop_continuous();
void adc_process_data();
int get_channel_1_data();
int get_channel_2_data();

#endif // NTC_ADC_H