    #include <stdio.h>
    #include <stdbool.h>
    #include <string.h>


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

    #include "nvs_flash.h"
    #include "nvs.h"
    uint8_t STORED_SECRET_HASH[32] = {
        0x83, 0xcf, 0x8b, 0x60, 0x9d, 0xe6, 0x00, 0x36,
        0xa8, 0x27, 0x7b, 0xd0, 0xe9, 0x61, 0x35, 0x75,
        0x1b, 0xbc, 0x07, 0xeb, 0x23, 0x42, 0x56, 0xd4,
        0xb6, 0x5b, 0x89, 0x33, 0x60, 0x65, 0x1b, 0xf2
    };

    void load_secret_hash(void) {
        nvs_handle_t my_handle;
        if (nvs_open("storage", NVS_READWRITE, &my_handle) == ESP_OK) {
            size_t required_size = 32;
            nvs_get_blob(my_handle, "hash", STORED_SECRET_HASH, &required_size);
            nvs_close(my_handle);
        }
    }

    void update_secret_hash(const uint8_t new_hash[32]) {
        memcpy(STORED_SECRET_HASH, new_hash, 32);
        nvs_handle_t my_handle;
        if (nvs_open("storage", NVS_READWRITE, &my_handle) == ESP_OK) {
            nvs_set_blob(my_handle, "hash", STORED_SECRET_HASH, 32);
            nvs_commit(my_handle);
            nvs_close(my_handle);
        }
    }

    static LockState current_state = STATE_SLEEP;

    static char passcode[11];
    static int passcode_index = 0;
    static int attempts_left = 3;
    static bool is_sleeping = false;     
    static bool show_plaintext = false;  

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
        { STATE_ENTERING_PIN, EVENT_PIN_WRONG,     STATE_DENIED },
        { STATE_DENIED,       EVENT_TIMEOUT,       STATE_ENTERING_PIN },

        { STATE_UNLOCKED,     EVENT_DOOR_AUTHORIZED_OPEN, STATE_AUTHORIZED_OPENING },
        { STATE_AUTHORIZED_OPENING, EVENT_DOOR_CLOSED,    STATE_SLEEP },

        { STATE_ALARM,        EVENT_DEV_OVERRIDE,     STATE_RECOVERY },
        { STATE_RECOVERY,     EVENT_TIMEOUT,          STATE_SLEEP },
        
        { STATE_UNLOCKED,     EVENT_DEV_OVERRIDE,     STATE_SLEEP },
        { STATE_ENTERING_PIN, EVENT_DEV_OVERRIDE,     STATE_SLEEP },
        { STATE_LOCKED_OUT,   EVENT_DEV_OVERRIDE,     STATE_SLEEP }
    };

    #define NUM_TRANSITIONS (sizeof(state_transitions) / sizeof(state_transitions[0]))

    #include "mqtt.h"

    void state_machine_process_event(LockEvent event) {
        if (event == EVENT_NONE) return;

        for (int i = 0; i < NUM_TRANSITIONS; i++) {
            if (state_transitions[i].from_state == current_state &&
                state_transitions[i].event == event) {
                current_state = state_transitions[i].to_state;
                

                mqtt_publish_state(current_state);
                
                return;
            }
        }
    }

    void state_machine_run(void) {
        char key = keypad_get_key();


        bool key_pressed = (key != '\0');

        if (dev_override_flag) {
            dev_override_flag = false;
            state_machine_process_event(EVENT_DEV_OVERRIDE);
        }


        static bool first_run = true;
        static bool last_door_open = false;
        
        if (first_run) {
            last_door_open = reed_is_door_open();
            first_run = false;
        }

        bool current_door_open = reed_is_door_open();
        
        if (current_door_open && !last_door_open) {

            if (current_state == STATE_UNLOCKED) {
                state_machine_process_event(EVENT_DOOR_AUTHORIZED_OPEN);
            } else if (current_state != STATE_AUTHORIZED_OPENING) {
                state_machine_process_event(EVENT_DOOR_FORCED_OPEN);
            }
        }
        
        if (!current_door_open && last_door_open) {

            if (current_state == STATE_AUTHORIZED_OPENING) {
                state_machine_process_event(EVENT_DOOR_CLOSED);
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


                if (key_pressed || pir_is_motion_detected()) {
                    if (key_pressed) {
                        melody_key_press();
                    }


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

                if ((xTaskGetTickCount() - last_activity_time) > pdMS_TO_TICKS(5000)) {

                    is_sleeping = false;
                    state_machine_process_event(EVENT_TIMEOUT);
                    break;
                }

                if (key_pressed || pir_is_motion_detected()) { last_activity_time = xTaskGetTickCount(); }

            if (key_pressed) {
                    melody_key_press();


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
                            

                            lock_open(); 

                            melody_success();
                            
                            state_machine_process_event(EVENT_PIN_CORRECT);
    			} else {
                            attempts_left--;
                            if (attempts_left <= 0) {
                                state_machine_process_event(EVENT_MAX_ATTEMPTS);
                            } else {
                                state_machine_process_event(EVENT_PIN_WRONG);
                            }
    			}
                    }
                }
                break;

            case STATE_DENIED:
                update_lcd_ui(passcode_index, attempts_left, 2);
                melody_error();
                vTaskDelay(pdMS_TO_TICKS(1000));

                passcode_index = 0;
                memset(passcode, 0, sizeof(passcode));
                update_lcd_ui(passcode_index, attempts_left, 0);

                last_activity_time = xTaskGetTickCount();
                state_machine_process_event(EVENT_TIMEOUT);
                break;

            case STATE_UNLOCKED:
                lcd_clear();
                lcd_set_cursor(0, 1);
                lcd_put_string("   DOOR UNLOCKED!   ");

                lock_open();



                bool entered_auth_opening = false;


                while (1) {
                    if (dev_override_flag) {
                        break;
                    }
                    if (handle_is_pressed()) {
                        
                        break;
                    }
                    if (reed_is_door_open()) {
                        entered_auth_opening = true;
                        break;
                    }
                    vTaskDelay(pdMS_TO_TICKS(100));
                }

                if (entered_auth_opening) {
                    state_machine_process_event(EVENT_DOOR_AUTHORIZED_OPEN);
                } else {
                    lock_close();
                    is_sleeping = false;
                    state_machine_process_event(EVENT_TIMEOUT);
                }
                break;

            case STATE_AUTHORIZED_OPENING:
                lcd_clear();
                lcd_set_cursor(0, 1);
                lcd_put_string(" DOOR IS OPEN... ");

                while (1) {
                    if (dev_override_flag) break;
                    
                    if (!reed_is_door_open()) {

                        break;
                    }
                    vTaskDelay(pdMS_TO_TICKS(100));
                }

                lock_close(); 
                is_sleeping = false;
                state_machine_process_event(EVENT_DOOR_CLOSED); 
                break;
                
            case STATE_RECOVERY:
                lcd_clear();
                lcd_set_cursor(0, 1);
                lcd_put_string(" SYSTEM RECOVERY ");
                

                vTaskDelay(pdMS_TO_TICKS(3000));
                
                is_sleeping = false;
                state_machine_process_event(EVENT_TIMEOUT);
                break;

            case STATE_LOCKED_OUT:
                lcd_clear();
                lcd_set_cursor(0, 1);
                lcd_put_string(" SYSTEM LOCKED OUT! ");

                melody_locked_out();

                for (int i = 0; i < 80; i++) {
                    if (dev_override_flag) break;
                    

                    if (reed_is_door_open()) {
                        state_machine_process_event(EVENT_DOOR_FORCED_OPEN);
                        return; 
                    }
                    
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
                
                while (1) {
                    if (dev_override_flag) {
                        break; 
                    }
                    
                    melody_alarm(); 
                    
                    if (dev_override_flag) {
                        break; 
                    }
                    
                    char a_key = keypad_get_key();
                    if (a_key != '\0') {
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
                                break; 
                            }
                            passcode_index = 0; 
                            memset(passcode, 0, sizeof(passcode));
                        }
                    }
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
                break;
        }
    }
