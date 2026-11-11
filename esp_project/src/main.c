#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdbool.h>
#include "display.h"
#include "menu.h"
#include "wifi.h"
#include "state.h"
#include "menu_data.h"
#include "T_curve.h"

/* Define stack size and thread priority for pad-handling thread*/
#define STACK_SIZE 1024
#define PRIORITY1 7 //buttons' state Thread
#define PRIORITY2 7 //heater Thread

extern void refresh_local_time(void);
extern const Screen *current_screen;
extern float read_ntc_voltage(void);
extern int adc_init(void);
extern float GetTemperature(float V_ADC);

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(my_led0), gpios);
static const struct gpio_dt_spec heater = GPIO_DT_SPEC_GET(DT_ALIAS(my_heater), gpios);
/* Fetch pin specs directly from device tree aliases */
static const struct gpio_dt_spec btn_up = GPIO_DT_SPEC_GET(DT_ALIAS(btn_up), gpios);
static const struct gpio_dt_spec btn_down = GPIO_DT_SPEC_GET(DT_ALIAS(btn_down), gpios);
static const struct gpio_dt_spec btn_select = GPIO_DT_SPEC_GET(DT_ALIAS(btn_select), gpios);
static const struct gpio_dt_spec btn_back= GPIO_DT_SPEC_GET(DT_ALIAS(btn_back), gpios);

LOG_MODULE_REGISTER(app_main, LOG_LEVEL_INF);

/* Define a mutex for the LCD/SPI bus */
K_MUTEX_DEFINE(lcd_lock);

/* Thread-safe wrapper for rendering */
void safe_menu_render(void) {
    /* Try to acquire the mutex with a timeout (e.g., 100ms) to avoid permanent deadlocks */
    if (k_mutex_lock(&lcd_lock, K_MSEC(100)) == 0) {
        menu_render();
        k_mutex_unlock(&lcd_lock);
    } else {
        LOG_WRN("Failed to acquire LCD mutex for rendering");
    }
}

int init_buttons(void) {
    if (!device_is_ready(btn_up.port) || 
        !device_is_ready(btn_down.port) ||
        !device_is_ready(btn_back.port) || 
        !device_is_ready(btn_select.port)) {
        LOG_ERR("One or more button GPIO ports not ready!");
        return -1;
    }

    gpio_pin_configure_dt(&btn_up, GPIO_INPUT);
    gpio_pin_configure_dt(&btn_down, GPIO_INPUT);
    gpio_pin_configure_dt(&btn_select, GPIO_INPUT);
    gpio_pin_configure_dt(&btn_back, GPIO_INPUT);

    LOG_INF("Buttons initialized via Device Tree overlay.");
    return 0;
}

static void Gpio_pin_get(const struct gpio_dt_spec *btn, MenuEvent event){
 	/* Since they are configured with GPIO_ACTIVE_LOW, 
           gpio_pin_get_dt returns 1 when pressed, 0 when released. */
              if (gpio_pin_get_dt(btn) == 1) {
                  k_msleep(50); // Debounce
                  while (gpio_pin_get_dt(btn) == 1) k_msleep(10); /*Wait till btn is relaxed*/
                  menu_process_event(event);
                  //menu_render();
                  safe_menu_render();
	       }
	 }
/* Pad_Handler_Thread: Waits 1ms and toggles state1 */
void Pad_Handler_thread_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    while (1) {
        Gpio_pin_get(&btn_up,EVENT_UP);
        Gpio_pin_get(&btn_down,EVENT_DOWN);
        Gpio_pin_get(&btn_select,EVENT_SELECT);
        Gpio_pin_get(&btn_back,EVENT_BACK);
        k_msleep(200);
    }
}

/* Statically define and initialize the threads */
K_THREAD_DEFINE(Pad_Handler, STACK_SIZE, Pad_Handler_thread_entry, NULL, NULL, NULL,
                PRIORITY1, 0, 0);
