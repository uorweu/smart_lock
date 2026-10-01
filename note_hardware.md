# Smart Lock Hardware Notes

## 1. The 12V Solenoid Inrush Crash
**Symptoms:** 
When the system tries to unlock the door (either by pressing the inside handle button on GPIO 8, or entering the correct PIN), the entire ESP32 system instantly shuts off and the screen goes blank.

**The Cause:** 
Solenoids are massive electrical magnets. The exact millisecond they turn on, they draw a massive amount of "inrush current". Because the Solenoid and the Buck Converter (which powers the ESP32) share the exact same 12V power supply wall adapter, the Solenoid sucks all the voltage out of the wires. This starves the Buck Converter, which causes the ESP32 to instantly lose power and die.

**The Fix:**
To fix this permanently, the hardware needs to provide enough stable power so the Buck Converter never starves:
1. **Bigger Power Supply:** Use a 12V adapter with a higher Amperage rating (e.g. 2 Amps or 3 Amps).
2. **Capacitor Buffer:** Wire a large Electrolytic Capacitor (e.g. 1000µF 16V or higher) across the 12V and GND input pins of the Buck Converter. This acts as a tiny battery buffer. When the solenoid sucks the power down, the capacitor feeds the Buck Converter to keep the ESP32 alive during the split-second voltage drop.

## 2. 5V Relay vs 3.3V ESP32 (Open-Drain Fix)
**Symptoms:**
When the system boots, the Relay turns on instantly. The ESP32 tries to turn it off by sending a `HIGH` signal, but the relay ignores it and stays permanently stuck ON.

**The Cause:**
The Relay Module is powered by `5V`, but the ESP32 only outputs `3.3V`. When the ESP32 sends a `3.3V` signal to tell the relay to turn OFF, the relay subtracts the two (`5V - 3.3V = 1.7V`). That 1.7V leaks through the relay's transistor, keeping it permanently stuck ON.

**The Fix:**
In `lock.c`, we configure `GPIO 12` to `GPIO_MODE_OUTPUT_OD` (Open-Drain). Instead of sending `3.3V`, it physically disconnects the pin internally (like a floating wire), completely cutting the 1.7V leak and forcing the relay to shut off.
## 3. The "Cursed" USB Bootloop (Solenoid Holds in on Power-Up)
**Symptoms:**
When you plug in the 12V power source, the Solenoid instantly activates and holds the rod in forever. The ESP32 is completely unresponsive.

**The Cause:**
The Relay was originally connected to `GPIO 12`. On the ESP32-C6, `GPIO 12` and `GPIO 13` are hardwired internally as the USB Data pins (USB_D- and USB_D+). 
When the ESP32 powers on, the internal ROM bootloader automatically sends a 3.3V pulse to `GPIO 12` to check if a computer is plugged into the USB port. 
Because your Active-LOW 5V Relay module is very sensitive to voltage leaks, that tiny 3.3V USB pulse accidentally turned the Relay ON during the boot sequence! 
When the Relay turned on, the heavy Solenoid fired, draining the voltage. This crashed the ESP32 before it could even finish booting. The ESP32 then tried to reboot, sent the USB pulse again, fired the solenoid again, and got stuck in an **Infinite Crash Loop** thousands of times a second. Because the loop was so fast, the Solenoid just appeared to be permanently stuck ON!

**The Fix:**
We moved the Relay off the cursed USB pin and onto `GPIO 10`. The ROM bootloader ignores `GPIO 10`, so the ESP32 boots up in peace without accidentally firing the lock!

## 4. The SPI Flash Crash (Monitor Constantly Refreshing)
**Symptoms:**
The LCD screen doesn't turn on, and the serial monitor constantly prints crash logs and reboots (Panic / WDT Reset).

**The Cause:**
We accidentally assigned the inside PIR sensors to `GPIO 25` and `GPIO 26`. On the ESP32-C6, pins in the `24 - 30` range are physically hardwired inside the chip to the **Internal SPI Flash Memory** chip (which holds all your code). The exact microsecond the code tried to configure `GPIO 25`, it accidentally disconnected the ESP32's processor from its own memory, causing a massive crash!

**The Fix:**
Always avoid pins `24` through `30` on the ESP32-C6. We safely moved the sensors to `GPIO 7` and `GPIO 14`.

## 5. The Muted Reed Switch Alarm (Pin Conflict)
**Symptoms:**
The Reed Switch triggers the Alarm state perfectly, but the buzzer never sounds and the screen instantly returns to the default PIN entry screen.

**The Cause:**
The Developer Override Button (which cancels the alarm) was wired to `GPIO 15`. However, the code was *also* trying to use `GPIO 15` for the Inside PIR Indicator LED. 
Because the LED driver was aggressively fighting the Button driver over the exact same pin, the pin got pulled to GND. This made the ESP32 think the Developer Button was being held down 24/7! 
When you triggered the Reed Switch alarm, it tried to sound the siren, but the "stuck" Developer Button instantly cancelled it in less than 1 millisecond.

**The Fix:**
Never assign two different hardware components to the exact same GPIO pin in the code! We moved the LED to `GPIO 14` so the Developer Button had exclusive control over `GPIO 15`.

## 6. Matrix Keypad Dead Columns
**Symptoms:**
You tried to re-wire the 4x4 keypad using only 2 pins for the columns and 4 pins for the rows, but buttons were pressing the wrong keys or doing nothing.

**The Cause:**
A matrix keypad is a physical grid of wires. You cannot bypass the internal membrane. If you want to use 8 buttons (2 columns and 4 rows), you MUST physically plug in all 6 of those wires into the ESP32, and the ESP32 must configure all 6 GPIOs. You had dead pins on `GPIO 2` and `GPIO 3`, which made you think you could skip plugging them in entirely.

**The Fix:**
We properly mapped 6 healthy pins (Rows: `21, 20, 19, 18` and Columns: `23, 22`) and physically plugged the keypad ribbon cable into those exact pins.
