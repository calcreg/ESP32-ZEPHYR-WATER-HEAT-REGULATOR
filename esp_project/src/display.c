#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include "display.h"
#include "font5x7.h"

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

void pcd8544_send(const uint8_t *data, size_t len, bool is_data)
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
        /* "°" of °C is 176 in ASCII we need to keep it as is*/
        if ((c < 32 || c > 126) && c != 176) c = ' ';
        
        const uint8_t *glyph = font5x7[(unsigned char)c];
        
        for (int col = 0; col < 5; col++) {
            if (x + col < DISPLAY_WIDTH) {
                fb[(page * DISPLAY_WIDTH) + (x + col)] = glyph[col];
            }
        }
        x += 6;
    }
}

void draw_string_inv(uint8_t *fb, int x, int page, const char *str)
{
    while (*str) {
        char c = *str++;
        /* "°" of °C is 176 in ASCII we need to keep it as is*/
        if ((c < 32 || c > 126) && c != 176) continue; //c = ' ';
        
        const uint8_t *glyph = font5x7[(unsigned char)c];
        
        for (int col = 0; col < 5; col++) {
            if (x + col < DISPLAY_WIDTH) {
                fb[(page * DISPLAY_WIDTH) + (x + col)] = (~glyph[col]);
            }
        }
        fb[(page * DISPLAY_WIDTH) + (x + 5)] = 0xFF;
        x += 6;
    }
}

/* Classic PCD8544 Initialization Sequence */
uint8_t init_cmds[] = {
    0x21, // 1: Extended instruction set (H=1)
    0xA9, // 2: Set VOP (Contrast). Try changing 0xB1 between 0x80 and 0xEF if needed.
    0x04, // 3: Set Temp coefficient (TC2)
    0x14, // 4: Set bias system (BSx = 0x14 for 1:48 bias)
    0x20, // 5: Back to Basic instruction set (H=0)
    0x0C  // 6: Display control: Normal mode (0x0C = normal, 0x0D = inverted)
};

int init_nokia_screen(){
    LOG_INF("Starting Nokia 5110 text display app...");

    if (!device_is_ready(spi_dev.bus) || !gpio_is_ready_dt(&dc_gpio) || !gpio_is_ready_dt(&rst_gpio)) {
        LOG_ERR("Hardware peripherals not ready!");
        return -1;
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
    for (int i = 0; i < sizeof(init_cmds); i++) {
        pcd8544_send(&init_cmds[i], 1, false);
    }
    /*No errors*/
    return 0;
}

/* Create Frame Buffer */
uint8_t fb[BUFFER_SIZE];
uint8_t set_addr[] = { 0x80, 0x40 }; // RAM address to origin (X=0, Y=0)

void clear_framebuffer(){
    memset(fb, 0x00, sizeof(fb));
}
void draw_box(){
    /* Draw a box border */
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
        //fb[x] |= 0x01;                       // Top edge
        fb[5*DISPLAY_WIDTH + x] |= 0x80;    // Bottom edge
    }
    for (int p = 1; p < 6; p++) {
        fb[p * DISPLAY_WIDTH] = 0xFF;         // Left edge
        fb[p * DISPLAY_WIDTH + 83] = 0xFF;    // Right edge
    }
    /* Send Frame Buffer Data */
    pcd8544_send(set_addr, 2, false);
    pcd8544_send(fb, sizeof(fb), true);
}

void fb_draw_line_under_title(){
    /* Draw a box border */
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
        fb[x] |= 0x80;                       // Top under title
    }
}

void fbprint(int x, int y, char* text){
    draw_string (fb, x, y, text);
    /* Send Frame Buffer Data */
    pcd8544_send(set_addr, 2, false);
    pcd8544_send(fb, sizeof(fb), true);
}

