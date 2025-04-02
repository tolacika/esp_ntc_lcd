#include "ntc_adc.h"

static adc_continuous_handle_t adc_handle;
static SemaphoreHandle_t channel_data_mutex;
static int channel_1_downsample = 0;
static int channel_1_downsample_count = 0;
static int channel_2_downsample = 0;
static int channel_2_downsample_count = 0;

int get_channel_1_data() {
    if (xSemaphoreTake(channel_data_mutex, portMAX_DELAY)) {
        int downsample = channel_1_downsample;
        int downsample_count = channel_1_downsample_count;
        xSemaphoreGive(channel_data_mutex);
        channel_1_downsample = 0;
        channel_1_downsample_count = 0;
        if (downsample_count > 0) {
            return downsample / downsample_count; // Return the average value
        } else {
            return 0; // Return 0 if no data is available
        }
    }
    return 0; // Return 0 to indicate failure
}

int get_channel_2_data() {
    if (xSemaphoreTake(channel_data_mutex, portMAX_DELAY)) {
        int downsample = channel_2_downsample;
        int downsample_count = channel_2_downsample_count;
        xSemaphoreGive(channel_data_mutex);
        channel_2_downsample = 0;
        channel_2_downsample_count = 0;
        if (downsample_count > 0) {
            return downsample / downsample_count; // Return the average value
        } else {
            return 0; // Return 0 if no data is available
        }
    }
    return 0; // Return 0 to indicate failure
}

float adc_to_centigrade(int adc_raw) {
    // ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_1, &adc_raw));
    // printf("ADC Raw: %d\n", adc_raw);

    // Convert ADC value to voltage (12-bit ADC, 0 dB attenuation = 0–1.1V range)
    float voltage_mv = (adc_raw * 1100) / 4095.0; // Convert to mV
    // printf("Voltage: %.2f V\n", voltage_mv / 1000.0);

    // Calculate NTC resistance
    float R_ntc = R_FIXED * (V_SUPPLY / voltage_mv - 1.0);
    // printf("NTC Resistance: %.2f Ω\n", R_ntc);

    // Convert resistance to temperature using Steinhart-Hart equation
    float t_kelvin = 1.0 / ((1.0 / T0_KELVIN) + (1.0 / NTC_BETA) * log(R_ntc / NTC_R25));
    float temperature = t_kelvin - 273.15; // Convert to Celsius
    // printf("Temperature: %.2f °C\n", temperature);

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

    adc_digi_pattern_config_t patterns[2] = {
        {.atten = ADC_ATTEN_DB_0, .channel = ADC_USED_CHANNEL_1, .unit = ADC_UNIT_1, .bit_width = ADC_BITWIDTH_12},
        {.atten = ADC_ATTEN_DB_0, .channel = ADC_USED_CHANNEL_2, .unit = ADC_UNIT_1, .bit_width = ADC_BITWIDTH_12},
    };

    channel_config.pattern_num = 2;
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
    int channel_1_sum = 0, channel_2_sum = 0;
    int channel_1_count = 0, channel_2_count = 0;

    while (1) {
        uint32_t read_size = 0;
        esp_err_t ret = adc_continuous_read(adc_handle, buffer, sizeof(buffer), &read_size, pdMS_TO_TICKS(1000));
        if (ret == ESP_OK) {
            // printf("Read %ld bytes\n", read_size);
            for (int i = 0; i < read_size; i += sizeof(adc_digi_output_data_t)) {
                data = (adc_digi_output_data_t *)&buffer[i];
                if (data->type1.channel == ADC_USED_CHANNEL_1) {
                    channel_1_sum += data->type1.data;
                    channel_1_count++;
                } else if (data->type1.channel == ADC_USED_CHANNEL_2) {
                    channel_2_sum += data->type1.data;
                    channel_2_count++;
                }
            }

            if (channel_1_count > 0) {
                int channel_1_avg = channel_1_sum / channel_1_count;
                if (xSemaphoreTake(channel_data_mutex, portMAX_DELAY)) {
                    channel_1_downsample += channel_1_avg;
                    channel_1_downsample_count++;
                    xSemaphoreGive(channel_data_mutex);
                }
            }

            if (channel_2_count > 0) {
                int channel_2_avg = channel_2_sum / channel_2_count;
                if (xSemaphoreTake(channel_data_mutex, portMAX_DELAY)) {
                    channel_2_downsample += channel_2_avg;
                    channel_2_downsample_count++;
                    xSemaphoreGive(channel_data_mutex);
                }
            }

            channel_1_sum = channel_2_sum = 0;
            channel_1_count = channel_2_count = 0;
        }
    }
}