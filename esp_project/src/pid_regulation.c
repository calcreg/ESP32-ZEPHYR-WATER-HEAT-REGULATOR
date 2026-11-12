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
    
    float current_temp_float = extract_float_from(app_state.TempMax);
    /*for test only:
    float current_temp_float = extract_float_from(app_state.TempCurrently);*/
    float target_temp_float = extract_float_from(app_state.TempTarget);
    /*PID Regulation calculation and Heat/NoHeat determination*/
    float KP = extract_float_from(app_state.KP);
    float KI = extract_float_from(app_state.KI);
    float KD = extract_float_from(app_state.KD);

    float error = target_temp_float - current_temp_float;

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
    app_state.pid_output = p_term + i_term + d_term;
    LOG_INF("pid_output = %d, error=%d",(int)app_state.pid_output, (int)error);

    /*pid_output = 10 => 0H30 heating
                = 20 => 1H00 
                = 40 => 2H00 
                ...
                return (app_state.pid_output * 3) minutes
                */
    if (app_state.pid_output <= 0) return 0;
    else if (app_state.pid_output <= 60) return (int)(3.0f * app_state.pid_output);
    else return 6 * 30; //should rarely happen otherwise modify PID params
}

