#include <zephyr/kernel.h>
#include <string.h>
#include <stdio.h>
#include "display.h"
#include "menu.h"
#include "menu_data.h"
#include "state.h"

/* External references from display.c */
extern uint8_t fb[];
extern uint8_t set_addr[];
extern void pcd8544_send(const uint8_t *data, size_t len, bool is_data);
void menu_process_event_editing(MenuEvent event, char *time_str);
void modify_active_digit_2digitvalue(char *val_str, int pos, int direction);
void update_system_time_from_strings(int TimeOrDate);

const Screen *current_screen = &screen_d0;
static const Screen *parent_screen = &screen_d0;
static int current_selection = 0;

/* Draw Arrow to have a display of 'line selection' */
int sel_arrow = 0; /*0 = Dark background, 1 = Left Arrow*/

void menu_init(void) {
    current_screen = &screen_d0;
    parent_screen = &screen_d0;
    current_selection = 0;
}

void menu_render(void) {
    clear_framebuffer();

    if (!current_screen) return;

    /* 1. Render Screen Title / Header */
    if (current_screen->name[0] != '\0') {
        draw_string(fb, 0, 0, (char *)current_screen->name);
    }
    /*Draw a line under the title*/
    fb_draw_line_under_title();
    /*Close with outside box*/
    draw_box();

    /* 2. Render Items Dynamically */ 
    for (int i = 0; i < current_screen->item_count && i < 5; i++) {
        int display_row = i + 1;
        char line_buf[32];

        /* 1. Combine label and dynamic strPtr (if it exists) into a single line buffer */
        if (current_screen->items[i].strPtr) {
            snprintf(line_buf, sizeof(line_buf), "%s%s", 
                    current_screen->items[i].label ? current_screen->items[i].label : "", 
                    current_screen->items[i].strPtr);
        } else if (current_screen->items[i].label) {
            snprintf(line_buf, sizeof(line_buf), "%s", current_screen->items[i].label);
        } else {
            line_buf[0] = '\0';
        }

        /* 2. Render based on selection state */
        if (i == current_selection && (strcmp(current_screen->id, "D0") != 0)) {
            if (sel_arrow == 1) {
                /* Draw selection cursor '>' at the edge, followed by the full line */
                draw_string(fb, 0, display_row, ">");
                draw_string(fb, 6, display_row, line_buf);
            } else {
                /* Mode sel_arrow == 0: Dark background highlights the ENTIRE line */
                draw_string_inv(fb, 6, display_row, line_buf);
            }
        } else {
            /* Unselected item: Draw normal text */
            draw_string(fb, 6, display_row, line_buf);
        }
    }

    /* Push framebuffer to screen */
    pcd8544_send(set_addr, 2, false);
    pcd8544_send(fb, BUFFER_SIZE, true);
}

void menu_process_event(MenuEvent event) {
    if (!current_screen) return;

    /* If we are actively editing time/Date digits, route events to the digit editor */
    if (app_state.is_editing) {
        menu_process_event_editing(
            event, 
            (char*)current_screen->items[current_selection].strPtr
            );
        return; 
    }
    switch (event) {
        case EVENT_UP:
            if (current_selection > 0) {
                current_selection--;
            }
            break;

        case EVENT_DOWN:
            if (current_selection < current_screen->item_count - 1) {
                current_selection++;
            }
            break;

        case EVENT_SELECT:
            if (current_screen == &screen_d0) {
                parent_screen = &screen_d0;
                current_screen = &screen_m0;
                current_selection = 0;
            } else {
                const char *target = current_screen->items[current_selection].target_screen;
                menu_action_cb_t action = current_screen->items[current_selection].target_action;
                if (target) {
                    const Screen *next_scr = get_screen_by_id(target);
                    if (next_scr) {
                        parent_screen = current_screen;
                        current_screen = next_scr;
                        current_selection = 0;
                    }
                }
                else if (action){
                    /*Launch the related callback */
                    action();
                }
            }
            break;

        case EVENT_BACK:
            if (current_screen != &screen_d0) {
                if (current_screen == &screen_m0) {
                    current_screen = &screen_d0;
                } else if(current_screen == &screen_m0_2_0){
                    current_screen = &screen_m0_2;
                }
                else{
                    current_screen = &screen_m0; // Fallback back to main menu
                }
                current_selection = 0;
            }
            break;

        default:
            break;
    }
}


