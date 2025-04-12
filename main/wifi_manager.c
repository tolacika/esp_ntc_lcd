#include "wifi_manager.h"

//EventGroupHandle_t wifi_event_group;
static const char *TAG = "wifi_ap";
static bool ap_enabled = false;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;

            case WIFI_EVENT_STA_DISCONNECTED: {
                wifi_event_sta_disconnected_t *disconnected = (wifi_event_sta_disconnected_t *)event_data;
                ESP_LOGW(TAG, "WiFi disconnected, reason: %d", disconnected->reason);

                switch (disconnected->reason) {
                    case WIFI_REASON_NO_AP_FOUND:
                        ESP_LOGE(TAG, "SSID not found");
                        break;
                    case WIFI_REASON_AUTH_FAIL:
                        ESP_LOGE(TAG, "Authentication failed");
                        break;
                    default:
                        ESP_LOGE(TAG, "Disconnected for unknown reason %d", disconnected->reason);
                        break;
                }

                // Trigger EVENT_WIFI_DISCONNECTED
                events_post(EVENT_WIFI_DISCONNECTED, &disconnected->reason, sizeof(disconnected->reason));

                vTaskDelay(pdMS_TO_TICKS(5000)); // Wait before retrying
                ESP_LOGI(TAG, "Retrying connection...");

                //esp_wifi_connect(); // Retry connection
                break;
            }
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        // Trigger EVENT_WIFI_CONNECTED
        events_post(EVENT_WIFI_CONNECTED, &event->ip_info.ip, sizeof(event->ip_info.ip));
    }
}

void wifi_sta_init(void) {
    running_config_t *config = get_running_config();

    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default WiFi STA
    esp_netif_create_default_wifi_sta();

    // Initialize WiFi with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

    // Configure WiFi STA settings
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

    ESP_LOGI(TAG, "WiFi STA initialized and connecting...");
}

void enable_ap_mode(void) {
    running_config_t *config = get_running_config();

    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap(); // Set IP address for the access point
    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.gw, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
    ESP_ERROR_CHECK(esp_netif_dhcps_stop(ap_netif)); // Stop DHCP server before setting IP
    ESP_ERROR_CHECK(esp_netif_set_ip_info(ap_netif, &ip_info));
    ESP_ERROR_CHECK(esp_netif_dhcps_start(ap_netif)); // Restart DHCP server


    // Configure WiFi AP settings
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "",
            .ssid_len = 0,
            .password = "",
            .max_connection = WIFI_AP_MAX_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .channel = config->ap_channel,
        },
    };

    strncpy((char *)wifi_config.ap.ssid, config->ap_ssid, sizeof(wifi_config.ap.ssid));
    strncpy((char *)wifi_config.ap.password, config->ap_pass, sizeof(wifi_config.ap.password));
    wifi_config.ap.ssid_len = strlen(config->ap_ssid);

    if (strlen(config->ap_pass) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP enabled with SSID: %s", config->ap_ssid);

    // Starting DNS server
    cp_start_dns_server();
    ESP_LOGI(TAG, "DNS server started");

    // Starting captive portal
    cp_start_http_server();
}

static void _wifi_button_long_press_event_handler(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data) {
    ESP_LOGI(TAG, "Long press detected, switching to AP mode...");
    if (!ap_enabled) {
        ap_enabled = true;
        ESP_LOGI(TAG, "Enabling AP mode...");
        enable_ap_mode();
    } else {
        // Disable AP mode and restart the device
        ESP_LOGI(TAG, "Configuration is done, restarting in 5 seconds...");
        vTaskDelay(pdMS_TO_TICKS(5000));
        esp_restart(); // Restart the device
    }
}

void wifi_initialize() {
    wifi_sta_init();

    events_subscribe(EVENT_BUTTON_LONG_PRESS, _wifi_button_long_press_event_handler, NULL);
}
