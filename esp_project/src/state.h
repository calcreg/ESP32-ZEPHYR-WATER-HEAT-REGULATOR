#ifndef STATE_H
#define STATE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    /* Time & Date (populated by SNTP or manual setting) */
    char CurrentTime[16];   /* e.g., "22H30" */
    char CurrentDate[36];   /* 16 should be enough but generates warning. e.g., "29/10/2026" */

    /* Temperatures only ASCII Values to display*/
    char TempCurrently[8]; //Actual Temp
    char TempMax[8];    //Pic Temp detected lastely
    char TempMin[8];    //Min detected lastly
    char TempMaxAct[16];// Max°C / Act°C compilation of TempMax and TempCurrently
    char TempTarget[8]; // Temperature to reach

    /* Heating Schedules (Slot 1 & Slot 2) */
    char TimeStartHeat1[8];  /* e.g., "06:00" */
    char TimeStopHeat1[8];   /* e.g., "08:00" */
    char slot2[8]; /*ON/OFF to enable/disable time window slot2*/
    char TimeStartHeat2[8];  /* e.g., "18:00" */
    char TimeStopHeat2[8];   /* e.g., "21:00" */
    char GlobalState[16];/*Heating Global State : ARRET / EN CHAUFFE / REGULATION*/
    char State[16];/*Current State of the Heater ON/OFF*/
    char RegulMode[16]; /*Regulation Mode ON/FULLON/OFF*/
    char HeatDuration[8];   /*For immediate heat*/
    char KP[8];//pid stuff
    char KI[8];//pid stuff
    char KD[8];//pid stuff
    float pid_integral;//pid stuff
    float prev_error; //pid stuff
    char CHTitle[16];       /*Immediat Heat Title, example : CH.:1H30*/
    /*For editing Time and Date*/
    bool is_editing;        // True if currently editing a time string
    int  edit_digit_pos;    // Current active character index (0, 1, 3, or 4)
    int edit_lastdigit_pos; /*4(0,1,3,4) for Time, 1(0,1) for TempTarget*/
    int edit_what;          /*0=TimeSchedule(17H30), 1=TempTarget(65°C), 2=TimeClock(17H00), 3=Date(02/11/2026)*/
    char *editing_target_ptr; // Pointer to the char variable in edition
    bool adcReady;
} AppState;

/* Expose a global reference to the application state */
extern AppState app_state;

#endif
