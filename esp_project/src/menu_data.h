#ifndef MENU_DATA_H
#define MENU_DATA_H

#include <stdint.h>

/* Define the function pointer type for actions */
typedef void (*menu_action_cb_t)(void);

typedef struct {
    const char *label;
    const char *target_screen;
    const char *strPtr;
    menu_action_cb_t target_action; /* Direct function pointer callback when selected */
} MenuItem;

typedef struct {
    const char *id;
    const char *name;
    const char *description;
    const MenuItem *items;
    uint8_t item_count;
} Screen;

/* Declare screens */
extern const Screen screen_d0;
extern const Screen screen_m0;
extern const Screen screen_m0_0;
extern const Screen screen_m0_1;
extern const Screen screen_m0_2;
extern const Screen screen_m0_2_0;

const Screen *get_screen_by_id(const char *id);

#endif
