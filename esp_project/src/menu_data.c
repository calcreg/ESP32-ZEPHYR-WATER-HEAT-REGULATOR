#include "menu_data.h"
#include <stddef.h>
#include <string.h>
#include "state.h"
#include <stdio.h>

//extern void menu_render(); /*In menu.c*/
extern void safe_menu_render();
/* Forward declare your callback functions */
void cb_set_duration_0H30(void);
void cb_set_duration_0H45(void);
void cb_set_duration_1H00(void);
void cb_set_duration_1H30(void);
void cb_set_duration_2H00(void);
void cb_set_regulation(void);
void cb_set_clock_time(void);
void cb_set_date(void);
void cb_set_target_temp(void);
void cb_set_Time(void);
void cb_set_kp(void);
void cb_set_ki(void);
void cb_set_kd(void);

/* D0: Front Page */
static const MenuItem items_d0[] = {
    { .label = ""},
    { .label = "   ", .strPtr = app_state.CurrentTime },
    { .label = " ", .strPtr = app_state.CurrentDate },
    { .label = "", .strPtr = app_state.TempMaxAct },
    { .label = " ", .strPtr = app_state.GlobalState }
};

/* M0: MENU */
static const MenuItem items_m0[] = {
    { .label = "TEMP.EAU",   .target_screen = "M0_0" },
    { .label = "CHAUFFER",   .target_screen = "M0_1" },
    { .label = "PARAMETRES", .target_screen = "M0_2" },
    { .label = "KPI", .target_screen = "M0_3" }
};

/* M0_0: TEMP.EAU */
static const MenuItem items_m0_0[] = {
    { .label = "MAX: ", .strPtr = app_state.TempMax },
    { .label = "MIN: ", .strPtr = app_state.TempMin },
    { .label = "T.ACT: ", .strPtr = app_state.TempCurrently },
    { .label = "T.CIBLE:" , .strPtr = app_state.TempTarget, .target_action = cb_set_target_temp},
    { .label = "T=f(t) DISP.", .target_screen = "M0_0_1" }
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
    { .label = "HEURE: ", .strPtr = app_state.CurrentTime, .target_action = cb_set_clock_time },
    { .label = "DT:", .strPtr = app_state.CurrentDate, .target_action = cb_set_date},
    { .label = "HORAIRES CH.", .target_screen = "M0_2_0" },
    { .label = "T.CIBLE: ", .strPtr = app_state.TempTarget, .target_action = cb_set_target_temp  }
};

/* M0_3: KPI */
static const MenuItem items_m0_3[] = {
    { .label = "KP: ", .strPtr = app_state.KP, .target_action = cb_set_kp },
    { .label = "KI: ", .strPtr = app_state.KI, .target_action = cb_set_ki },
    { .label = "KD: ", .strPtr = app_state.KD, .target_action = cb_set_kd}
};
/* M0_2_0: HORAIRE CH. */
static const MenuItem items_m0_2_0[] = {
    { .label = "DEBUT1:" , .strPtr = app_state.TimeStartHeat1, .target_action = cb_set_Time },
    { .label = "FIN1  :" , .strPtr = app_state.TimeStopHeat1, .target_action = cb_set_Time },
    { .label = "DEBUT2:" , .strPtr = app_state.TimeStartHeat2, .target_action = cb_set_Time },
    { .label = "FIN2  :" , .strPtr = app_state.TimeStopHeat2, .target_action = cb_set_Time }
};

const Screen screen_d0     = { "D0", "  REGULATEUR", "Front Page", items_d0, 5 };
const Screen screen_m0     = { "M0", "MENU", "Main Menu", items_m0, 4 };
const Screen screen_m0_0   = { "M0_0", "TEMP.EAU", "Temperatures", items_m0_0, 5 };
const Screen screen_m0_0_1 = { "M0_0_1", "T=f(t)", "Display T=f(t)", NULL, 0 };
const Screen screen_m0_1   = { "M0_1", app_state.CHTitle, "Heat Now", items_m0_1, 5 };
const Screen screen_m0_2   = { "M0_2", "PARAMETRES", "Settings", items_m0_2, 6 };
const Screen screen_m0_3   = { "M0_2", "KPI Settings", "set KPI", items_m0_3, 3 };
const Screen screen_m0_2_0 = { "M0_2_0", "HORAIRE CH.", "Heating Periods", items_m0_2_0, 4 };

const Screen *get_screen_by_id(const char *id) {
    if (strcmp(id, "D0") == 0) return &screen_d0;
    if (strcmp(id, "M0") == 0) return &screen_m0;
    if (strcmp(id, "M0_0") == 0) return &screen_m0_0;
    if (strcmp(id, "M0_0_1") == 0) return &screen_m0_0_1;
    if (strcmp(id, "M0_1") == 0) return &screen_m0_1;
    if (strcmp(id, "M0_2") == 0) return &screen_m0_2;
    if (strcmp(id, "M0_3") == 0) return &screen_m0_3;
    if (strcmp(id, "M0_2_0") == 0) return &screen_m0_2_0;
    return NULL;
}

/* 1. Core helper function that accepts the duration string */
static void apply_duration(const char *dur) {
    snprintf(app_state.HeatDuration, sizeof(app_state.HeatDuration), "%s", dur);
    snprintf(app_state.CHTitle, sizeof(app_state.CHTitle), "CH.: %s", dur);
    //menu_render();
    safe_menu_render();
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
    
    //menu_render();
    safe_menu_render();
}

void cb_set_Time(){
    /*Reset editing state and position of the cursor*/
    app_state.is_editing = true;
    app_state.edit_what = 0; /*For Heater Time Schedule*/
    app_state.edit_digit_pos = 0;
    app_state.edit_lastdigit_pos = 4; /*0,1,3,4*/
}

void cb_set_target_temp(void){
    /*Reset editing state and position of the cursor*/
    app_state.is_editing = true;
    app_state.edit_what = 1; /*For TempTarget*/
    app_state.edit_digit_pos = 0;
    app_state.edit_lastdigit_pos = 1; /*0 and 1*/
}

void cb_set_clock_time(void){
    app_state.is_editing = true;
    app_state.edit_what = 2; /*For Time*/
    app_state.edit_digit_pos = 0;
    app_state.edit_lastdigit_pos = 4; /*0,1,3,4*/
}

void cb_set_date(void){
   app_state.is_editing = true;
    app_state.edit_what = 3; /*For Date*/
    app_state.edit_digit_pos = 0;
    app_state.edit_lastdigit_pos = 9; /*0,1,3,4,6,7,8,9*/
}

void cb_set_kp(void){
    app_state.is_editing = true;
    app_state.edit_what = 4; /*For kp*/
    app_state.edit_digit_pos = 0;
    app_state.edit_lastdigit_pos = 3; /*0,2,3*/
}
void cb_set_ki(void){
    app_state.is_editing = true;
    app_state.edit_what = 5; /*For ki*/
    app_state.edit_digit_pos = 0;
    app_state.edit_lastdigit_pos = 3; /*0,2,3*/
}
void cb_set_kd(void){
    app_state.is_editing = true;
    app_state.edit_what = 6; /*For kd*/
    app_state.edit_digit_pos = 0;
    app_state.edit_lastdigit_pos = 3; /*0,2,3*/
}
