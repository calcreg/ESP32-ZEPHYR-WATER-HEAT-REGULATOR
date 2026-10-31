#include <time.h>
#include "state.h"
#include <zephyr/posix/time.h> // Depending on Zephyr includes, or standard <time.h>

void update_system_time_from_sntp(time_t sntp_epoch_seconds) {
    struct timespec ts;
    
    ts.tv_sec = sntp_epoch_seconds; /* Time received from your SNTP query */
    ts.tv_nsec = 0;                 /* Milliseconds/nanoseconds offset if available */

    /* Set the ESP32 system clock (POSIX CLOCK_REALTIME) */
    clock_settime(CLOCK_REALTIME, &ts);
}

void refresh_local_time(void) {
    time_t now;
    struct tm timeinfo;

    /* Read the current system time maintained by the ESP32 */
    time(&now);
    
    /* Convert epoch time to local calendar time struct */
    localtime_r(&now, &timeinfo);

    /* Format it into your app state buffer (e.g., "14H32") */
    snprintf(app_state.CurrentTime, sizeof(app_state.CurrentTime), "%02dH%02d", 
             timeinfo.tm_hour, timeinfo.tm_min);
             
    /* If you also store the date: */
    snprintf(app_state.CurrentDate, sizeof(app_state.CurrentDate), "%02d/%02d/%04d", 
             timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
}
