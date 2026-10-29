#define DISPLAY_WIDTH  84
#define DISPLAY_HEIGHT 48
#define BUFFER_SIZE    ((DISPLAY_WIDTH * DISPLAY_HEIGHT) / 8)
# include <stdbool.h>

void clear_framebuffer();
void draw_box();
void fbprint(int x, int y, char* text);
void pcd8544_send(const uint8_t *data, size_t len, bool is_data);
void draw_string(uint8_t *fb, int, int, const char*);
void draw_string_inv(uint8_t *fb, int, int, const char*);
void fb_draw_line_under_title();
int init_nokia_screen();
