    #include <stdio.h>
    #include <stdbool.h>
    #include <string.h>

    // Added for vTaskDelay!
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"

    #include "state_machine.h"
    #include "keypad.h"
    #include "lcd.h"
    #include "sha256.h"
    #include "pir.h"
    #include "buzzer.h"
    #include "lock.h"
    #include "handle.h"
    #include "reed.h"
    #include "dev_button.h"
    
    static TickType_t last_activity_time = 0;

    // The SHA-256 hash for "999"
    const uint8_t STORED_SECRET_HASH[32] = {
        0x83, 0xcf, 0x8b, 0x60, 0x9d, 0xe6, 0x00, 0x36,
        0xa8, 0x27, 0x7b, 0xd0, 0xe9, 0x61, 0x35, 0x75,
        0x1b, 0xbc, 0x07, 0xeb, 0x23, 0x42, 0x56, 0xd4,
        0xb6, 0x5b, 0x89, 0x33, 0x60, 0x65, 0x1b, 0xf2
    };

    static LockState current_state = STATE_SLEEP;

    // --- Tuned Global Variables ---
    static char passcode[11];
    static int passcode_index = 0;
    static char last_key = '\0';
    static int attempts_left = 3;
    static bool is_sleeping = false;     // Tracks if the screen is off
    static bool show_plaintext = false;  // Toggle for the 'B' button

    static const struct state_transition state_transitions[] = {
        { STATE_SLEEP,        EVENT_PIR_MOTION,    STATE_ENTERING_PIN },
        { STATE_SLEEP,        EVENT_KEY_PRESSED,   STATE_ENTERING_PIN },
        { STATE_ENTERING_PIN, EVENT_PIN_CORRECT,   STATE_UNLOCKED },
        { STATE_ENTERING_PIN, EVENT_MAX_ATTEMPTS,  STATE_LOCKED_OUT },
        { STATE_ENTERING_PIN, EVENT_TIMEOUT,       STATE_SLEEP },
        { STATE_UNLOCKED,     EVENT_TIMEOUT,       STATE_SLEEP },
        { STATE_LOCKED_OUT,   EVENT_TIMEOUT,       STATE_SLEEP },
        
        { STATE_SLEEP,        EVENT_INSIDE_HANDLE, STATE_UNLOCKED },
        { STATE_ENTERING_PIN, EVENT_INSIDE_HANDLE, STATE_UNLOCKED },
        { STATE_LOCKED_OUT,   EVENT_INSIDE_HANDLE, STATE_UNLOCKED },

        { STATE_SLEEP,        EVENT_DOOR_FORCED_OPEN, STATE_ALARM },
        { STATE_ENTERING_PIN, EVENT_DOOR_FORCED_OPEN, STATE_ALARM },
        { STATE_LOCKED_OUT,   EVENT_DOOR_FORCED_OPEN, STATE_ALARM },
        
        { STATE_ALARM,        EVENT_PIN_CORRECT,      STATE_UNLOCKED },
        
        { STATE_ALARM,        EVENT_DEV_OVERRIDE,     STATE_SLEEP },
        { STATE_UNLOCKED,     EVENT_DEV_OVERRIDE,     STATE_SLEEP },
        { STATE_ENTERING_PIN, EVENT_DEV_OVERRIDE,     STATE_SLEEP },
        { STATE_LOCKED_OUT,   EVENT_DEV_OVERRIDE,     STATE_SLEEP }
    };

    #define NUM_TRANSITIONS (sizeof(state_transitions) / sizeof(state_transitions[0]))

    void state_machine_process_event(LockEvent event) {
        if (event == EVENT_NONE) return;

        for (int i = 0; i < NUM_TRANSITIONS; i++) {
            if (state_transitions[i].from_state == current_state &&
                state_transitions[i].event == event) {
                current_state = state_transitions[i].to_state;
                return;
            }
        }
    }

    void state_machine_run(void) {
        char key = keypad_get_key();

        // Debounce
        bool key_pressed = (key != '\0' && key != last_key);
        last_key = key;

        if (dev_override_flag) {
            dev_override_flag = false; // Reset the flag
            state_machine_process_event(EVENT_DEV_OVERRIDE);
        }

        // 1. Poll the Reed Switch to detect forced entry
        static bool first_run = true;
        static bool last_door_open = false;
        
        if (first_run) {
            last_door_open = reed_is_door_open();
            first_run = false;
        }

        bool current_door_open = reed_is_door_open();
        
        if (current_door_open && !last_door_open) {
            // The door was just opened!
            if (current_state != STATE_UNLOCKED) {
                state_machine_process_event(EVENT_DOOR_FORCED_OPEN);
            }
        }
        last_door_open = current_door_open;

        if (handle_is_pressed()) {
            state_machine_process_event(EVENT_INSIDE_HANDLE);
        }

        switch (current_state) {

            case STATE_SLEEP:
                if (!is_sleeping) {
                    lcd_clear();
                    is_sleeping = true;
                }

                // Wake up if someone presses a key OR walks in front of the PIR!
                if (key_pressed || pir_is_motion_detected()) {
                    if (key_pressed) {
                        melody_key_press();
                    }

                    // Start the 5-second stopwatch!
                    last_activity_time = xTaskGetTickCount();

                    is_sleeping = false;
                    show_plaintext = false;
                    passcode_index = 0;
                    memset(passcode, 0, sizeof(passcode));

                    update_lcd_ui(passcode_index, attempts_left, 0);

                    if (key_pressed || pir_is_motion_detected()) { last_activity_time = xTaskGetTickCount(); }

            if (key_pressed) {
                        state_machine_process_event(EVENT_KEY_PRESSED);
                    } else {
                        state_machine_process_event(EVENT_PIR_MOTION);
                    }
                }
                break;

            case STATE_ENTERING_PIN:
                // 1. Check the Stopwatch! Has it been 5 seconds (5000ms)?
                if ((xTaskGetTickCount() - last_activity_time) > pdMS_TO_TICKS(5000)) {
                    // Time is up! Go back to sleep.
                    is_sleeping = false; // Force the screen to clear
                    state_machine_process_event(EVENT_TIMEOUT);
                    break; // Stop running the rest of this case
                }

                if (key_pressed || pir_is_motion_detected()) { last_activity_time = xTaskGetTickCount(); }

            if (key_pressed) {
                    melody_key_press();

                    // 2. They pressed a key! Reset the stopwatch back to 0!
                    last_activity_time = xTaskGetTickCount();

                    if (key >= '0' && key <= '9') {
                        if (passcode_index < 10) {
                            passcode[passcode_index] = key;
                            passcode_index++;
                            update_lcd_ui(passcode_index, attempts_left, 0);

                            if (show_plaintext) {
                                lcd_set_cursor(5, 1);
                                lcd_put_string(passcode);
                            }
                        }
                    }
                    else if (key == 'A') {
                        if (passcode_index > 0) {
                            passcode_index--;
                            passcode[passcode_index] = '\0';
                            update_lcd_ui(passcode_index, attempts_left, 0);

                            if (show_plaintext) {
                                lcd_set_cursor(5, 1);
                                lcd_put_string(passcode);
                                lcd_put_character(' ');
                            }
                        }
                    }
                    else if (key == 'C') {
                        passcode_index = 0;
                        memset(passcode, 0, sizeof(passcode));
                        update_lcd_ui(passcode_index, attempts_left, 0);
                    }
                    else if (key == 'B') {
                        show_plaintext = !show_plaintext;
                        if (show_plaintext) {
                            lcd_set_cursor(5, 1);
                            lcd_put_string(passcode);
                        } else {
                            update_lcd_ui(passcode_index, attempts_left, 0);
                        }
                    }
                    else if (key == 'D') {
                        passcode[passcode_index] = '\0';

                        uint8_t input_hash[32];
                        sha256_hash_string(passcode, input_hash);

                        bool is_correct = (memcmp(input_hash, STORED_SECRET_HASH, 32) == 0);

                        if (is_correct) {
                            attempts_left = 3;
                            update_lcd_ui(passcode_index, attempts_left, 1);
                            
                            melody_success(); // Happy tone!
                            vTaskDelay(pdMS_TO_TICKS(700)); // Reduced since melody blocks
                            
                            state_machine_process_event(EVENT_PIN_CORRECT);
    			} else {
                            attempts_left--;
                            update_lcd_ui(passcode_index, attempts_left, 2);
                            
                            if (attempts_left > 0) {
                                melody_error(); // Quick double-beep
                            }
                            vTaskDelay(pdMS_TO_TICKS(1000)); // Reduced since melody blocks

                            passcode_index = 0;
                            memset(passcode, 0, sizeof(passcode));
                            update_lcd_ui(passcode_index, attempts_left, 0);

                            // Because this took 1.5s, we should reset the stopwatch again
                            // so it doesn't instantly timeout while they are reading "DENIED!"
                            last_activity_time = xTaskGetTickCount();

                            if (attempts_left <= 0) {
    				state_machine_process_event(EVENT_MAX_ATTEMPTS);
                            }
    			}
                    }
                }
                break;

            case STATE_UNLOCKED:
                lcd_clear();
                lcd_set_cursor(0, 1);
                lcd_put_string("   DOOR UNLOCKED!   ");

                lock_open(); // Pop the solenoid open!

                handle_wait_for_release(); // Wait for them to let go if they just pressed it!

                // INFINITE LOOP: Hold the solenoid open FOREVER until the button is pressed!
                while (1) {
                    if (dev_override_flag) {
                        break;
                    }
                    if (handle_is_pressed()) {
                        handle_wait_for_release(); // Wait for them to let go
                        break; // Break the infinite loop to lock the door!
                    }
                    vTaskDelay(pdMS_TO_TICKS(100)); // Sleep briefly to save CPU
                }

                lock_close(); // Release the solenoid to lock!

                is_sleeping = false;
                state_machine_process_event(EVENT_TIMEOUT);
                break;

            case STATE_LOCKED_OUT:
                lcd_clear();
                lcd_set_cursor(0, 1);
                lcd_put_string(" SYSTEM LOCKED OUT! ");

                melody_locked_out(); // Sad descending penalty sound

                // Wait for 8 seconds, but allow dev override!
                for (int i = 0; i < 80; i++) {
                    if (dev_override_flag) break;
                    vTaskDelay(pdMS_TO_TICKS(100));
                }

                attempts_left = 3;
                is_sleeping = false;
                state_machine_process_event(EVENT_TIMEOUT);
                break;

            case STATE_ALARM:
                lcd_clear();
                lcd_set_cursor(0, 1);
                lcd_put_string(" !!! ALARM !!! ");
                
                passcode_index = 0;
                memset(passcode, 0, sizeof(passcode));
                
                // Infinite alarm loop until PIN is correct
                while (1) {
                    if (dev_override_flag) {
                        break; // The main loop will handle the state transition!
                    }
                    
                    melody_alarm(); // Siren sound
                    
                    if (dev_override_flag) {
                        break; // Check again in case it was pressed during the 800ms melody
                    }
                    
                    char a_key = keypad_get_key();
                    if (a_key != '\0' && a_key != last_key) {
                        last_key = a_key;
                        if (a_key >= '0' && a_key <= '9') {
                            if (passcode_index < 10) {
                                passcode[passcode_index] = a_key;
                                passcode_index++;
                            }
                        } else if (a_key == 'C') {
                            passcode_index = 0;
                            memset(passcode, 0, sizeof(passcode));
                        } else if (a_key == 'D') {
                            passcode[passcode_index] = '\0';
                            uint8_t input_hash[32];
                            sha256_hash_string(passcode, input_hash);
                            if (memcmp(input_hash, STORED_SECRET_HASH, 32) == 0) {
                                attempts_left = 3;
                                passcode_index = 0;
                                memset(passcode, 0, sizeof(passcode));
                                melody_success();
                                state_machine_process_event(EVENT_PIN_CORRECT);
                                break; // Break out of alarm loop!
                            }
                            passcode_index = 0; // Wrong pin, reset without feedback
                            memset(passcode, 0, sizeof(passcode));
                        }
                    }
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
                break;
        }
    }
