#ifndef NVS_MANAGER_H
#define NVS_MANAGER_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "freertos/FreeRTOS.h"

#define SSID_MAX_LEN 20
#define PASS_MAX_LEN 20

#define AP_SSID_KEY "as"
#define AP_PASS_KEY "ap"
#define AP_CHANNEL_KEY "ac"
#define STA_SSID_KEY "ss"
#define STA_PASS_KEY "sp"

typedef struct {
    char ap_ssid[SSID_MAX_LEN];
    char ap_pass[PASS_MAX_LEN];
    int32_t ap_channel;
    char sta_ssid[SSID_MAX_LEN];
    char sta_pass[PASS_MAX_LEN];
} running_config_t;

void nvs_initialize();
void store_string(const char* key, const char* value);
esp_err_t read_string(const char* key, char* value, size_t max_len);
void store_int(const char* key, int32_t value);
esp_err_t read_int(const char* key, int32_t* value);

running_config_t* get_running_config();
void store_running_config();
void read_running_config();

#endif