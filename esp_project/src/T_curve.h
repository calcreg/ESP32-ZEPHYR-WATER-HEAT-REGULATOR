#include <stdbool.h>
#include <stdint.h> // Required for uint8_t
#include <zephyr/logging/log.h>

extern void fb_draw_pixel(int x, int y);
extern void fb_draw_line(int x1, int y1, int x2, int y2);
extern void safe_menu_render();

#define TMEMSIZE (24 * 60) // Temperature every minutes on a day is a cycle

extern void append_temp_value(int temp_scaled);
extern void T_Curve_Display();
extern uint8_t retrieve_local_maxmin(int lookback_count, int maxmin);
