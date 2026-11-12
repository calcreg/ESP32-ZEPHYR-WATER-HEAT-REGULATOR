#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "state.h"
#include <zephyr/logging/log.h>

#define REGUL_HEAT 1
#define REGUL_NOHEAT 0

extern int run_heating_regulation();
