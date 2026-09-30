# Smart Lock State Machine

## Overview
This document outlines the master logic flow for the Smart Lock. The hierarchical logic has been flattened into 5 primary states to be programmed using a Function Pointer State Machine in C. It includes the physical hardware buttons used to simulate mechanical door handles.

## The 5 Core States

### 1. `STATE_SLEEP` (Low Power / Monitoring / Door Locked)
* **What it does:** The LCD is completely OFF to save power. The system sits quietly, and the door is LOCKED.
* **Transitions:**
  * If **PIR Sensor** detects motion -> Go to `STATE_ENTERING_PIN`
  * If **Inside UNLOCK Button** (GPIO 8) is pressed -> Go directly to `STATE_UNLOCKED`
  * If **Reed Switch** opens (door is forced open without unlocking!) -> Go to `STATE_ALARM`

### 2. `STATE_ENTERING_PIN` (Active UI)
* **What it does:** Wakes up the LCD. Shows "Enter Passcode:". Accepts keypad input.
* **Transitions:**
  * If PIN is **Correct** -> Go to `STATE_UNLOCKED`
  * If PIN is **Incorrect** (under 5 tries) -> Clear screen, wait for retry.
  * If PIN is **Incorrect** (5 tries reached) -> Go to `STATE_LOCKED_OUT`
  * If **Inside UNLOCK Button** (GPIO 8) is pressed -> Go directly to `STATE_UNLOCKED`
  * If **Reed Switch** opens (thief breaks door while typing) -> Go to `STATE_ALARM`
  * If user walks away (timeout of 30 seconds) -> Go to `STATE_SLEEP`

### 3. `STATE_UNLOCKED` (Access Granted / Door Unlocked)
* **What it does:** Plays success melody. Triggers the Relay to unlock the door. Temporarily ignores the Reed Switch security check so the user can freely open and close the door.
* **Transitions:**
  * If **LOCK Button** (GPIO 10) is pressed AND the **Reed Switch** confirms the door is physically closed -> Lock Solenoid -> Go to `STATE_SLEEP`

### 4. `STATE_LOCKED_OUT` (Penalty Box)
* **What it does:** LCD displays "BLOCKED". Ignores all keypad input.
* **Transitions:**
  * Wait 10 minutes -> Go back to `STATE_SLEEP`
  * If **Inside UNLOCK Button** (GPIO 8) is pressed -> Go directly to `STATE_UNLOCKED` (Safety override to let people out of the house!)
  * If **Reed Switch** opens (door forced) -> Go to `STATE_ALARM`

### 5. `STATE_ALARM` (Security Breach)
* **What it does:** The door was opened without a valid PIN! Blasts the buzzer at max volume. LCD flashes "ALARM!".
* **Transitions:**
  * Stays in this state forever until a hard reset, or until a Master Code is typed on the keypad.

---

## IoT & Cloud Integration Notes
We do **not** need to create separate states for the Wi-Fi/Cloud! Because we are using FreeRTOS, the MQTT Cloud Logic will run as a completely separate "Background Task". 
* **Telemetry:** Whenever the state changes (e.g., from `SLEEP` to `ALARM`), the State Machine simply tells the MQTT task: *"Hey, publish an alert to the dashboard!"*
* **Remote Control:** If the cloud sends a remote "Unlock" command, the MQTT task simply forces the lock into `STATE_UNLOCKED`. 
This keeps the local hardware logic 100% separate from the networking logic, ensuring the lock works offline perfectly.
