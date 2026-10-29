#define DISPLAY_WIDTH  84
#define DISPLAY_HEIGHT 48
#define BUFFER_SIZE    ((DISPLAY_WIDTH * DISPLAY_HEIGHT) / 8)

void clear_framebuffer();
void draw_box();
void fbprint(int x, int y, char* text);
int init_nokia_screen();
