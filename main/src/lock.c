#include "lock.h"
#include "driver/gpio.h"
#include "gpio.h"

void lock_init(void) {
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << RELAY_PIN);
    io_conf.mode         = GPIO_MODE_OUTPUT_OD; // OPEN-DRAIN to fix the 1.7V leak!
    io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type    = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
    
    lock_close(); // Lock the door by default!
}

void lock_open(void) {
    // Ground the IN pin to trigger the Active-LOW relay
    gpio_set_level(RELAY_PIN, 0);
}

void lock_close(void) {
    // Float the IN pin to turn off the relay
    gpio_set_level(RELAY_PIN, 1);
}