/* ---*/
/* Heater_Thread:*/
/* Lightweight time parser without sscanf (safely reads "HH:MM" or "HHHMM") */
static int parse_time_str(const char *str) {
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

/* Heater_thread_entry: Evaluates time windows safely */
void Heater_thread_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    int ret;

    // Make sure that the GPIO was initialized
    if (!gpio_is_ready_dt(&heater)) {
        LOG_INF("Couldn't initialise heater GPIO 2.");
    }

    // Set the GPIO as output
    ret = gpio_pin_configure_dt(&heater, GPIO_OUTPUT);
    if (ret < 0) {
        LOG_INF("Couldn't configure heater GPIO 2.");
    }
       
    while (1) {
        // 1. Get current time and schedule windows dynamically from app_state using safe parser
        int current_minutes = parse_time_str(app_state.CurrentTime);
        
        int start1 = parse_time_str(app_state.TimeStartHeat1);
        int stop1  = parse_time_str(app_state.TimeStopHeat1);
        int start2 = parse_time_str(app_state.TimeStartHeat2);
        int stop2  = parse_time_str(app_state.TimeStopHeat2);
        int state = 0; /* can be 0,1 to 4*/
        static int previous_state = -1;

        // 2. Check if current time falls within Slot 1 or Slot 2
        bool in_time_window = ((current_minutes >= start1 && current_minutes <= stop1) ||
                               (current_minutes >= start2 && current_minutes <= stop2));

        // 3. Heater logic
        if ((strcmp(app_state.RegulMode, "ON") == 0) && in_time_window) {
            gpio_pin_set_dt(&heater, 1);
            snprintf(app_state.GlobalState, sizeof(app_state.GlobalState), "EN CHAUFFE");
            snprintf(app_state.State, sizeof(app_state.State), "ON");
            state=1;
        }else if ((strcmp(app_state.RegulMode, "ON") == 0) && !in_time_window) {
            gpio_pin_set_dt(&heater, 0);
            snprintf(app_state.GlobalState, sizeof(app_state.GlobalState), "HORS PLAGE");
            snprintf(app_state.State, sizeof(app_state.State), "OFF");
            state=2;
        } else if (strcmp(app_state.RegulMode, "FULLON") == 0) {
            gpio_pin_set_dt(&heater, 1);
            snprintf(app_state.State, sizeof(app_state.State), "ON");
            snprintf(app_state.GlobalState, sizeof(app_state.GlobalState), "TJRS EN CH.");
            state=3;
        }else{
            gpio_pin_set_dt(&heater, 0);
            snprintf(app_state.State, sizeof(app_state.State), "OFF");
            snprintf(app_state.GlobalState, sizeof(app_state.GlobalState), "-- ARRET --");
            state=4;
        }
        if (previous_state != state) safe_menu_render();
        previous_state = state;
        k_msleep(1000);
    }
}

// Statically define and initialize the threads
K_THREAD_DEFINE(Heater, STACK_SIZE, Heater_thread_entry, NULL, NULL, NULL,
                PRIORITY2, 0, 0);


