#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// 1. Include our hardware drivers
#include "keypad.h"
#include "lcd.h"
#include "pir.h" // Added PIR sensor!
#include "buzzer.h"
#include "lock.h"
#include "handle.h"
#include "reed.h"
#include "dev_button.h"

// 2. Include our new State Machine!
#include "state_machine.h"

void app_main(void) {
  printf("Starting Smart Lock...\n");

  // Initialize the hardware peripherals
  lcd_init();
  keypad_init();
  pir_init(); // Crucial step so the ESP32 listens to the PIR pin!
  buzzer_init();
  lock_init();
  handle_init();
  reed_init();
  dev_button_init();

  // Clear the screen when the ESP32 first boots up
  lcd_clear();

  // The infinite heartbeat loop
  while (1) {
    // The State Machine Engine handles everything now!
    state_machine_run();

    // Feed the FreeRTOS watchdog timer so the ESP32 doesn't crash (50ms delay)
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
