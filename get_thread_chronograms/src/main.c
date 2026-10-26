#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

// Settings
//static const int32_t sleep_time_ms = 1000;
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(my_led0), gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(my_led1), gpios);

/* Define stack size and thread priority */
#define STACK_SIZE 1024
#define PRIORITY1 0
#define PRIORITY2 7

/* State variables to be toggled */
volatile bool state1 = false;
volatile bool state2 = false;

/* Thread 1: Waits 1ms and toggles state1 */
void thread1_entry(void *p1, void *p2, void *p3)
{
	int ret;
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

   /* Desired interval in milliseconds (1ms for half-period / 500Hz square wave) */
    int64_t interval_ms = 1;
    
    /* Get initial absolute uptime target */
    int64_t next_wakeup = k_uptime_get() + interval_ms;

    while (1) {
        state1 = !state1;
		// Set pin state
		ret = gpio_pin_set_dt(&led1, state1);
		if (ret < 0) {
			printk ("ERR: set led1");
		}
		
		k_sleep(K_TIMEOUT_ABS_MS(next_wakeup));

        /* Advance the target timestamp for the next cycle */
        next_wakeup += interval_ms;
        //k_msleep(1);
    }
}






/* Thread 2: Waits 2ms and toggles state2 */
void thread2_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    while (1) {
        state2 = !state2;
        k_msleep(2);
    }
}

/* Statically define and initialize the threads */
K_THREAD_DEFINE(thread1_id, STACK_SIZE, thread1_entry, NULL, NULL, NULL,
                PRIORITY1, 0, 0);

K_THREAD_DEFINE(thread2_id, STACK_SIZE, thread2_entry, NULL, NULL, NULL,
                PRIORITY2, 0, 0);

int main(void)
{
	int ret;
	int state = 0;

	// Make sure that the GPIO was initialized
	if (!gpio_is_ready_dt(&led0) || !gpio_is_ready_dt(&led1) ) {
		return 0;
	}

	// Set the GPIO as output
	ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT);
	if (ret < 0) {
		return 0;
	}
	ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT);
	if (ret < 0) {
		return 0;
	}
	// Do forever
	while (1) {

		// Change the state of the pin and print
		state = !state;
		//printk("LED state: %d\r\n", state);
		
		// Set pin state
		ret = gpio_pin_set_dt(&led0, state);
		if (ret < 0) {
			return 0;
		}

		// Sleep
		//k_msleep(sleep_time_ms);
		k_msleep(500);
	}

	return 0;
}
