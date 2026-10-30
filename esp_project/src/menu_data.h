#ifndef MENU_DATA_H
#define MENU_DATA_H

#include <stdint.h>

typedef struct {
    const char *label;
    const char *target_screen;
    const char *target_action;
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
