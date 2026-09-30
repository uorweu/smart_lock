#ifndef GPIO_H
#define GPIO_H

// --- I2C LCD PINS ---
#define I2C_SDA_PIN 5
#define I2C_SCL_PIN 4

// --- KEYPAD COLUMN PINS (Inputs) ---
#define KEYPAD_C1 2
#define KEYPAD_C2 3
#define KEYPAD_C3 23
#define KEYPAD_C4 22

// --- KEYPAD LINE/ROW PINS (Outputs) ---
#define KEYPAD_L1 21
#define KEYPAD_L2 20
#define KEYPAD_L3 19
#define KEYPAD_L4 18

// --- BUZZER ---
#define BUZZER_PIN 6

// --- SENSORS & LOCK ---
#define PIR_PIN          11  // Motion Sensor (Wakes up the screen)
#define RELAY_PIN        12  // Solenoid Lock Control
#define REED_SWITCH_PIN  13  // Magnetic Door Sensor (Checks if door is physically open/closed)

// --- MECHANICAL HANDLE SIMULATION BUTTONS ---
#define BTN_LOCK_PIN   10  // Press to lock the door (when closed)
#define BTN_UNLOCK_PIN  8  // Press to unlock from the inside
#endif
