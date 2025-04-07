#include "nvs_manager.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "nvs_manager";

static running_config_t running_config = {
    .ap_ssid = CONFIG_DEFAULT_AP_SSID,
    .ap_pass = CONFIG_DEFAULT_AP_PASSWORD,
    .sta_ssid = CONFIG_DEFAULT_STA_SSID,
    .sta_pass = CONFIG_DEFAULT_STA_PASSWORD,
    .ap_channel = CONFIG_DEFAULT_AP_CHANNEL,
};

void nvs_initialize()
{
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
}

void store_string(const char *key, const char *value)
{
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err == ESP_OK)
  {
    err = nvs_set_str(nvs_handle, key, value);
    if (err == ESP_OK)
    {
      nvs_commit(nvs_handle);
    }
    nvs_close(nvs_handle);
  }
}

esp_err_t read_string(const char *key, char *value, size_t max_len)
{
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs_handle);
  if (err == ESP_OK) {
    size_t required_size;
    err = nvs_get_str(nvs_handle, key, NULL, &required_size);
    if (err == ESP_OK) {
      if (required_size <= max_len) {
        nvs_get_str(nvs_handle, key, value, &required_size);
      } else {
        return ESP_ERR_NVS_INVALID_LENGTH;
      }
    } else {
      return err;
    }
    nvs_close(nvs_handle);
  } else {
    return err;
  }

  return ESP_OK;
}

void store_int(const char *key, int32_t value)
{
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err == ESP_OK)
  {
    err = nvs_set_i32(nvs_handle, key, value);
    if (err == ESP_OK)
    {
      nvs_commit(nvs_handle);
    }
    nvs_close(nvs_handle);
  }
}

esp_err_t read_int(const char *key, int32_t *value)
{
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs_handle);
  if (err == ESP_OK)
  {
    nvs_get_i32(nvs_handle, key, value);
    nvs_close(nvs_handle);
  } else {
    return err;
  }

  return ESP_OK;
}

running_config_t *get_running_config()
{
  return &running_config;
}

void store_running_config()
{
  store_string(AP_SSID_KEY, running_config.ap_ssid);
  store_string(AP_PASS_KEY, running_config.ap_pass);
  store_string(STA_SSID_KEY, running_config.sta_ssid);
  store_string(STA_PASS_KEY, running_config.sta_pass);
  store_int(AP_CHANNEL_KEY, running_config.ap_channel);
}
void read_running_config()
{
  esp_err_t err = read_string(AP_SSID_KEY, running_config.ap_ssid, sizeof(running_config.ap_ssid));
  if (err == ESP_ERR_NVS_NOT_FOUND) {
    ESP_LOGI(TAG, "AP SSID not found, using default: %s", CONFIG_DEFAULT_AP_SSID);
    store_string(AP_SSID_KEY, CONFIG_DEFAULT_AP_SSID);
    strncpy(running_config.ap_ssid, CONFIG_DEFAULT_AP_SSID, sizeof(running_config.ap_ssid));
  }
  
  err = read_string(AP_PASS_KEY, running_config.ap_pass, sizeof(running_config.ap_pass));
  if (err == ESP_ERR_NVS_NOT_FOUND) {
    ESP_LOGI(TAG, "AP Password not found, using default: %s", CONFIG_DEFAULT_AP_PASSWORD);
    store_string(AP_PASS_KEY, CONFIG_DEFAULT_AP_PASSWORD);
    strncpy(running_config.ap_pass, CONFIG_DEFAULT_AP_PASSWORD, sizeof(running_config.ap_pass));
  }

  err = read_string(STA_SSID_KEY, running_config.sta_ssid, sizeof(running_config.sta_ssid));
  if (err == ESP_ERR_NVS_NOT_FOUND) {
    ESP_LOGI(TAG, "STA SSID not found, using default: %s", CONFIG_DEFAULT_STA_SSID);
    store_string(STA_SSID_KEY, CONFIG_DEFAULT_STA_SSID);
    strncpy(running_config.sta_ssid, CONFIG_DEFAULT_STA_SSID, sizeof(running_config.sta_ssid));
  }

  err = read_string(STA_PASS_KEY, running_config.sta_pass, sizeof(running_config.sta_pass));
  if (err == ESP_ERR_NVS_NOT_FOUND) {
    ESP_LOGI(TAG, "STA Password not found, using default: %s", CONFIG_DEFAULT_STA_PASSWORD);
    store_string(STA_PASS_KEY, CONFIG_DEFAULT_STA_PASSWORD);
    strncpy(running_config.sta_pass, CONFIG_DEFAULT_STA_PASSWORD, sizeof(running_config.sta_pass));
  }

  read_int(AP_CHANNEL_KEY, &running_config.ap_channel);
  if (err == ESP_ERR_NVS_NOT_FOUND) {
    ESP_LOGI(TAG, "AP Channel not found, using default: %d", CONFIG_DEFAULT_AP_CHANNEL);
    store_int(AP_CHANNEL_KEY, CONFIG_DEFAULT_AP_CHANNEL);
    running_config.ap_channel = CONFIG_DEFAULT_AP_CHANNEL;
  }
  // Validate channel range (1-13)
  if (running_config.ap_channel < 1 || running_config.ap_channel > 13)
  {
    running_config.ap_channel = 1; // Default channel
  }

  ESP_LOGI(TAG, "Stored running config:\n\tAP SSID:\t\"%s\",\n\tAP Pass:\t\"%s\",\n\tSTA SSID:\t\"%s\",\n\tSTA Pass:\t\"%s\",\n\tAP Channel:\t%ld",
           running_config.ap_ssid, running_config.ap_pass, running_config.sta_ssid, running_config.sta_pass, running_config.ap_channel);
}