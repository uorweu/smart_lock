#include "pir.h"
#include "gpio.h"
#include "driver/gpio.h"
#include "esp_log.h"

void pir_init(void) {
    // 1. Setup the Outside PIR Sensor & LED
    gpio_reset_pin(PIR_PIN);
    gpio_set_direction(PIR_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIR_PIN, GPIO_FLOATING); 
    
    gpio_reset_pin(PIR_LED_PIN);
    gpio_set_direction(PIR_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(PIR_LED_PIN, 0); 

    // 2. Setup the Inside PIR Sensor & LED
    gpio_reset_pin(PIR_IN_PIN);
    gpio_set_direction(PIR_IN_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIR_IN_PIN, GPIO_FLOATING); 
    
    gpio_reset_pin(PIR_IN_LED_PIN);
    gpio_set_direction(PIR_IN_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(PIR_IN_LED_PIN, 0);
}

bool pir_is_motion_detected(void) {
    bool motion_detected = false;

    // Check Outside Sensor
    if (gpio_get_level(PIR_PIN) == 1) {
        gpio_set_level(PIR_LED_PIN, 1); 
        motion_detected = true;
    } else {
        gpio_set_level(PIR_LED_PIN, 0); 
    }

    // Check Inside Sensor (DISABLED - Floating pin causes false motion and keeps screen awake!)
    /*
    if (gpio_get_level(PIR_IN_PIN) == 1) {
        gpio_set_level(PIR_IN_LED_PIN, 1); 
        motion_detected = true;
    } else {
        gpio_set_level(PIR_IN_LED_PIN, 0); 
    }
    */

    // Return true if EITHER sensor sees motion
    return motion_detected;
}
