#ifndef GPIO_H
#define GPIO_H

// --- I2C LCD PINS ---
#define I2C_SDA_PIN 5
#define I2C_SCL_PIN 4

// --- KEYPAD COLUMN PINS (Inputs) ---
#define KEYPAD_C1 16 // Disconnected (Dead pin)
#define KEYPAD_C2 17 // Disconnected (Dead pin)
#define KEYPAD_C3 23 // Used for 3, 6, 9, #
#define KEYPAD_C4 22 // Used for A, B, C, D

// --- DEVELOPER OVERRIDE BUTTON ---
#define DEV_BUTTON_PIN 15 // Interrupts the Alarm

// --- KEYPAD LINE/ROW PINS (Outputs) ---
#define KEYPAD_L1 21
#define KEYPAD_L2 20
#define KEYPAD_L3 19
#define KEYPAD_L4 18

// --- BUZZER ---
#define BUZZER_PIN 6

// --- SENSORS & LOCK ---
#define PIR_PIN          11  // Motion Sensor (Wakes up the screen)
#define PIR_LED_PIN      3   // Software-controlled Indicator LED
#define PIR_IN_PIN       7   // Moved off SPI flash pins!
#define PIR_IN_LED_PIN   14  // Moved off SPI flash pins!
#define RELAY_PIN        10  // MOVED from 12 to 10 to fix USB bootloop crash!
#define REED_SWITCH_PIN  13  // Magnetic Door Sensor (Checks if door is physically open/closed)

// --- MECHANICAL HANDLE SIMULATION BUTTONS ---
#define BTN_LOCK_PIN   10  // Press to lock the door (when closed)
#define BTN_UNLOCK_PIN  8  // Press to unlock from the inside
#endif
