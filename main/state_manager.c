#include "state_manager.h"

running_state_t running_state = {
    .wifi_sta_connection_state = 0,
    .wifi_ap_connection_state = 0,
    .wifi_ap_mode = false,
};

void state_initialize(void) {
    running_state.wifi_sta_connection_state = 0;
    running_state.wifi_ap_connection_state = 0;
    running_state.wifi_ap_mode = false;
}

// considering thread safety
void state_set_wifi_sta_connection_state(uint8_t state) {
    running_state.wifi_sta_connection_state = state;
}

void state_set_wifi_ap_connection_state(uint8_t state) {
    running_state.wifi_ap_connection_state = state;
}

void state_set_wifi_ap_mode(bool mode) {
    running_state.wifi_ap_mode = mode;
}

uint8_t state_get_wifi_sta_connection_state(void) {
    return running_state.wifi_sta_connection_state;
}

uint8_t state_get_wifi_ap_connection_state(void) {
    return running_state.wifi_ap_connection_state;
}

bool state_get_wifi_ap_mode(void) {
    return running_state.wifi_ap_mode;
}


