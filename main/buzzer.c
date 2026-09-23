#include "buzzer.h"
#include "freertos/FreeRTOS.h"
#include "freerrtos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
int current_buzzer_pin = -1;
void buzzer_init(int pin){
  current_buzzer_pin = pin;
  gpio_reset_pin(current_buzzer_pin);
  ledc_timer_config_t ledc_timer = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .timer_num = LEDC_TIMER_0,
    .
  }
}
