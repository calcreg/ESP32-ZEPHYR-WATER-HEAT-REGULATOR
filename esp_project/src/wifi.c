#include "wifi.h"

LOG_MODULE_REGISTER(app_wifi, LOG_LEVEL_INF);

static struct net_mgmt_event_callback scan_cb;
static struct net_mgmt_event_callback wifi_connect_cb;
static struct net_mgmt_event_callback ipv4_addr_cb;

static K_SEM_DEFINE(scan_done_sem, 0, 1);
static K_SEM_DEFINE(dhcp_obtain_sem, 0, 1);

/* Store the best matching target AP parameters found during scan */
static struct {
    char ssid[32];
    uint8_t channel;
    uint8_t security;
    int8_t rssi;
    bool found;
} target_ap = { .rssi = -128, .found = false };

static void scan_result_handler(struct net_mgmt_event_callback *cb,
                                uint32_t mgmt_event,
                                struct net_if *iface)
{
    if (mgmt_event == NET_EVENT_WIFI_SCAN_RESULT) {
        const struct wifi_scan_result *entry = (const struct wifi_scan_result *)cb->info;
        
        LOG_INF("AP Found | SSID: %-32s | RSSI: %3d dBm | Ch: %2d | Security: %d",
                entry->ssid_length ? (char *)entry->ssid : "<Hidden>",
                entry->rssi, entry->channel, entry->security);

        /* Look for our target SSID and pick the one with the strongest RSSI */
        if (strcmp((char *)entry->ssid, WIFI_SSID) == 0) {
            if (entry->rssi > target_ap.rssi) {
                memcpy(target_ap.ssid, entry->ssid, sizeof(target_ap.ssid));
                target_ap.channel = entry->channel;
                target_ap.security = entry->security;
                target_ap.rssi = entry->rssi;
                target_ap.found = true;
            }
        }
    } 
    else if (mgmt_event == NET_EVENT_WIFI_SCAN_DONE) {
        LOG_INF("--- Wi-Fi Scan Complete ---");
        k_sem_give(&scan_done_sem);
    }
}

/*static void net_mgmt_event_handler(struct net_mgmt_event_callback *cb,
                                   uint32_t mgmt_event,
                                   struct net_if *iface)
{
    if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
        const struct wifi_status *status = (const struct wifi_status *)cb->info;
        if (status->status == 0) {
            LOG_INF(">> Wi-Fi Link established successfully!");
        } else {
            LOG_ERR(">> Wi-Fi Association failed: %d", status->status);
        }
    } 
    else if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
        // Grab the assigned IP address from the interface to print it out
        char ip_str[NET_IPV4_ADDR_LEN];
        struct net_if_addr *if_addr = &iface->config.ip.ipv4->unicast[0];
        
        net_addr_ntop(AF_INET, &if_addr->address.in_addr, ip_str, sizeof(ip_str));
        
        LOG_INF(">> DHCP Success! Assigned IP Address: %s", ip_str);
        
        // UNBLOCK THE WAITING THREAD
        k_sem_give(&dhcp_obtain_sem);
    }
}*/
/* L2 Wi-Fi Event Handler */
static void wifi_event_handler(struct net_mgmt_event_callback *cb,
                               uint32_t mgmt_event,
                               struct net_if *iface)
{
    if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
        const struct wifi_status *status = (const struct wifi_status *)cb->info;
        if (status->status == 0) {
            LOG_INF(">> Wi-Fi Link established successfully! Waiting for DHCP lease...");
        } else {
            LOG_ERR(">> Wi-Fi Association failed: %d", status->status);
        }
    }
}

/* L3 IPv4 Event Handler */
static void ipv4_event_handler(struct net_mgmt_event_callback *cb,
                               uint32_t mgmt_event,
                               struct net_if *iface)
{
    if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
        char ip_str[NET_IPV4_ADDR_LEN];
        struct net_if_addr *if_addr = &iface->config.ip.ipv4->unicast[0];
        
        net_addr_ntop(AF_INET, &if_addr->address.in_addr, ip_str, sizeof(ip_str));
        
        LOG_INF(">> DHCP Success! Assigned IP Address: %s", ip_str);
        
        /* UNBLOCK THE WAITING THREAD */
        k_sem_give(&dhcp_obtain_sem);
    }
}

void start_wifi_scan(void) {
    struct net_if *iface = net_if_get_default();

    net_mgmt_init_event_callback(&scan_cb, scan_result_handler,
                                 NET_EVENT_WIFI_SCAN_RESULT | NET_EVENT_WIFI_SCAN_DONE);
    net_mgmt_add_event_callback(&scan_cb);

    LOG_INF("Scanning for nearby Wi-Fi networks...");
    if (net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0) < 0) {
        LOG_ERR("Wi-Fi scan request failed!");
        net_mgmt_del_event_callback(&scan_cb);
        return;
    }

    k_sem_take(&scan_done_sem, K_SECONDS(10));
    net_mgmt_del_event_callback(&scan_cb);
}

