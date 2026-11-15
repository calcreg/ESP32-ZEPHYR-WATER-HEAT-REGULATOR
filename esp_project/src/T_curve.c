#include "T_curve.h"
#include "display.h"
#include "state.h"

/* External references from display.c */
extern uint8_t fb[];
extern uint8_t set_addr[];

static uint8_t Temp_mem[TMEMSIZE];
static int t_index = 0;         // Points to the next write position
static bool buffer_wrapped = false; // Tracks if we've filled the buffer at least once

//For test :
/*static int HeatTime_mem[HTMEMSIZE] = {30, 60, 120, 180, 240, 210, 150, 90, 45, 135,
                                    310, 160, 20, 180, 140, 210, 150, 90, 45, 135};
static int ht_index = 20;
*/

static int HeatTime_mem[HTMEMSIZE];
static int ht_index =0;         // Points to the next write position
static bool ht_buffer_wrapped = false; //Tracks if we've filled the buffer once

LOG_MODULE_REGISTER(app_Tcurve, LOG_LEVEL_INF);

void append_temp_value(int temp_scaled){

    // (Assuming temp_scaled has already been calculated)
    Temp_mem[t_index] = (uint8_t)(temp_scaled / 5);
    //LOG_INF("2T(%d) = %d",t_index,Temp_mem[t_index]);
    t_index++;
    if (t_index >= TMEMSIZE) {
        t_index = 0;
        buffer_wrapped = true; // We've completed a full-day cycle
    }
}

void T_Curve_Display(int mode) {
    int valid_items = buffer_wrapped ? TMEMSIZE : t_index;
    
    if (valid_items < 2) {
        fbprint(5, 2, "Pas assez de");
        fbprint(5, 3, "donnees ...");
        fbprint(5, 4, "Attendre SVP");
        return; // Not enough data to draw a curve
    }

    // Determine how many horizontal steps to draw
    int n_steps = (valid_items < DISPLAY_WIDTH) ? valid_items : DISPLAY_WIDTH;
    int start_offset = (mode == DISPLAY_MODE_RECENT && valid_items > DISPLAY_WIDTH) ? (valid_items - DISPLAY_WIDTH) : 0;
    /* 40 pix height -> 90 °C amplitude => *40 / (90 * 2) */
    #define display_factor 40 / (70 * 2) ; //display_factor for 70°C amplitude

    for (int x = 0; x < n_steps - 1; x++) {
        int k1, k2;

        if (mode == DISPLAY_MODE_RECENT) {
            k1 = start_offset + x;
            k2 = start_offset + x + 1;
        } else { // DISPLAY_MODE_FULL
            k1 = (x * valid_items) / DISPLAY_WIDTH;
            k2 = ((x + 1) * valid_items) / DISPLAY_WIDTH;
            if (k2 >= valid_items) k2 = valid_items - 1;
        }

        // Resolve circular buffer indices uniformly
        int src_index1 = buffer_wrapped ? (t_index + k1) % TMEMSIZE : k1;
        int src_index2 = buffer_wrapped ? (t_index + k2) % TMEMSIZE : k2;

        // Calculate Y coordinates
        int Y1 = DISPLAY_HEIGHT - (int)Temp_mem[src_index1] * display_factor;
        int Y2 = DISPLAY_HEIGHT - (int)Temp_mem[src_index2] * display_factor;

        fb_draw_line(x, Y1, x + 1, Y2);
    }

    /* Test marker / middle edge indicator */
    for (int p = 1; p < 6; p++) {
        fb[p * DISPLAY_WIDTH + DISPLAY_WIDTH/2] = 0xAA;    // middle edge
    }
    /* Draw a horizontal reference line at T = 50°C */
    int target_temp_2T = 50 * 2; // Since memory stores 2 * T (half-degree steps)
    int y_line = DISPLAY_HEIGHT - (int)target_temp_2T * display_factor;

    for (int x = 0; x < DISPLAY_WIDTH; x++) {
        // Optional: Make it a dashed line by checking (x % 2 == 0) if you don't want it solid
        if (x % 2 == 0) fb_draw_pixel(x, y_line);
    }
    //complete bottom full line
        for (int x = 12; x < DISPLAY_WIDTH-12; x++) {
        fb[5*DISPLAY_WIDTH + x] |= 0x80;    // Missing part of Bottom edge
    }
    /* Draw short horizontal reference lines at T = 30°C, 40°C, 60°C */
    int ref_temps[] = {30, 40, 60};
    int num_temps = sizeof(ref_temps) / sizeof(ref_temps[0]);

    for (int i = 0; i < num_temps; i++) {
        
        int y_line = DISPLAY_HEIGHT - (ref_temps[i] * 2) * display_factor;

        // Draw short ticks on Left, Center, and Right
        fb_draw_line(0, y_line, 3, y_line);  // Left edge
        fb_draw_line(DISPLAY_WIDTH / 2 - 2, y_line, DISPLAY_WIDTH / 2 + 2, y_line); // Center
        fb_draw_line(DISPLAY_WIDTH - 3, y_line, DISPLAY_WIDTH, y_line); // Right edge
    }
    if (mode == DISPLAY_MODE_RECENT) fbprint(45, 0, "Recent");
    else fbprint(45, 0, "full");
}

