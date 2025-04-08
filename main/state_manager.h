#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    uint8_t wifi_sta_connection_state; // see wifi_err_reason_t
    uint8_t wifi_ap_connection_state; // see wifi_err_reason_t
    bool wifi_ap_mode; // 0 = off, 1 = on
} running_state_t;

extern running_state_t running_state;

void state_initialize(void);
void state_set_wifi_sta_connection_state(uint8_t state);
void state_set_wifi_ap_connection_state(uint8_t state);
void state_set_wifi_ap_mode(bool mode);
uint8_t state_get_wifi_sta_connection_state(void);
uint8_t state_get_wifi_ap_connection_state(void);
bool state_get_wifi_ap_mode(void);

#endif // STATE_MANAGER_H