#include "state.h"

/* The single source of truth for your app's global variables */
AppState app_state = {
    .CurrentTime = "--H--",
    .CurrentDate = "--/--/----",
    .TempCurrently = "--°C",
    .TempMax = "--°C",
    .TempMin = "--°C",
    .TempTarget = "60°C",
    .TimeStartHeat1 = "06:00",
    .TimeStopHeat1  = "08:00",
    .TimeStartHeat2 = "18:00",
    .TimeStopHeat2  = "21:00",
    .State = "-- ARRET --",
    .RegulMode = "OFF",
    .HeatDuration = "0H15",
    .CHTitle = "CH.: 0H15"
};
