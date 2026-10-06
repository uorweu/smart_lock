# ESP32-C6 Smart Lock - GPIO Wiring Reference

This document maps all the GPIO pins used in the smart lock project based on the current firmware code (`main/inc/gpio.h`).

## I2C Display (LCD)
| GPIO Pin | Name in Code | Direction | Notes |
| :--- | :--- | :--- | :--- |
| **GPIO 5** | `I2C_SDA_PIN` | In/Out | I2C Data line for LCD (Address 0x27) |
| **GPIO 4** | `I2C_SCL_PIN` | Output | I2C Clock line for LCD |

## 4x4 Matrix Keypad
*Note: Row pins are set to output (LOW when scanning), and Column pins are inputs with Pull-Down resistors enabled.*

| GPIO Pin | Name in Code | Direction | Notes |
| :--- | :--- | :--- | :--- |
| **GPIO 21** | `KEYPAD_L1` | Output | Row 1 |
| **GPIO 20** | `KEYPAD_L2` | Output | Row 2 |
| **GPIO 19** | `KEYPAD_L3` | Output | Row 3 |
| **GPIO 18** | `KEYPAD_L4` | Output | Row 4 |
| **GPIO 16** | `KEYPAD_C1` | Input | Column 1 (Pull-Down) |
| **GPIO 17** | `KEYPAD_C2` | Input | Column 2 (Pull-Down) |
| **GPIO 23** | `KEYPAD_C3` | Input | Column 3 (Pull-Down) |
| **GPIO 22** | `KEYPAD_C4` | Input | Column 4 (Pull-Down) |

## Sensors (Inputs)
| GPIO Pin | Name in Code | Direction | Notes |
| :--- | :--- | :--- | :--- |
| **GPIO 11** | `PIR_PIN` | Input | Outside Motion Sensor (Floating, driven by PIR) |
| **GPIO 7** | `PIR_IN_PIN` | Input | Inside Motion Sensor (Floating, driven by PIR) |
| **GPIO 13** | `REED_SWITCH_PIN` | Input | Magnetic Door Sensor (Internal Pull-Up enabled). HIGH = Door open. |
| **GPIO 8** | `BTN_UNLOCK_PIN` | Input | Inside Handle Button (Internal Pull-Up enabled). |
| **GPIO 10** | `BTN_LOCK_PIN` | Input | Defined in `gpio.h` but currently unused in `handle.c`. |
| **GPIO 15** | `DEV_BUTTON_PIN` | Input | Developer Override Button (Internal Pull-Up enabled). |

## Actuators & Indicators (Outputs)
| GPIO Pin | Name in Code | Direction | Notes |
| :--- | :--- | :--- | :--- |
| **GPIO 1** | `RELAY_PIN` | Output | Drives 5V Relay for 12V Solenoid. Configured as **Open-Drain**. |
| **GPIO 6** | `BUZZER_PIN` | Output | PWM output for alarm/melodies (`LEDC_TIMER_0`). |
| **GPIO 3** | `PIR_LED_PIN` | Output | Outside motion indicator LED. |
| **GPIO 14** | `PIR_IN_LED_PIN` | Output | Inside motion indicator LED. |

---
**Hardware Notes & Warnings:**
* **Pins 24 - 30** are intentionally avoided as they are hardwired to the internal SPI Flash memory.
* **Pins 12 & 13** are USB data pins. Bootloader pulses can cause unintended triggers on sensitive relays, which is why the relay is isolated to GPIO 1.
* **Power Requirements:** The Wi-Fi radio and the 12V Solenoid draw significant current spikes. Ensure capacitors are placed on the power lines to prevent brownout boot-loops.
