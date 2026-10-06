# 6. Threat Model and Security Mitigations

As an Internet of Things security device, the smart lock operates in an inherently hostile environment where both physical and digital attack vectors must be neutralized. To fulfill the project's security requirements, the system's architecture—spanning the edge hardware, Finite State Machine, and cloud backend—was designed to proactively prevent various misuse cases. 

This chapter outlines a concise threat model, detailing five realistic threat vectors the system can prevent and the specific engineering mitigations used to achieve this.

## 6.1 Threat Vector 1: Network Eavesdropping (Packet Sniffing)
**The Attack / Misuse Case:** 
An attacker gains access to the local Wi-Fi network and uses packet-sniffing tools (like Wireshark) to silently capture the wireless traffic between the smart lock and the server. Their goal is to extract plaintext passwords or intercept administrative unlock commands.

**How We Prevent It (Mitigation):** 
The system employs a strict zero-knowledge architecture to protect data in transit. Plaintext PINs are never transmitted over the network; they are hashed locally on the ESP32 using the `mbedTLS` library (SHA-256). Furthermore, all telemetry and commands are routed over MQTT through a heavily encrypted TLS v1.2 tunnel (Port 8883). Even if the packets are intercepted, the attacker only sees mathematically indecipherable ciphertext, rendering sniffing completely ineffective.

## 6.2 Threat Vector 2: Rogue MQTT Broker Impersonation (Man-in-the-Middle)
**The Attack / Misuse Case:** 
A sophisticated attacker performs an ARP spoofing attack on the local network, redirecting the ESP32's traffic to their own malicious MQTT broker. By tricking the lock into connecting to their fake server, the attacker hopes to push a fake "Remote Unlock" payload down to the physical lock.

**How We Prevent It (Mitigation):** 
The system defends against this by utilizing Asymmetric Cryptography. The dedicated host gateway acts as its own Certificate Authority (CA) and generates a unique Public Key. This `.PEM` certificate is hardcoded as a constant string variable directly into the ESP32's C firmware. When the ESP32 connects to the network, it cryptographically verifies the broker's identity. If an attacker's rogue broker attempts to present a fake certificate, the ESP32 instantly recognizes the mismatch, rejects the TLS handshake, and drops the connection before any commands can be exchanged.

## 6.3 Threat Vector 3: Physical Theft of the Edge Node (Memory Extraction)
**The Attack / Misuse Case:** 
An intruder physically rips the ESP32-C6 microcontroller off the wall, takes it to a lab, and uses serial debugging tools to dump the silicon's flash memory. They aim to extract the master PIN or use the lock's network credentials to issue unlock commands to other doors in the building.

**How We Prevent It (Mitigation):** 
First, the master PIN is stored in the ESP32's Non-Volatile Storage (NVS) strictly as a one-way SHA-256 hash, meaning the attacker cannot reverse-engineer the original passcode from the memory dump. Second, the Mosquitto MQTT Broker enforces strict Role-Based Access Control (RBAC) lists on a per-device basis. The embedded credentials on the stolen lock only grant "Publish" rights for telemetry data. Even if the attacker extracts the Wi-Fi and MQTT credentials, the broker will categorically block the stolen node from subscribing to or issuing administrative `cmd/unlock` commands to other locks.

## 6.4 Threat Vector 4: Audit Log Tampering (Database File Theft)
**The Attack / Misuse Case:** 
A malicious insider gains unauthorized access to the local host machine (e.g., the laptop acting as the gateway) and attempts to open, alter, or delete the SQLite database file to cover up their tracks after an unauthorized entry.

**How We Prevent It (Mitigation):** 
To ensure data at rest remains secure and immutable, the backend architecture integrates `SQLCipher`. This open-source extension transparently applies 256-bit AES full-database encryption directly to the binary SQLite file. Because the `.db` file is encrypted on the hard drive, anyone attempting to open the file with a standard text editor or database browser will only see corrupted gibberish. The historical audit logs remain entirely encrypted and tamper-proof without the decryption key embedded securely within the Python backend environment.

## 6.5 Threat Vector 5: Malicious Wire-Cutting (Power Loss Bypass)
**The Attack / Misuse Case:** 
An attacker purposefully severs the 12V power lines supplying the smart lock on the outside of the door, or the facility experiences a total, sudden power outage, with the expectation that the lock's motor will disengage and grant free entry.

**How We Prevent It (Mitigation):** 
The physical locking mechanism relies on a heavy-duty solenoid driven by an NPN transistor and a 5V relay module. Crucially, the 12V power supply to the solenoid is wired through the **Normally Open (NO)** terminal of the relay. This creates a strict "Fail-Secure" configuration. The system requires an active electrical signal from the ESP32 to close the relay and retract the locking pin. In the event of severed wires, a microcontroller crash, or a facility power loss, the relay de-energizes and breaks the circuit. The solenoid violently springs shut, ensuring the door defaults to a permanently locked state during an emergency.
