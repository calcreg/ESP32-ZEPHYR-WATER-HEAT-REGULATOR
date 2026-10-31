#ifndef STATE_H
#define STATE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    /* Time & Date (populated by SNTP or manual setting) */
    char CurrentTime[16];   /* e.g., "22H30" */
    char CurrentDate[16];   /* e.g., "29/10/2026" */

    /* Temperatures only ASCII Values to display*/
    char TempCurrently[8]; //Actual Temp
    char TempMax[8];    //Pic Temp detected lastely
    char TempMin[8];    //Min detected lastly
    char TempTarget[8]; // Temperature to reach

    /* Heating Schedules (Slot 1 & Slot 2) */
    char TimeStartHeat1[8];  /* e.g., "06:00" */
    char TimeStopHeat1[8];   /* e.g., "08:00" */
    char TimeStartHeat2[8];  /* e.g., "18:00" */
    char TimeStopHeat2[8];   /* e.g., "21:00" */
    /*Heating State : ARRET / EN CHAUFFE / REGULATION*/
    char State[16];
    /*Regulation Mode ON/FULLON/OFF*/
    char RegulMode[16];
    char HeatDuration[8];   /*For immediate heat*/
    char CHTitle[16];       /*Immediat Heat Title, example : CH.:1H30*/
} AppState;

/* Expose a global reference to the application state */
extern AppState app_state;

#endif
