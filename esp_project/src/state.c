#include "state.h"
#include <stdbool.h>

/* The single source of truth for your app's global variables */
AppState app_state = {
    .CurrentTime = "--H--",
    .CurrentDate = "--/--/----",
    .TempCurrently = "--@C",
    .TempMax = "--@C",
    .TempMin = "--@C",
    .TempTarget = "60@C",
    .TimeStartHeat1 = "06H00",
    .TimeStopHeat1  = "08H00",
    .TimeStartHeat2 = "18H00",
    .TimeStopHeat2  = "21H00",
    .GlobalState = "-- ARRET --",
    .State = "OFF",
    .RegulMode = "OFF",
    .HeatDuration = "0H15",
    .CHTitle = "CH.: 0H15",
    .adcReady = false
};
