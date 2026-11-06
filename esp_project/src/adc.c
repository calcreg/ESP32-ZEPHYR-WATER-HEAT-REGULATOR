#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_adc, LOG_LEVEL_INF);

/* Fetch the ADC1 controller directly by node label */
static const struct device *adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc0));
static uint16_t raw_value;

/* Explicit channel configuration matching your overlay */
static const struct adc_channel_cfg channel_cfg = {
    .gain = ADC_GAIN_1_2,           // 6dB attenuation (0 - 1.5V range)
    .reference = ADC_REF_INTERNAL,  // Internal reference 
    .acquisition_time = ADC_ACQ_TIME_DEFAULT,
    .channel_id = 0,                // Channel 0 -> SENSOR_VP (GPIO36)
};

static struct adc_sequence sequence = {
    .buffer = &raw_value,
    .buffer_size = sizeof(raw_value),
    .resolution = 12,               // 12-bit resolution (0 - 4095)
    .channels = BIT(0),             // Bitmask for Channel 0
};

int adc_init(void) {
    int err;

    if (!device_is_ready(adc_dev)) {
        LOG_ERR("Error: ADC device is not ready\n");
        return -ENODEV;
    }

    /* Setup the ADC channel configuration */
    err = adc_channel_setup(adc_dev, &channel_cfg);
    if (err < 0) {
        LOG_ERR("Failed to setup ADC channel (%d)\n", err);
        return err;
    }
    return 0;
}

float read_ntc_voltage(void) {
    int err;
    uint32_t sum_mv = 0;
    int valid_samples = 0;
    const int num_samples = 32;

    for (int i = 0; i < num_samples; i++) {
        err = adc_read(adc_dev, &sequence);
        if (err == 0) {
            int32_t val_mv = (int32_t)raw_value;
            /* Convert each raw sample to millivolts */
            if (adc_raw_to_millivolts(1100, ADC_GAIN_1_2, 12, &val_mv) >= 0) {
                sum_mv += val_mv;
                valid_samples++;
            }
        }
        /* Short delay between burst samples to let the sample capacitor settle */
        k_busy_wait(100); 
    }

    if (valid_samples == 0) {
        LOG_ERR("Failed to read ADC across sample burst");
        return -1.0f;
    }

    int32_t avg_mv = sum_mv / valid_samples;
    float val_v = (float)avg_mv / 1000.0f;
    
    //LOG_INF("Averaged Voltage: %d mV", avg_mv);
    return val_v;
}

float GetTemperature(float V_ADC){
    /*R_{NTC} = \frac{V_{ADC}}{3.3-V_{ADC}}\times 10 (in kOhms)*/
    float R_NTC = V_ADC / (3.3f-V_ADC) * 100.0f;
    const float a = 0.00665; 
    const float b = -1.34; 
    const float c = 74.93;
    float Temperature = a * R_NTC * R_NTC + b * R_NTC + c;
    return Temperature;
}