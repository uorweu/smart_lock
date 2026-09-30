#include "pir.h"
#include "gpio.h"
#include "driver/gpio.h"

void pir_init(void) {
    // Set the PIR pin as an input
    gpio_set_direction(PIR_PIN, GPIO_MODE_INPUT);
    
    // Add a pull-down resistor so it doesn't float randomly if the wire is loose
    gpio_set_pull_mode(PIR_PIN, GPIO_PULLDOWN_ONLY);
}

bool pir_is_motion_detected(void) {
    // Read the hardware register and return true if it's HIGH (1)
    if (gpio_get_level(PIR_PIN) == 1) {
        return true;
    } else {
        return false;
    }
}
