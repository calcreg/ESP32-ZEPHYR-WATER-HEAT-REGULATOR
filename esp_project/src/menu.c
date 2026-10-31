#include <string.h>
#include <stdio.h>
#include "display.h"
#include "menu.h"
#include "menu_data.h"

/* External references from display.c */
extern uint8_t fb[];
extern uint8_t set_addr[];
extern void pcd8544_send(const uint8_t *data, size_t len, bool is_data);

static const Screen *current_screen = &screen_d0;
static const Screen *parent_screen = &screen_d0;
static int current_selection = 0;

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
        int sel_arrow = 0; /* Keep your existing arrow logic variable */
        
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
                } else {
                    current_screen = &screen_m0; // Fallback back to main menu
                }
                current_selection = 0;
            }
            break;

        default:
            break;
    }
}

