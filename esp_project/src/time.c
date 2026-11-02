#include <time.h>
#include "state.h"
#include <zephyr/posix/time.h> // Depending on Zephyr includes, or standard <time.h>
#include <stdbool.h>


void update_system_time_from_sntp(time_t sntp_epoch_seconds) {
    struct timespec ts;
    
    ts.tv_sec = sntp_epoch_seconds; /* Time received from your SNTP query */
    ts.tv_nsec = 0;                 /* Milliseconds/nanoseconds offset if available */

    /* Set the ESP32 system clock (POSIX CLOCK_REALTIME) */
    clock_settime(CLOCK_REALTIME, &ts);
}


/* Check if Daylight Saving Time (CEST, UTC+2) is active in France */
static bool is_france_dst(const struct tm *utc_t) {
    int month = utc_t->tm_mon + 1; // tm_mon is 0-11 (Jan = 1)
    int day = utc_t->tm_mday;
    int year = utc_t->tm_year + 1900;

    // Winter months (Nov, Dec, Jan, Feb) are always CET (UTC+1)
    if (month < 3 || month > 10) return false;
    // Summer months (Apr through Sep) are always CEST (UTC+2)
    if (month > 3 && month < 10) return true;

    // Last Sunday of March calculation (DST starts at 01:00 UTC)
    int last_sun_mar = 31 - (((5 * year / 4) + 4) % 7);
    if (month == 3) {
        if (day > last_sun_mar) return true;
        if (day == last_sun_mar && utc_t->tm_hour >= 1) return true;
        return false;
    }

    // Last Sunday of October calculation (DST ends at 01:00 UTC)
    int last_sun_oct = 31 - (((5 * year / 4) + 1) % 7);
    if (month == 10) {
        if (day < last_sun_oct) return true;
        if (day == last_sun_oct && utc_t->tm_hour < 1) return true;
        return false;
    }

    return false;
}

void refresh_local_time(void) {
    time_t now;
    struct tm utc_tm;

    /* Read the current system time maintained by the ESP32 (UTC from SNTP) */
    time(&now);
    
    /* Get raw UTC calendar time struct safely */
    if (!gmtime_r(&now, &utc_tm)) return;

    /* Determine seasonal offset dynamically (+2h in summer, +1h in winter) */
    int offset_hours = is_france_dst(&utc_tm) ? 2 : 1;

    /* Copy UTC structure to local structure and apply offset */
    struct tm local_tm = utc_tm;
    local_tm.tm_hour += offset_hours;

    /* 
     * mktime normalizes the structure, automatically rolling over 
     * hours, days, months, and years if the offset pushes past midnight.
     */
    mktime(&local_tm);

    /* Format time into your app state buffer (e.g., "14H32") */
    snprintf(app_state.CurrentTime, sizeof(app_state.CurrentTime), "%02dH%02d", 
             local_tm.tm_hour, local_tm.tm_min);
             
    /* Store the adjusted local date (e.g., "02/11/2026") */
    snprintf(app_state.CurrentDate, sizeof(app_state.CurrentDate), "%02d/%02d/%04d", 
             local_tm.tm_mday, local_tm.tm_mon + 1, local_tm.tm_year + 1900);
}