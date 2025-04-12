#include "captive_portal.h"
#include "esp_log.h"

static const char *TAG = "CAPTIVE_PORTAL";
static const char *DNS_TAG = "DNS_SERVER";
static const char *HTML_CONTENT = PORTAL_SRC;
static httpd_handle_t http_server = NULL;

static void dns_server_task(void *arg) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[512];

    // Create a UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        ESP_LOGE(DNS_TAG, "Failed to create socket");
        vTaskDelete(NULL);
        return;
    }

    // Bind the socket to port 53 (DNS)
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(53);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(DNS_TAG, "Failed to bind socket");
        close(sock);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(DNS_TAG, "DNS server started");

    while (1) {
        // Receive DNS query
        int len = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &addr_len);
        if (len < 0) {
            ESP_LOGE(DNS_TAG, "Error receiving data");
            continue;
        }

        // Check if the received packet is a valid DNS query
        if (len < 12) { // Minimum DNS query size
            ESP_LOGW(DNS_TAG, "Invalid DNS query received");
            continue;
        }

        // Respond to the DNS query
        buffer[2] |= 0x80; // Set response flag
        buffer[3] |= 0x80; // Set authoritative answer flag

        // Set the answer section to point to the ESP32's AP IP address
        uint8_t response[] = {
            0xc0, 0x0c, // Pointer to the query name
            0x00, 0x01, // Type A
            0x00, 0x01, // Class IN
            0x00, 0x00, 0x00, 0x3c, // TTL (60 seconds)
            0x00, 0x04, // Data length
            192, 168, 4, 1 // IP address (192.168.4.1)
        };

        memcpy(buffer + len, response, sizeof(response));
        len += sizeof(response);

        // Send the response
        sendto(sock, buffer, len, 0, (struct sockaddr *)&client_addr, addr_len);
    }

    close(sock);
    vTaskDelete(NULL);
}

void cp_start_dns_server(void) {
    xTaskCreate(dns_server_task, "dns_server_task", 4096, NULL, 5, NULL);
}

static esp_err_t handle_root_get(httpd_req_t *req) {
    httpd_resp_send(req, HTML_CONTENT, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t handle_configure_post(httpd_req_t *req) {
    char buf[128];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            ESP_LOGI(TAG, "Timeout while receiving data");
        }
        return ESP_FAIL;
    }
    buf[ret] = '\0'; // Null-terminate the received data
    ESP_LOGI(TAG, "Received configuration: %s", buf);
    // Process the configuration data here
    return httpd_resp_send(req, "Configuration received", HTTPD_RESP_USE_STRLEN);
}

static esp_err_t handle_captive_check(httpd_req_t *req) {
    // Respond with a minimal HTML page
    const char *response = "<html><head><title>Captive Portal</title></head><body>Redirecting...</body></html>";
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t handle_redirect(httpd_req_t *req) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/");
    httpd_resp_send(req, NULL, 0); // No body for redirect
    return ESP_OK;
}

void cp_start_http_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_start(&http_server, &config);

    httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = handle_root_get,
    };
    httpd_register_uri_handler(http_server, &root_uri);

    httpd_uri_t configure_uri = {
        .uri = "/configure",
        .method = HTTP_POST,
        .handler = handle_configure_post,
    };
    httpd_register_uri_handler(http_server, &configure_uri);

    // Handle captive portal detection URIs
    httpd_uri_t captive_check_uri = {
        .uri = "/generate_204", // Android captive portal check
        .method = HTTP_GET,
        .handler = handle_captive_check,
    };
    httpd_register_uri_handler(http_server, &captive_check_uri);

    httpd_uri_t apple_captive_check_uri = {
        .uri = "/hotspot-detect.html", // iOS captive portal check
        .method = HTTP_GET,
        .handler = handle_captive_check,
    };
    httpd_register_uri_handler(http_server, &apple_captive_check_uri);

    // Handle wildcard URI for redirection
    httpd_uri_t wildcard_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = handle_redirect,
    };
    httpd_register_uri_handler(http_server, &wildcard_uri);
}
void cp_stop_http_server(void) {
    if (http_server) {
        httpd_stop(http_server);
        http_server = NULL;
    }
    ESP_LOGI(TAG, "HTTP server stopped");
}