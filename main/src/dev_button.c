#include "dev_button.h"
#include "driver/gpio.h"
#include "gpio.h"
#include "esp_attr.h"

volatile bool dev_override_flag = false;

static void IRAM_ATTR dev_button_isr_handler(void* arg) {
    dev_override_flag = true;
}
void dev_button_init(void) {
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << DEV_BUTTON_PIN);
    io_conf.mode         = GPIO_MODE_INPUT;
    io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;   
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type    = GPIO_INTR_NEGEDGE;    

    gpio_config(&io_conf);
    gpio_install_isr_service(0); 
    
    gpio_isr_handler_add(DEV_BUTTON_PIN, dev_button_isr_handler, NULL);
}
