#ifndef MENU_H
#define MENU_H

#include <stdint.h>
#include <stdbool.h>

/* Navigation / Button Events */
typedef enum {
    EVENT_NONE,
    EVENT_UP,
    EVENT_DOWN,
    EVENT_SELECT,
    EVENT_BACK
} MenuEvent;

/* Public Menu Functions */
void menu_init(void);
void menu_process_event(MenuEvent event);
void menu_render(void);

#endif /* MENU_H */