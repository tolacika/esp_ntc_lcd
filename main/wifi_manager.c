#include "wifi_manager.h"

EventGroupHandle_t wifi_event_group;
static const char *TAG = "wifi_ap";

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_connect_init(void) {
    running_config_t *config = get_running_config();
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    strncpy((char *)wifi_config.sta.ssid, config->sta_ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, config->sta_pass, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_ap_init(void) {
    char ssid[32] = {0};
    char password[64] = {0};

    // Initialize NVS and read SSID and password
    nvs_initialize();
    read_string(WIFI_AP_SSID_KEY, ssid, sizeof(ssid));
    read_string(WIFI_AP_PASS_KEY, password, sizeof(password));

    // Fallback to default values if not set
    if (strlen(ssid) == 0) {
        strcpy(ssid, CONFIG_DEFAULT_AP_SSID);
    }
    if (strlen(password) == 0) {
        strcpy(password, CONFIG_DEFAULT_AP_PASSWORD);
    }

    ESP_LOGI(TAG, "Starting WiFi AP with SSID: %s, Password: %s", ssid, password);

    // Initialize the TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default WiFi AP
    esp_netif_create_default_wifi_ap();

    // Initialize WiFi with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Configure WiFi AP settings
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "",
            .ssid_len = 0,
            .password = "",
            .max_connection = WIFI_AP_MAX_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .channel = WIFI_AP_CHANNEL,
        },
    };

    strncpy((char *)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid));
    strncpy((char *)wifi_config.ap.password, password, sizeof(wifi_config.ap.password));
    wifi_config.ap.ssid_len = strlen(ssid);

    if (strlen(password) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP started successfully");
}