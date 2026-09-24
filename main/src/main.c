    #include <stdio.h>
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "lcd.h"
    #include <unistd.h>

    void app_main(void) {
      lcd_init();
        // 1. Setup our pointer

      while (1) {
        lcd_clear();
        lcd_set_cursor(4, 1);
        // 2. Loop through the pointer one letter at a time
        const char *str = "Interstella";
        while (*str) {
            lcd_put_character(*str);                        // Print ONE letter
            vTaskDelay(200 / portTICK_PERIOD_MS);  // Wait 200ms before printing the next one!
            str++;                                 // Move the pointer to the next letter
        }

        // 3. Keep the program alive forever
        }
    }
