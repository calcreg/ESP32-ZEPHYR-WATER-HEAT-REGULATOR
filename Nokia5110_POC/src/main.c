#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include "font5x7.h"

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(my_led0), gpios);

LOG_MODULE_REGISTER(nokia_text, LOG_LEVEL_INF);

/* Direct GPIO initializations for DC (GPIO 19) and RST (GPIO 22) */
static const struct gpio_dt_spec dc_gpio = {
    .port = DEVICE_DT_GET(DT_NODELABEL(gpio0)),
    .pin = 19,
    .dt_flags = GPIO_ACTIVE_HIGH
};

static const struct gpio_dt_spec rst_gpio = {
    .port = DEVICE_DT_GET(DT_NODELABEL(gpio0)),
    .pin = 22,
    .dt_flags = GPIO_ACTIVE_LOW
};

/* SPI device specification with all 3 required arguments */
static const struct spi_dt_spec spi_dev = SPI_DT_SPEC_GET(
    DT_NODELABEL(nokia_dev), 
    SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
    0
);

#define DISPLAY_WIDTH  84
#define DISPLAY_HEIGHT 48
#define BUFFER_SIZE    ((DISPLAY_WIDTH * DISPLAY_HEIGHT) / 8)

static void pcd8544_send(const uint8_t *data, size_t len, bool is_data)
{
    gpio_pin_set_dt(&dc_gpio, is_data ? 1 : 0);
    struct spi_buf buf = { .buf = (void *)data, .len = len };
    struct spi_buf_set tx = { .buffers = &buf, .count = 1 };
    spi_write_dt(&spi_dev, &tx);
}

void draw_string(uint8_t *fb, int x, int page, const char *str)
{
    while (*str) {
        char c = *str++;
        if (c < 32 || c > 126) c = ' ';
        
        const uint8_t *glyph = font5x7[(unsigned char)c];
        
        for (int col = 0; col < 5; col++) {
            if (x + col < DISPLAY_WIDTH) {
                fb[(page * DISPLAY_WIDTH) + (x + col)] = glyph[col];
            }
        }
        x += 6;
    }
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

    LOG_INF("Starting Nokia 5110 text display app...");

    if (!device_is_ready(spi_dev.bus) || !gpio_is_ready_dt(&dc_gpio) || !gpio_is_ready_dt(&rst_gpio)) {
        LOG_ERR("Hardware peripherals not ready!");
        return 0;
    }

 /* Configure pins as outputs */
    gpio_pin_configure_dt(&dc_gpio, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&rst_gpio, GPIO_OUTPUT_INACTIVE);

    /* --- Proper Hardware Reset Pulse --- */
    /* 1. Ensure reset is released (Logical 0 = Physical High) */
    gpio_pin_set_dt(&rst_gpio, 0);
    k_msleep(10);

    /* 2. Assert reset (Logical 1 = Physical Low) */
    gpio_pin_set_dt(&rst_gpio, 1);
    k_msleep(50); // Hold reset low for 50ms

    /* 3. Release reset to start the chip (Logical 0 = Physical High) */
    gpio_pin_set_dt(&rst_gpio, 0);
    k_msleep(50);

     /* Classic PCD8544 Initialization Sequence */
    uint8_t init_cmds[] = {
        0x21, // 1: Extended instruction set (H=1)
        0xAB, // 2: Set VOP (Contrast). Try changing 0xB1 between 0x80 and 0xEF if needed.
        0x04, // 3: Set Temp coefficient (TC2)
        0x14, // 4: Set bias system (BSx = 0x14 for 1:48 bias)
        0x20, // 5: Back to Basic instruction set (H=0)
        0x0C  // 6: Display control: Normal mode (0x0C = normal, 0x0D = inverted)
    };

    for (int i = 0; i < sizeof(init_cmds); i++) {
        pcd8544_send(&init_cmds[i], 1, false);
    }

    /* Create Frame Buffer */
    uint8_t fb[BUFFER_SIZE];
    memset(fb, 0x00, sizeof(fb));

    /* Draw a box border */
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
        fb[x] |= 0x01;                       // Top edge
        fb[5*DISPLAY_WIDTH + x] |= 0x80;    // Bottom edge
    }
    for (int p = 0; p < 6; p++) {
        fb[p * DISPLAY_WIDTH] = 0xFF;         // Left edge
        fb[p * DISPLAY_WIDTH + 83] = 0xFF;    // Right edge
    }

    /* Draw Text ("Hello World!") onto Page 2, starting at X = 6 */
    draw_string(fb, 6, 1, "Hello World!");
    /* Send Frame Buffer Data */
    uint8_t set_addr[] = { 0x80, 0x40 }; // RAM address to origin (X=0, Y=0)
    pcd8544_send(set_addr, 2, false);
    pcd8544_send(fb, sizeof(fb), true);

    LOG_INF("Hello World! rendered successfully!");
    /* Create Frame Buffer */
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
        draw_string(fb, 15, 3, strbuf);
        /* Send Frame Buffer Data */
        pcd8544_send(set_addr, 2, false);
        pcd8544_send(fb, sizeof(fb), true);
        
        k_sleep(K_MSEC(1000));
    }
}