// Returns the maximum temperature as an integer (representing 2 * T)
// e.g., A return value of 51 means 25.5°C (51 / 2)
uint8_t retrieve_local_maxmin(int lookback_count, int maxmin) {
    //if maxmin=0 we retrieve max, else retrieve min
    int valid_items = buffer_wrapped ? TMEMSIZE : t_index;
    if (valid_items == 0) {
        return 0; // No data recorded yet
    }

    // Ensure we don't look back further than the data available
    int count = (lookback_count < valid_items) ? lookback_count : valid_items;
    if (count <= 0) {
        return Temp_mem[0]; //Take the first value
    }

    uint8_t max_val = 0;
    uint8_t min_val = 255; //Great value that is impossible to reach

    // Iterate backwards from the most recent entry (t_index - 1)
    for (int i = 0; i < count; i++) {
        int index;
        
        if (buffer_wrapped) {
            index = (t_index - 1 - i + TMEMSIZE) % TMEMSIZE;
        } else {
            index = (t_index - 1 - i);
        }

        uint8_t current_val = Temp_mem[index];
        if (current_val < min_val) {
            min_val = current_val;
        }
        if (current_val > max_val) {
            max_val = current_val;
        }
    }

    if (maxmin == RETRIEVE_MAX)return max_val;
    else return min_val;
}


void append_Heat_Time_value(int htime){

    // (Assuming temp_scaled has already been calculated)
    HeatTime_mem[ht_index] = htime;
    ht_index++;
    if (ht_index >= HTMEMSIZE) {
        ht_index = 0;
        ht_buffer_wrapped = true; // We've completed a full 31-day cycle
    }
}
/*HEAT TIME CURVE DISPLAY*/
void HT_Curve_Display(int mode) {

    int valid_items = ht_buffer_wrapped ? HTMEMSIZE : ht_index;
    
    if (valid_items < 2) {
        fbprint(5, 2, "Pas assez de");
        fbprint(5, 3, "donnees ...");
        fbprint(5, 4, "Attendre SVP");
        return; // Not enough data to draw a curve
    }
    /*Display vertical graduations*/
    fbprint(1, 5, "1");/*1h*/
    fbprint(1, 3, "2");/*2h*/
    fbprint(1, 1, "3h");/*3h*/

    int count = (mode == DISPLAY_MODE_RECENT) ? ((valid_items < 7) ? valid_items : 7) : valid_items;
    int start_offset = (mode == DISPLAY_MODE_RECENT && valid_items > 7) ? (valid_items - 7) : 0;
    
    int n_steps = (count < DISPLAY_WIDTH) ? count : DISPLAY_WIDTH;

    for (int x = 0; x < n_steps - 1; x++) {
        int k1 = start_offset + x;
        int k2 = start_offset + x + 1;

        int src_index1 = ht_buffer_wrapped ? (ht_index + k1) % HTMEMSIZE : k1;
        int src_index2 = ht_buffer_wrapped ? (ht_index + k2) % HTMEMSIZE : k2;

        int ht1 = HeatTime_mem[src_index1];
        int ht2 = HeatTime_mem[src_index2];
        if (ht1 > 240) ht1 = 240;
        if (ht2 > 240) ht2 = 240;

        int Y1 = DISPLAY_HEIGHT - (ht1 * DISPLAY_HEIGHT_TR) / 240;
        int Y2 = DISPLAY_HEIGHT - (ht2 * DISPLAY_HEIGHT_TR) / 240;
        int absx1 = count <= 7 ? x * DISPLAY_WIDTH  / 7 : x;
        int absx2 = count <= 7 ? (x + 1) * DISPLAY_WIDTH / 7 : x + 1;
        fb_draw_line(absx1, Y1, absx2 + 1, Y2);
        /*draw graduations*/
        if (mode == DISPLAY_MODE_RECENT) fb_draw_line(absx2, DISPLAY_HEIGHT, absx2, DISPLAY_HEIGHT-2);
        else if (absx2 % (DISPLAY_WIDTH / 7) == 0)fb_draw_line(absx2, DISPLAY_HEIGHT, absx2, DISPLAY_HEIGHT-2);
    }

    /* Draw a horizontal reference line at HT = 120 mins */
    int target_ht = 120;
    int y_line = DISPLAY_HEIGHT - (target_ht * DISPLAY_HEIGHT_TR) / 240;

    for (int x = 0; x < DISPLAY_WIDTH; x++) {
        if (x % 2 == 0) fb_draw_pixel(x, y_line);
    }

    // Complete bottom full line
    for (int x = 12; x < DISPLAY_WIDTH - 12; x++) {
        fb[5 * DISPLAY_WIDTH + x] |= 0x80;
    }

    /* Draw short horizontal reference lines at HT = 60 mins, 180 mins */
    int ref_times[] = {60, 180};
    int num_times = sizeof(ref_times) / sizeof(ref_times[0]);

    for (int i = 0; i < num_times; i++) {
        int y_line = DISPLAY_HEIGHT - (ref_times[i] * DISPLAY_HEIGHT_TR) / 240;

        fb_draw_line(0, y_line, 3, y_line);  // Left edge
        fb_draw_line(DISPLAY_WIDTH - 3, y_line, DISPLAY_WIDTH, y_line); // Right edge
    }

    if (mode == DISPLAY_MODE_RECENT) fbprint(45, 0, "WEEK");
    else fbprint(45, 0, "FULL");
}
