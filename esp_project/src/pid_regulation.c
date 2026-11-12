#include "pid_regulation.h"

float extract_float_from(char *val_str) {
    if (!val_str) return 0.0f;
    
    float result = 0.0f;
    int i = 0;
    int has_decimal = 0;

 
    // Parse integer part (or digits before the dot)
    while (val_str[i] >= '0' && val_str[i] <= '9') {
        result = result * 10.0f + (val_str[i] - '0');
        i++;
    }

    // Parse fractional part if an explicit decimal point exists
    if (val_str[i] == '.') {
        has_decimal = 1;
        i++;
        float divisor = 10.0f;
        while (val_str[i] >= '0' && val_str[i] <= '9') {
            result += (val_str[i] - '0') / divisor;
            divisor *= 10.0f;
            i++;
        }
    }

    return result;
}


LOG_MODULE_REGISTER(app_pid, LOG_LEVEL_INF);

// Main regulation function to be called periodically (e.g., every minute or every 10 seconds)
int run_heating_regulation()
{
    //TempMax is the Pic Temperature detected lastely, it is meant to be
    //The current temperature inside de bulk water heater
    //Do not mix with the TempCurrently which is the temperature measured on the pipe
    //As hot water is pourred, the pipe heats up and we extract the TempMax
    
    //float current_temp_float = extract_float_from(app_state.TempMax);
    /*for test only:*/
    float current_temp_float = extract_float_from(app_state.TempCurrently);
    float target_temp_float = extract_float_from(app_state.TempTarget);
    /*PID Regulation calculation and Heat/NoHeat determination*/
    float KP = extract_float_from(app_state.KP);
    float KI = extract_float_from(app_state.KI);
    float KD = extract_float_from(app_state.KD);

    float error = target_temp_float - current_temp_float;

    if (error > 5.0f) {
        //error is large from target, therefore Heat on to catchup
        app_state.prev_error = error;
        return REGUL_HEAT;
    }

    // Proportional term
    float p_term = KP * error;

    // Integral term (accumulated error over time)
    app_state.pid_integral += error;
    // Anti-windup clamping
    if (app_state.pid_integral > 100.0f) app_state.pid_integral = 100.0f;
    if (app_state.pid_integral < -100.0f) app_state.pid_integral = -100.0f;
    float i_term = KI * app_state.pid_integral;

    // Derivative term (rate of change)
    float d_term = KD * (error - app_state.prev_error);
    app_state.prev_error = error;

    // Total PID output (can represent duty cycle percentage 0-100%)
    float pid_output = p_term + i_term + d_term;
    LOG_INF("pid_output = %d, error=%d",(int)pid_output, (int)error);

    // Decision rule based on PID output (e.g., Time-Proportioned PWM or simple threshold)
    if (pid_output > 0.0f) {
        return REGUL_HEAT;
    } else {
        return REGUL_NOHEAT;
    }
    //Il faudrait plutot extraire un temps de chauffe en minutes
    // entre 1H00 et 3H00 max soit 60 à 180 mins
    /*pid_output = 5 => 0H30 de chauffe
                = 10 => 1H00 de chauffe
                = 20 => 2H00 de chauffe
                >= 30 => 2H30 de chauffe 
                */
}

