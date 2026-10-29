#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include "display.h"
#include "menu.h"

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(my_led0), gpios);
/* Fetch pin specs directly from device tree aliases */
static const struct gpio_dt_spec btn_up = GPIO_DT_SPEC_GET(DT_ALIAS(btn_up), gpios);
static const struct gpio_dt_spec btn_down = GPIO_DT_SPEC_GET(DT_ALIAS(btn_down), gpios);
static const struct gpio_dt_spec btn_select = GPIO_DT_SPEC_GET(DT_ALIAS(btn_select), gpios);
LOG_MODULE_REGISTER(app_main, LOG_LEVEL_INF);


int init_buttons(void) {
    if (!device_is_ready(btn_up.port) || 
        !device_is_ready(btn_down.port) || 
        !device_is_ready(btn_select.port)) {
        LOG_ERR("One or more button GPIO ports not ready!");
        return -1;
    }

    gpio_pin_configure_dt(&btn_up, GPIO_INPUT);
    gpio_pin_configure_dt(&btn_down, GPIO_INPUT);
    gpio_pin_configure_dt(&btn_select, GPIO_INPUT);

    LOG_INF("Buttons initialized via Device Tree overlay.");
    return 0;
}

int main(void)
{
    int ret;
    int state = 0;

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
        return;
    }

    /* 2. Initialize the Menu State Machine */
    menu_init();

    /* 3. Perform initial render of the D0 Front Page */
    menu_render();

    LOG_INF("System ready. Entering main event loop.");

    uint8_t strbuf[BUFFER_SIZE];

    while (1) {
        // Change the state of the pin and print
        state = !state;
        // Set pin state
        ret = gpio_pin_set_dt(&led0, state);
        if (ret < 0) {
            return 0;
        }

        /* Since they are configured with GPIO_ACTIVE_LOW, 
           gpio_pin_get_dt returns 1 when pressed, 0 when released. */
        if (gpio_pin_get_dt(&btn_up) == 1) {
            menu_process_event(EVENT_UP);
            menu_render();
            k_msleep(200); // Debounce
        }

        if (gpio_pin_get_dt(&btn_down) == 1) {
            menu_process_event(EVENT_DOWN);
            menu_render();
            k_msleep(200);
        }

        if (gpio_pin_get_dt(&btn_select) == 1) {
            menu_process_event(EVENT_SELECT);
            menu_render();
            k_msleep(200);
        }

        //snprintf(strbuf, sizeof(strbuf), "LED is %s", (state == 1 ? "ON " : "OFF"));
        //fbprint(15, 3, strbuf);
        
        //k_sleep(K_MSEC(1000));
    }
}