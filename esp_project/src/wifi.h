

#ifndef WIFI_H
#define WIFI_H

#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/sntp.h>
#include <time.h>
#include <stdio.h>
#include <zephyr/net/net_linkaddr.h>
#include <zephyr/net/net_ip.h>

#define NTP_SERVER "pool.ntp.org"
#define NTP_TIMEOUT_MS 5000 // 5 seconds timeout

#define WIFI_SSID "********"
#define WIFI_PSK  "********"

/* External prototype from get_date.c */
int fetch_internet_time_and_date(char *time_buf, size_t time_len, char *date_buf, size_t date_len);

void wifi_connect_init(void);
int wifi_wait_for_ip(k_timeout_t timeout);
void disconnect_wifi_securely(void) ;

/* Add the scan prototype here */
void start_wifi_scan(void);

void print_esp32_mac_address(void);

#endif
