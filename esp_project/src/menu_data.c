#include "menu_data.h"
#include <stddef.h>
#include <string.h>

/* D0: Front Page */
static const MenuItem items_d0[] = {
    { .label = "Temp_Detected" },
    { .label = "Time" },
    { .label = "Date" },
    { .label = "STATE" }
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
    { .label = "DUREE: 1H00", .target_action = "set_duration" }
};

/* M0_2: PARAMETRES */
static const MenuItem items_m0_2[] = {
    { .label = "REGUL: ----", .target_action = "set_regulation"  },
    { .label = "HEURE: --H--", .target_action = "set_manual_time" },
    { .label = "DATE : ----", .target_action = "set_datetime" },
    { .label = "HORAIRES CH.", .target_screen = "M0_2_0" },
    { .label = "T.CIBLE:--°C", .target_action = "set_target_temp"  }
};

/* M0_2_0: HORAIRE CH. */
static const MenuItem items_m0_2_0[] = {
    { .label = "DEBUT1 0H00" },
    { .label = "FIN1   2H00" },
    { .label = "DEBUT2 17H30" },
    { .label = "FIN2   20H00" }
};

const Screen screen_d0     = { "D0", "  REGULATEUR", "Front Page", items_d0, 4 };
const Screen screen_m0     = { "M0", "MENU", "Main Menu", items_m0, 3 };
const Screen screen_m0_0   = { "M0_0", "TEMP.EAU", "Temperatures", items_m0_0, 4 };
const Screen screen_m0_1   = { "M0_1", "CHAUFFER", "Heat Now", items_m0_1, 1 };
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

