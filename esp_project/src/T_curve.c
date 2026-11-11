#include "T_curve.h"
#include "display.h"
#include "state.h"

/* External references from display.c */
extern uint8_t fb[];
extern uint8_t set_addr[];

static uint8_t Temp_mem[TMEMSIZE];
static int t_index = 0;         // Points to the next write position
static bool buffer_wrapped = false; // Tracks if we've filled the buffer at least once

LOG_MODULE_REGISTER(app_Tcurve, LOG_LEVEL_INF);

void append_temp_value(int temp_scaled){

    // (Assuming temp_scaled has already been calculated)
    Temp_mem[t_index] = (uint8_t)(temp_scaled / 5);
    //LOG_INF("2T(%d) = %d",t_index,Temp_mem[t_index]);
    t_index++;
    if (t_index >= TMEMSIZE) {
        t_index = 0;
        buffer_wrapped = true; // We've completed a full 31-day cycle
    }
}

void T_Curve_Display(){
    // Display the chronologically ordered array

    int max_disp_val=0; /*display is DISPLAY_WIDTH wide*/
    if (t_index < DISPLAY_WIDTH ) max_disp_val = t_index;
    else max_disp_val = DISPLAY_WIDTH ;
    int valid_items = buffer_wrapped ? TMEMSIZE : max_disp_val;

    if (valid_items > 2) 
    for (int i = 0; i < valid_items-1; i++) {
        int src_index1;
        int src_index2;
        if (buffer_wrapped) {
            // If wrapped, the oldest item is right at t_index, 
            // so we start there and loop around the ring.
            src_index1 = (t_index + i) % TMEMSIZE;
            src_index2 = (t_index + i + 1) % TMEMSIZE;
        } else {
            // If not full yet, data simply starts at index 0 up to t_index - 1
            src_index1 = i;
            src_index2 = i+1;
        }
        /*Fit for Screen Xmax=84 Ymax=48*/
        /*Max Temp = 90 -> 40 height*/
        int Y1 = DISPLAY_HEIGHT-(int)Temp_mem[src_index1]*40/(90*2);
        int Y2 = DISPLAY_HEIGHT-(int)Temp_mem[src_index2]*40/(90*2);
        fb_draw_line(i,Y1, i+1, Y2) ;
        //LOG_INF("Draw_line(%d,%d,%d,%d)",i,Y1,i+1, Y2);
    }
    /*test :*/
    for (int p = 1; p < 6; p++) {
        fb[p * DISPLAY_WIDTH + 41] = 0xAA;    // middle edge
    }
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

    if (maxmin == 0)return max_val;
    else return min_val;
}