void wifi_connect_init(void) {
    
    /* First, scan so we know exactly which channel and security to use */
    start_wifi_scan();

    if (!target_ap.found) {
        LOG_ERR("Target AP %s not found during scan!",WIFI_SSID);
        return;
    }

    LOG_INF("Connecting to best AP -> SSID: %s, Channel: %d, Security: %d, RSSI: %d dBm",
            target_ap.ssid, target_ap.channel, target_ap.security, target_ap.rssi);

    struct wifi_connect_req_params params = {
        .ssid = target_ap.ssid,
        .ssid_length = strlen(target_ap.ssid),
        .psk = WIFI_PSK,
        .psk_length = strlen(WIFI_PSK),
        .channel = target_ap.channel,             /* Lock onto the correct channel */
        .security = target_ap.security,           /* Match the router's exact security mode */
    };

    /* Register separate callbacks for L2 (Wi-Fi) and L3 (IPv4) layers */
    net_mgmt_init_event_callback(&wifi_connect_cb, wifi_event_handler, NET_EVENT_WIFI_CONNECT_RESULT);
    net_mgmt_add_event_callback(&wifi_connect_cb);

    net_mgmt_init_event_callback(&ipv4_addr_cb, ipv4_event_handler, NET_EVENT_IPV4_ADDR_ADD);
    net_mgmt_add_event_callback(&ipv4_addr_cb);

    struct net_if *iface = net_if_get_default();
    if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params)) < 0) {
        LOG_ERR("Wi-Fi connection request failed!");
    }
}

int wifi_wait_for_ip(k_timeout_t timeout) {
    return k_sem_take(&dhcp_obtain_sem, timeout);
}

void disconnect_wifi_securely(void) {
    struct net_if *iface = net_if_get_default();

    LOG_INF("Initiating secure network shutdown...");

    /* 1. Gracefully disconnect from the Wi-Fi Access Point */
    if (net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0) < 0) {
        LOG_WRN("Failed to send Wi-Fi disconnect command.");
    } else {
        LOG_INF("Wi-Fi link dropped.");
    }

    /* 2. Administratively shut down the network interface stack */
    net_if_down(iface);
    LOG_INF("Network interface brought down. Device is now fully isolated (inaccessible from outside).");
}

/**
 * @brief Queries an internet SNTP server and populates the time and date strings.
 * 
 * @param time_buf Pointer to buffer for time (Formatted as: HH'H'MM -> e.g., "17H35")
 * @param time_len Size of time buffer
 * @param date_buf Pointer to buffer for date (Formatted as: DD/MM/YYYY -> e.g., "29/10/2026")
 * @param date_len Size of date buffer
 * @return int 0 on success, negative error code on failure.
 */
int fetch_internet_time_and_date(char *time_buf, size_t time_len, char *date_buf, size_t date_len) {
    struct sntp_time ts;
    int ret;

    LOG_INF("Connecting to SNTP server (%s)...", NTP_SERVER);

    /* Perform a one-shot SNTP query */
    ret = sntp_simple(NTP_SERVER, NTP_TIMEOUT_MS, &ts);
    if (ret < 0) {
        LOG_ERR("Failed to retrieve time from SNTP server (err: %d)", ret);
        return ret;
    }

    /* ts.seconds returns seconds elapsed since January 1, 1970 (Epoch time) */
    time_t epoch_seconds = (time_t)ts.seconds;

    /* 
     * NOTE: epoch_seconds is in UTC. 
     * If you need a local timezone offset, adjust it here in seconds. 
     * Example for UTC+1: epoch_seconds += 3600;
     */

    struct tm timeinfo;
    if (gmtime_r(&epoch_seconds, &timeinfo) == NULL) {
        LOG_ERR("Failed to convert epoch time to calendar time");
        return -1;
    }

    /* Format time as 17H35 */
    snprintf(time_buf, time_len, "%02dH%02d", timeinfo.tm_hour, timeinfo.tm_min);

    /* Format date as 29/10/2026 */
    snprintf(date_buf, date_len, "%02d/%02d/%04d", 
             timeinfo.tm_mday,          /* Day of the month (1-31) */
             timeinfo.tm_mon + 1,       /* Month (0-11 -> add 1) */
             timeinfo.tm_year + 1900);  /* Year since 1900 */

    LOG_INF("Network Time Synced Successfully -> Date: %s | Time: %s", date_buf, time_buf);
    return 0;
}

void print_esp32_mac_address(void)
{
    struct net_if *iface = net_if_get_default();
    if (!iface) {
        LOG_ERR("No default network interface found!");
        return;
    }

    const struct net_linkaddr *link_addr = net_if_get_link_addr(iface);
    if (!link_addr || link_addr->len != 6) {
        LOG_ERR("Failed to retrieve valid MAC address!");
        return;
    }

    LOG_INF("ESP32 Wi-Fi MAC Address: %02X:%02X:%02X:%02X:%02X:%02X",
            link_addr->addr[0],
            link_addr->addr[1],
            link_addr->addr[2],
            link_addr->addr[3],
            link_addr->addr[4],
            link_addr->addr[5]);
}

