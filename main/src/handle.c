#include "handle.h"
#include "driver/gpio.h"
#include "gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void handle_init(void) {
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << BTN_UNLOCK_PIN);
    io_conf.mode         = GPIO_MODE_INPUT;
    io_conf.pull_up_en   = GPIO_PULLUP_ENABLE; 
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type    = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
}

bool handle_is_pressed(void) {
    return (gpio_get_level(BTN_UNLOCK_PIN) == 0); 
}

void handle_wait_for_release(void) {
    vTaskDelay(pdMS_TO_TICKS(200)); // Debounce the press
    while (handle_is_pressed()) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelay(pdMS_TO_TICKS(200)); // Debounce the release
}
