#ifndef CAPTIVE_PORTAL_H
#define CAPTIVE_PORTAL_H

#include "esp_http_server.h"
#include "src/portal.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "esp_netif.h"

void cp_start_http_server(void);
void cp_stop_http_server(void);
void cp_start_dns_server(void);

#endif /* CAPTIVE_PORTAL_H */