/* For editing Time and Date --------------*/
void modify_active_digit(char *time_str, int pos, int direction) {
    if (!time_str) return;
    
    char c = time_str[pos];
    if (c == '-') c = '0'; /*Start from scratch*/
    if (c < '0' || c > '9') return; // Safety check

    int val = c - '0';
    val += direction; // +1 for UP, -1 for DOWN

    // Apply specific bounds per digit position in "HHHMM" format (index 2 is 'H')
    switch (pos) {
        case 0: // Hours tens (0-2)
            if (val > 2) val = 0;
            if (val < 0) val = 2;
            break;
        case 1: // Hours units (0-9, or 0-3 if tens is 2)
            if (time_str[0] == '2' && val > 3) val = 0;
            else {
                if (val > 9) val = 0;
                if (val < 0) val = 9;
            }
            break;
        case 3: // Minutes tens (0-5)
            if (val > 5) val = 0;
            if (val < 0) val = 5;
            break;
        case 4: // Minutes units (0-9)
            if (val > 9) val = 0;
            if (val < 0) val = 9;
            break;
        default:
            return; // Index 2 is 'H', do nothing
    }

    time_str[pos] = '0' + val;
}

/* For editing Time and Date --------------*/
void modify_active_digit_2digitValue(char *val_str, int pos, int direction) {
    if (!val_str) return;
    
    char c = val_str[pos];
    if (c == '-') c = '0'; /*Start from scratch*/
    if (c < '0' || c > '9') return; // Safety check

    int val = c - '0';
    val += direction; // +1 for UP, -1 for DOWN
    if (val > 9) val = 0;
    if (val < 0) val = 9;

    val_str[pos] = '0' + val;
}

void small_blink(){
    sel_arrow = 1;menu_render();
    k_msleep(250);
    sel_arrow = 0;menu_render();
    k_msleep(250);
}

void menu_process_event_editing(MenuEvent event, char *val_str) {
    switch (event) {
        case EVENT_UP:
            if (app_state.edit_what == 0 || app_state.edit_what == 2) modify_active_digit(val_str, app_state.edit_digit_pos, 1);
            else if (app_state.edit_what == 1) modify_active_digit_2digitValue(val_str, app_state.edit_digit_pos, 1);
            break;
            
        case EVENT_DOWN:
            if (app_state.edit_what == 0 || app_state.edit_what == 2) modify_active_digit(val_str, app_state.edit_digit_pos, -1);
            else if (app_state.edit_what == 1) modify_active_digit_2digitValue(val_str, app_state.edit_digit_pos, -1);
            break;
            
        case EVENT_SELECT:
            // Move to next editable digit, skipping index 2 ('H')
            app_state.edit_digit_pos++;
            small_blink();
            // If we've passed the last digit (index 4), finish editing and save
            if (app_state.edit_digit_pos > app_state.edit_lastdigit_pos) {
                for (int i=0; i<2; i++) {
                    small_blink();
                }
                app_state.is_editing = false;
                if (app_state.edit_what == 2){
                    /*Force this time into ESP32 system clock*/
                    printk("DEBUG: Calling update_system_time_from_strings...\n");
                    update_system_time_from_strings(0);//TimeOrDate = 0(Time), 1(Date)
                    printk("DEBUG: Time update complete.\n");
                }
                break;
            }
            /*Here we handle the case 'H' in Time editing*/
            if (app_state.edit_digit_pos == 2) {
                app_state.edit_digit_pos++; // Skip 'H'
            }
            break;
        default:
            break;
    }
}

 
