# IoT Cloud Architecture & Task List

This document breaks down the unchecked items from your "Mandatory IoT, Backend, and Application Requirements" into a step-by-step technical roadmap.

## 1. The MQTT Broker (The Messaging Hub)
To satisfy: *"Use authenticated MQTT; separate telemetry, alarm, and command authorization"*
* **Task:** Spin up a cloud server (AWS, DigitalOcean, or local Raspberry Pi).
* **Task:** Install **Eclipse Mosquitto** (MQTT Broker).
* **Task:** Enable **Authentication** (create a username and password file for Mosquitto so no one can hijack your lock).
* **Task:** Configure **Topics**. You need to design a clean topic structure, for example:
  * `smartlock/telemetry/status` (For normal door opens/closes)
  * `smartlock/telemetry/alarm` (For forced entry)
  * `smartlock/command/unlock` (For the dashboard to send commands down to the ESP32)

## 2. ESP32 Network Firmware
To satisfy: *"Use LWT/equivalent availability detection"*
* **Task:** Add Wi-Fi connection logic to your ESP32 `main.c`.
* **Task:** Add an MQTT Client to your ESP32.
* **Task:** Configure **LWT (Last Will and Testament)**. When the ESP32 connects to the broker, it registers an LWT message (e.g., `"Lock is Offline"`). If the ESP32 loses power or Wi-Fi, the Broker *automatically* publishes this message so your dashboard knows the lock died.

## 3. The Backend Server & Database (The Brain)
To satisfy: *"Backend must record access attempts... provide REST queries"*
* **Task:** Pick a language (Node.js/Express or Python/FastAPI) and a Database (MongoDB or PostgreSQL).
* **Task:** Write an MQTT Listener script. This script connects to Mosquitto, listens to `smartlock/telemetry/#`, and saves every event into the Database with a Timestamp.
* **Task:** Write **REST API Endpoints**. Create web URLs that your dashboard can ask for data (e.g., `GET /api/history` to get the last 50 door events).

## 4. The Dashboard & Security (Advanced Component)
To satisfy: *"Real-time dashboard... TLS and token-based dashboard/API authorization, or role-based permissions"*
* **Task:** Build a Web UI (HTML/JS, React, or Vue).
* **Task:** Implement a Login page for the Dashboard.
* **Task:** Implement **JWT (JSON Web Tokens)** or Role-based access. 
  * *Example:* If someone logs in as an "Admin", the dashboard shows the "Remote Unlock" button. If they log in as a "Viewer", the button is hidden, and they can only see the history graph.
* **Task:** Ensure the Dashboard securely calls your REST API to get the history and display it in a table or graph.
