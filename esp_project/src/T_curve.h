#include <stdbool.h>
#include <stdint.h> // Required for uint8_t
#include <zephyr/logging/log.h>

extern void fb_draw_pixel(int x, int y);
extern void fb_draw_line(int x1, int y1, int x2, int y2);
extern void safe_menu_render();

extern void append_temp_value(int temp_scaled);
extern void T_Curve_Display(int mode);
extern uint8_t retrieve_local_maxmin(int lookback_count, int maxmin);

#define TMEMSIZE (24 * 60 * 2) // Temperature every 30secs on a day is a cycle
#define DISPLAY_MODE_RECENT 0 // Shows only the last 84 data points (1-to-1 pixel mapping)
#define DISPLAY_MODE_FULL   1 // Compresses all recorded history across the 84 screen columns
#define RETRIEVE_MAX 0 //Retrieve max over the period of time
#define RETRIEVE_MIN 1 //Retrieve min over the period of time