int main(void)
{
    int ret;

    // Make sure that the GPIO was initialized
    if (!gpio_is_ready_dt(&led0)) {
        return 0;
    }

    // Set the GPIO as output
    ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT);
    if (ret < 0) {
        return 0;
    }

    LOG_INF("Starting Hot-Water Controller Application...");

    /* 1. Initialize the Nokia 5110 display hardware */
    if (init_nokia_screen() != 0) {
        LOG_ERR("Failed to initialize Nokia screen!");
    }

    if (init_buttons() != 0) {
        return 0;
    }

    /* 2. Initialize the Menu State Machine */
    menu_init();

    /* 3. Perform initial render of the D0 Front Page */
    //menu_render();
    safe_menu_render();

    /* 1. Scan for nearby Wi-Fi networks first (results print via logs, or hook to your menu) */
    //LOG_INF("Scanning for available Wi-Fi networks...");
    //start_wifi_scan();

    print_esp32_mac_address();
    /* 4. Connect to Wi-Fi */
    wifi_connect_init();

    LOG_INF("Waiting for network time synchronization...");
    snprintf(app_state.CurrentTime, sizeof(app_state.CurrentTime), "WSNTP");
    //menu_render();
    safe_menu_render();
    if (wifi_wait_for_ip(K_SECONDS(15)) == 0) {
        char time_str[16];
        char date_str[16];

        /* Give the network stack 1 second to bind DNS and routes */
        k_sleep(K_SECONDS(1));
        /* Fetch time from internet */
        if (fetch_internet_time_and_date(time_str, sizeof(time_str), date_str, sizeof(date_str)) == 0) {
            LOG_INF("Time UTC successfully synchronized: %s, %s", date_str, time_str);
            refresh_local_time();
            LOG_INF("Set to local Time Zone.");
            //menu_render();
            safe_menu_render();
        }else{
            snprintf(app_state.CurrentTime, sizeof(app_state.CurrentTime), "--H--");
            //menu_render();
            safe_menu_render();
        }
        
        /* Lock down the network immediately after sync to ensure security */
        disconnect_wifi_securely();
    } else {
        LOG_WRN("Network timeout. Disconnecting and running offline.");
        disconnect_wifi_securely();
    }

    LOG_INF("System ready. Entering main event loop.");
    
    ret=adc_init();
    if (ret <0){
        LOG_ERR("Couldn't init adc");
        app_state.adcReady = false;
    }else{
        LOG_INF("ADC Initialized and ready");
        app_state.adcReady = true;
    }
    
    int ctn=0;
    static int temp_scaled_previous;
    while (1) {
        ctn++;
        // Heartbit Led blinks every 5 secs
        if (ctn % 5 == 0 ) {
            gpio_pin_set_dt(&led0, 1);
            k_sleep(K_MSEC(50));
            gpio_pin_set_dt(&led0,0);;
            }
        if (ctn %60 == 0 ) {
            refresh_local_time();
            if (strcmp(current_screen->id, "D0") == 0) safe_menu_render();
        }
        //Read Voltage
        
        if (app_state.adcReady == true) {
            float V_ADC = read_ntc_voltage();
            float Temp = GetTemperature(V_ADC);
            // Scale by 10 to keep one decimal place using integers
            int temp_scaled = (int)(Temp * 10.0f);
            /*Save value of temperature along time every 30secs*/
            if (ctn %30 == 0 ) append_temp_value(temp_scaled);

            /*Handle TempMin, TempMax*/
            static int min_scaled = 0;
            static int max_scaled = 0;
            static bool initialized = false;
            // Initialize min/max on the very first reading
            if (!initialized) {
                min_scaled = temp_scaled;
                max_scaled = temp_scaled;
                initialized = true;
            } else {
                if (ctn %60 == 0 ) {
                    max_scaled = 5 * (int)retrieve_local_maxmin(TMEMSIZE*3/4,0); //Search Max over the last 18 hours roughly
                    min_scaled = 5 * (int)retrieve_local_maxmin(TMEMSIZE/10,1); //Search min over the last 18 hours roughly
                }
            }
            /* /!\ the _scaled temperatures are coming from Temp*10 */
            if (temp_scaled != temp_scaled_previous) {
                //LOG_INF("Temperature measured is %d.%d °C", temp_scaled / 10, abs(temp_scaled % 10));
                snprintf (app_state.TempCurrently, sizeof(app_state.TempCurrently),"%2d.%1d@C",temp_scaled/10, abs(temp_scaled % 10));
                snprintf(app_state.TempMax, sizeof(app_state.TempMax), "%2d.%d@C", max_scaled / 10, abs(max_scaled % 10));
                snprintf(app_state.TempMin, sizeof(app_state.TempMin), "%2d.%d@C", min_scaled / 10, abs(min_scaled % 10));
                snprintf(app_state.TempMaxAct, sizeof(app_state.TempMaxAct), "%s/%s", app_state.TempMax,app_state.TempCurrently);
                temp_scaled_previous = temp_scaled;
                if (strcmp(current_screen->id, "D0") == 0 || 
                    strcmp(current_screen->id, "M0_0") == 0) safe_menu_render();
            }
        }
        k_sleep(K_MSEC(1000));
    }
}
