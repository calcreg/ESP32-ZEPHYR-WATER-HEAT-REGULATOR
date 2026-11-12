#include "state.h"
#include <stdbool.h>

/* The single source of truth for your app's global variables */
AppState app_state = {
    .CurrentTime = "--H--",
    .CurrentDate = "--/--/----",
    .TempCurrently = "--@C",
    .TempMax = "--@C",
    .TempMin = "--@C",
    .TempTarget = "28@C",
    .TempMaxAct = "--.-@C/--.-@C",
    .TimeStartHeat1 = "02H00",
    .TimeStopHeat1  = "08H00",
    .slot2 = "OFF",
    .TimeStartHeat2 = "20H00",
    .TimeStopHeat2  = "22H00",
    .GlobalState = "-- ARRET --",
    .State = "OFF",
    .RegulMode = "OFF",
    .HeatDuration = "0H15",
    .CHTitle = "CH.: 0H15",
    .KP = "2.00",
    .KI = "0.05",
    .KD = "1.00",
    .adcReady = false,
    .pid_integral = 0.0f, // Reset PID state
    .prev_error = 0.0f
};
