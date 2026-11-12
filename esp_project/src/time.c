#include <time.h>
#include "state.h"
//#include <zephyr/posix/time.h> // Depending on Zephyr includes, or standard <time.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <ctype.h>

LOG_MODULE_REGISTER(app_time, LOG_LEVEL_INF);

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
    if (!gmtime_r(&now, &utc_tm)) {
        LOG_INF("Cannot get raw calendar time");
        return;
    }

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

/*In case Wifi cannot retrieve time from sntp, we'll do it manually 
and integrate new values here*/
void update_system_time_from_strings(int TimeOrDate) {
    time_t now;
    struct tm local_tm = {0};
  
    /* Read the current system time maintained by the ESP32 (UTC from SNTP) */
    time(&now);
    
    /* Get raw UTC calendar time struct safely */
    if (!gmtime_r(&now, &local_tm)) {
        LOG_INF("Cannot get raw calendar time");
        return;
    }

    int hour = 0, minute = 0;
    int day = 1, month = 1, year = 2026;
    
    /* 1. Parse CurrentTime (supports formats like "14H32" or "14:32") */
    /* sscanf crash esp32 we need another way*/
    if (TimeOrDate == 0) {
        hour   = (app_state.CurrentTime[0] - '0') * 10 + (app_state.CurrentTime[1] - '0');
        minute = (app_state.CurrentTime[3] - '0') * 10 + (app_state.CurrentTime[4] - '0');
        if (hour < 0 || hour > 23 || minute < 0 || minute > 59 ){
            LOG_INF("Error in retrieval Time : %2dH%2d", hour, minute);
            return;
        }
        /* 3. Populate a tm struct with the local French time */
        local_tm.tm_hour = hour;
        local_tm.tm_min = minute;
        local_tm.tm_sec = 0;
    }else{
        day   = (app_state.CurrentDate[0] - '0') * 10 + (app_state.CurrentDate[1] - '0');
        month = (app_state.CurrentDate[3] - '0') * 10 + (app_state.CurrentDate[4] - '0');
        year  = (app_state.CurrentDate[6] - '0') * 1000 + 
            (app_state.CurrentDate[7] - '0') * 100 + 
            (app_state.CurrentDate[8] - '0') * 10 + 
            (app_state.CurrentDate[9] - '0');
         /* Validate ranges to prevent calendar faults */
        if (day < 1 || day > 31 || month < 1 || month > 12 || year < 2020) {
            LOG_INF("Error in retrieval Date : %2d/%2d/%4d",day,month,year);
            return;
        }
        local_tm.tm_mday = day;
        local_tm.tm_mon = month - 1;     // tm_mon is 0-11
        local_tm.tm_year = year - 1900;  // tm_year is years since 1900
        local_tm.tm_isdst = -1;
    }
    //LOG_INF("Error in retrieval date : %2dH%2d %2d/%2d/%4d", hour, minute, day,month,year);

    /* 4. Determine the active French offset for this date (+2h summer, +1h winter) */
    int offset_hours = is_france_dst(&local_tm) ? 2 : 1;

    /* 5. Convert local time to UTC by subtracting the offset */
    local_tm.tm_hour -= offset_hours;

    /* 
     * 6. Convert the adjusted tm struct into UTC epoch seconds (time_t).
     * mktime normalizes any hour rollovers (e.g. if subtracting offset crossed midnight).
     */
    time_t utc_epoch = mktime(&local_tm);
    if (utc_epoch != (time_t)-1) {
        struct timespec ts;
        ts.tv_sec = utc_epoch;
        ts.tv_nsec = 0;

        /* Apply to the ESP32 system clock (POSIX CLOCK_REALTIME) */
        clock_settime(CLOCK_REALTIME, &ts);
        LOG_INF("Clock_settime done");
    }
}


/* Lightweight time parser without sscanf (safely reads "HH:MM" or "HHHMM") */
int parse_time_str(const char *str) {
    if (!str) return 0;
    
    int h = 0, m = 0;
    
    // Parse hours (first two characters)
    if (str[0] >= '0' && str[0] <= '9' && str[1] >= '0' && str[1] <= '9') {
        h = (str[0] - '0') * 10 + (str[1] - '0');
    } else {
        return 0;
    }
    
    // Parse minutes (characters at index 3 and 4)
    if (str[3] >= '0' && str[3] <= '9' && str[4] >= '0' && str[4] <= '9') {
        m = (str[3] - '0') * 10 + (str[4] - '0');
    } else {
        return 0;
    }
    
    return h * 60 + m;
}
