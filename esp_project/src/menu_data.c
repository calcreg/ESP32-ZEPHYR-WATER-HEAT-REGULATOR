#include "menu_data.h"
#include <stddef.h>
#include <string.h>
#include "state.h"
#include <stdio.h>

extern void menu_render(); /*In menu.c*/
/* Forward declare your callback functions */
void cb_set_duration_0H30(void);
void cb_set_duration_0H45(void);
void cb_set_duration_1H00(void);
void cb_set_duration_1H30(void);
void cb_set_duration_2H00(void);
void cb_set_regulation(void);
void cb_set_manual_time(void);
void cb_set_datetime(void);
void cb_set_target_temp(void);


/* D0: Front Page */
static const MenuItem items_d0[] = {
    { .label = "    ", .strPtr = app_state.TempMax },
    { .label = "    ", .strPtr = app_state.CurrentTime },
    { .label = " ", .strPtr = app_state.CurrentDate },
    { .label = ""},
    { .label = " ", .strPtr = app_state.State }
};

/* M0: MENU */
static const MenuItem items_m0[] = {
    { .label = "TEMP.EAU",   .target_screen = "M0_0" },
    { .label = "CHAUFFER",   .target_screen = "M0_1" },
    { .label = "PARAMETRES", .target_screen = "M0_2" }
};

/* M0_0: TEMP.EAU */
static const MenuItem items_m0_0[] = {
    { .label = "MAX: " },
    { .label = "MIN: " },
    { .label = "ACTUELLE: " },
    { .label = "T.CIBLE: " }
};

/* M0_1: CHAUFFER */
static const MenuItem items_m0_1[] = {
    { .label = "  0H30 ", .target_action = cb_set_duration_0H30 },
    { .label = "  0H45 ", .target_action = cb_set_duration_0H45 },
    { .label = "  1H00 ", .target_action = cb_set_duration_1H00 },
    { .label = "  1H30 ", .target_action = cb_set_duration_1H30 },
    { .label = "  2H00 ", .target_action = cb_set_duration_2H00 }
};

/* M0_2: PARAMETRES */
static const MenuItem items_m0_2[] = {
    { .label = "REGUL: ", .strPtr = app_state.RegulMode, .target_action = cb_set_regulation  },
    { .label = "HEURE: ", .strPtr = app_state.CurrentTime, .target_action = cb_set_manual_time },
    { .label = "DT:", .strPtr = app_state.CurrentDate, .target_action = cb_set_datetime },
    { .label = "HORAIRES CH.", .target_screen = "M0_2_0" },
    { .label = "T.CIBLE: ", .strPtr = app_state.TempTarget, .target_action = cb_set_target_temp  }
};

/* M0_2_0: HORAIRE CH. */
static const MenuItem items_m0_2_0[] = {
    { .label = "DEBUT1 0H00" },
    { .label = "FIN1   2H00" },
    { .label = "DEBUT2 17H30" },
    { .label = "FIN2   20H00" }
};

const Screen screen_d0     = { "D0", "  REGULATEUR", "Front Page", items_d0, 5 };
const Screen screen_m0     = { "M0", "MENU", "Main Menu", items_m0, 3 };
const Screen screen_m0_0   = { "M0_0", "TEMP.EAU", "Temperatures", items_m0_0, 4 };
const Screen screen_m0_1   = { "M0_1", app_state.CHTitle, "Heat Now", items_m0_1, 5 };
const Screen screen_m0_2   = { "M0_2", "PARAMETRES", "Settings", items_m0_2, 6 };
const Screen screen_m0_2_0 = { "M0_2_0", "HORAIRE CH.", "Heating Periods", items_m0_2_0, 4 };

const Screen *get_screen_by_id(const char *id) {
    if (strcmp(id, "D0") == 0) return &screen_d0;
    if (strcmp(id, "M0") == 0) return &screen_m0;
    if (strcmp(id, "M0_0") == 0) return &screen_m0_0;
    if (strcmp(id, "M0_1") == 0) return &screen_m0_1;
    if (strcmp(id, "M0_2") == 0) return &screen_m0_2;
    if (strcmp(id, "M0_2_0") == 0) return &screen_m0_2_0;
    return NULL;
}

/* 1. Core helper function that accepts the duration string */
static void apply_duration(const char *dur) {
    snprintf(app_state.HeatDuration, sizeof(app_state.HeatDuration), "%s", dur);
    snprintf(app_state.CHTitle, sizeof(app_state.CHTitle), "CH.: %s", dur);
    menu_render();
}

/* 2. Tiny wrapper functions matching the void (*)(void) menu signature */
void cb_set_duration_0H30(void) { apply_duration("0H30"); }
void cb_set_duration_0H45(void) { apply_duration("0H45"); }
void cb_set_duration_1H00(void) { apply_duration("1H00"); }
void cb_set_duration_1H30(void) { apply_duration("1H30"); }
void cb_set_duration_2H00(void) { apply_duration("2H00"); }

void cb_set_regulation(void) {
    /* Cycle through regulation modes: ON -> FULLON -> OFF -> ON */
    if (strcmp(app_state.RegulMode, "ON") == 0) {
        snprintf(app_state.RegulMode, sizeof(app_state.RegulMode), "FULLON");
        snprintf(app_state.State, sizeof(app_state.State), " Tjrs en CH.");
    } 
    else if (strcmp(app_state.RegulMode, "FULLON") == 0) {
        snprintf(app_state.RegulMode, sizeof(app_state.RegulMode), "OFF");
        snprintf(app_state.State, sizeof(app_state.State), "-- ARRET --");
    } 
    else {
        snprintf(app_state.RegulMode, sizeof(app_state.RegulMode), "ON");
        snprintf(app_state.State, sizeof(app_state.State), " REGUL.: ON");
    }
    
    menu_render();
}

void cb_set_manual_time(void){}
void cb_set_datetime(void){}
void cb_set_target_temp(void){}
