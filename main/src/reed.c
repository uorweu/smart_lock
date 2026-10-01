#include "reed.h"
#include "driver/gpio.h"
#include "gpio.h"

void reed_init(void) {
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << REED_SWITCH_PIN);
    io_conf.mode         = GPIO_MODE_INPUT;
    io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;  // Pull-up enabled!
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type    = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
}

bool reed_is_door_open(void) {
    // If the magnet is close (door closed), switch is closed, pulling to GND (0).
    // If magnet is away (door open), switch opens, pulled HIGH (1) by resistor.
    return (gpio_get_level(REED_SWITCH_PIN) == 1);
}
