#include "ntc_adc.h"

static adc_continuous_handle_t adc_handle;
static SemaphoreHandle_t channel_data_mutex;

static int channel_data[6] = { 0, 0, 0, 0, 0, 0 };

int get_channel_data(int channel_index) {
    if (channel_index < 0 || channel_index >= 6) {
        return -1;
    }

    if (xSemaphoreTake(channel_data_mutex, portMAX_DELAY)) {
        int sample = channel_data[channel_index];
        xSemaphoreGive(channel_data_mutex);
        return sample;
    }
    return -1;
}

float adc_to_centigrade(int adc_raw) {
    // Convert ADC value to voltage (12-bit ADC, 0 dB attenuation = 0–1.1V range)
    float voltage_mv = (adc_raw * 1100) / 4095.0; // Convert to mV

    // Calculate NTC resistance
    float R_ntc = R_FIXED * (V_SUPPLY / voltage_mv - 1.0);

    // Convert resistance to temperature using Steinhart-Hart equation
    float t_kelvin = 1.0 / ((1.0 / T0_KELVIN) + (1.0 / NTC_BETA) * log(R_ntc / NTC_R25));
    float temperature = t_kelvin - 273.15; // Convert to Celsius

    return temperature;
}

static void adc_init_thread_safe() {
    // Initialize the mutex
    channel_data_mutex = xSemaphoreCreateMutex();
    if (channel_data_mutex == NULL) {
        printf("Failed to create mutex\n");
        abort();
    }
}

esp_err_t adc_init() {
    // ADC configuration
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 1024,
        .conv_frame_size = 256,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &adc_handle));

    // Configure channels
    adc_continuous_config_t channel_config = {
        .sample_freq_hz = SOC_ADC_SAMPLE_FREQ_THRES_LOW, // Sampling frequency
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
    };

    adc_digi_pattern_config_t patterns[6] = {
        {.atten = ADC_ATTEN_DB_0, .channel = ADC_USED_CHANNEL_1, .unit = ADC_UNIT_1, .bit_width = ADC_BITWIDTH_12},
        {.atten = ADC_ATTEN_DB_0, .channel = ADC_USED_CHANNEL_2, .unit = ADC_UNIT_1, .bit_width = ADC_BITWIDTH_12},
        {.atten = ADC_ATTEN_DB_0, .channel = ADC_USED_CHANNEL_3, .unit = ADC_UNIT_1, .bit_width = ADC_BITWIDTH_12},
        {.atten = ADC_ATTEN_DB_0, .channel = ADC_USED_CHANNEL_4, .unit = ADC_UNIT_1, .bit_width = ADC_BITWIDTH_12},
        {.atten = ADC_ATTEN_DB_0, .channel = ADC_USED_CHANNEL_5, .unit = ADC_UNIT_1, .bit_width = ADC_BITWIDTH_12},
        {.atten = ADC_ATTEN_DB_0, .channel = ADC_USED_CHANNEL_6, .unit = ADC_UNIT_1, .bit_width = ADC_BITWIDTH_12}
    };

    channel_config.pattern_num = 6;
    channel_config.adc_pattern = patterns;

    ESP_ERROR_CHECK(adc_continuous_config(adc_handle, &channel_config));

    adc_init_thread_safe(); // Initialize thread safety

    return ESP_OK;
}

void adc_start_continuous() {
    ESP_ERROR_CHECK(adc_continuous_start(adc_handle));
}

void adc_stop_continuous() {
    ESP_ERROR_CHECK(adc_continuous_stop(adc_handle));
}

void adc_process_data() {
    uint8_t buffer[256];
    adc_digi_output_data_t *data;

    while (1) {
        uint32_t read_size = 0;
        esp_err_t ret = adc_continuous_read(adc_handle, buffer, sizeof(buffer), &read_size, pdMS_TO_TICKS(1000));
        if (ret == ESP_OK) {
            // printf("Read %ld bytes\n", read_size);
            for (int i = 0; i < read_size; i += sizeof(adc_digi_output_data_t)) {
                data = (adc_digi_output_data_t *)&buffer[i];
                if (data->type1.channel >= 8) {
                    continue; // Skip invalid channels
                }
                int index = 0;
                switch (data->type1.channel) {
                    case ADC_USED_CHANNEL_1: index = 0; break;
                    case ADC_USED_CHANNEL_2: index = 1; break;
                    case ADC_USED_CHANNEL_3: index = 2; break;
                    case ADC_USED_CHANNEL_4: index = 3; break;
                    case ADC_USED_CHANNEL_5: index = 4; break;
                    case ADC_USED_CHANNEL_6: index = 5; break;
                    default: continue;
                }
                if (xSemaphoreTake(channel_data_mutex, portMAX_DELAY)) {
                    channel_data[index] = data->type1.data;
                    xSemaphoreGive(channel_data_mutex);
                }
            }
        }
    }
}