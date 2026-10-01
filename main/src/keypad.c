#include "keypad.h"
#include "driver/gpio.h"
#include "gpio.h"
#include <stdint.h>

void keypad_init(void){
  gpio_config_t io_conf = {};
  io_conf.pin_bit_mask = (1ULL << KEYPAD_L1) | (1ULL << KEYPAD_L2) | (1ULL << KEYPAD_L3) | (1ULL << KEYPAD_L4);
  io_conf.mode         = GPIO_MODE_OUTPUT;
  io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.intr_type = GPIO_INTR_DISABLE;

  gpio_config(&io_conf);

  // ONLY initialize Column 3 and Column 4!
  io_conf.pin_bit_mask = (1ULL << KEYPAD_C3) | (1ULL << KEYPAD_C4);
  io_conf.mode         = GPIO_MODE_INPUT;
  io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;

  gpio_config(&io_conf);
}

uint8_t keypad_get_key(void){
  uint8_t keymap[4][4] = { 
    {'1', '2','3', 'A'},
    {'4', '5','6', 'B'},
    {'7', '8','9', 'C'},
    {'*', '0','#', 'D'}
  };

  int row_pins[4] = {KEYPAD_L1, KEYPAD_L2, KEYPAD_L3, KEYPAD_L4};
  int col_pins[4] = {KEYPAD_C1, KEYPAD_C2, KEYPAD_C3, KEYPAD_C4};

  for(int row = 0; row < 4; row++){
    gpio_set_level(row_pins[row], 1);
    
    // Start scanning at column index 2 (which skips C1 and C2 completely!)
    for(int col = 2; col < 4; col++){
      if(gpio_get_level(col_pins[col]) == 1){
        gpio_set_level(row_pins[row], 0);
        return keymap[row][col];
      }
    }
    gpio_set_level(row_pins[row],0);
  }
  return '\0';
}

