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
#include "wifi_connection.h"
#include "mqtt.h"

// 2. Include our new State Machine!
#include "state_machine.h"

void app_main(void) {
  printf("Starting Smart Lock...\n");

  // Initialize the hardware peripherals
  lcd_init();
  keypad_init();
  pir_init(); 
  buzzer_init();
  lock_init();
  handle_init();
  reed_init();
  dev_button_init();
  wifi_init_hotspot();
  mqtt_init();

  lcd_clear();

  while (1) {
        static bool first = true; if(first) { load_secret_hash(); first = false; }
    state_machine_run();

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
