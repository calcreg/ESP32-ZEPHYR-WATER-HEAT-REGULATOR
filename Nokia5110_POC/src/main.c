#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include "display.h"

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(my_led0), gpios);

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

    /*Nokia Screen reset/init procedure */
    if (init_nokia_screen() == -1) return 0;

    /*Initialize the framebuffer*/
    clear_framebuffer();
    /* Create Frame Buffer */
    draw_box();

    /* Draw Text ("Hello World!") onto Page 2, starting at X = 6 */
    fbprint(6, 1, "Hello World!");

    uint8_t strbuf[BUFFER_SIZE];

    while (1) {
        // Change the state of the pin and print
        state = !state;
        // Set pin state
        ret = gpio_pin_set_dt(&led0, state);
        if (ret < 0) {
            return 0;
        }
        snprintf(strbuf, sizeof(strbuf), "LED is %s", (state == 1 ? "ON " : "OFF"));
        fbprint(15, 3, strbuf);
        
        k_sleep(K_MSEC(1000));
    }
}