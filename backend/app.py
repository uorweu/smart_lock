import sqlite3
import datetime
import threading
from flask import Flask, request, jsonify, render_template_string, session, redirect
import paho.mqtt.client as mqtt

MQTT_BROKER = "192.168.1.23" 
MQTT_PORT = 8883

CA_CERTS = "/home/norman/certs/ca.crt"

ADMIN_PASSWORD = "admin" 
# ==========================================

app = Flask(__name__)
app.secret_key = "super_secure_academic_key"

def init_db():
    conn = sqlite3.connect('smart_lock.db')
    c = conn.cursor()
    c.execute('''CREATE TABLE IF NOT EXISTS audit_log
                 (id INTEGER PRIMARY KEY AUTOINCREMENT, timestamp TEXT, topic TEXT, message TEXT)''')
    conn.commit()
    conn.close()

def log_event(topic, message):
    conn = sqlite3.connect('smart_lock.db', timeout=10)
    c = conn.cursor()
    c.execute("INSERT INTO audit_log (timestamp, topic, message) VALUES (?, ?, ?)",
              (datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), topic, message))
    conn.commit()
    conn.close()

# --- 2. MQTT BACKGROUND LISTENER ---
def on_connect(client, userdata, flags, rc):
    print(f"[MQTT] Connected to Mosquitto with result code {rc}")
    client.subscribe("smart_lock/state")
    client.subscribe("smart_lock/status")

def on_message(client, userdata, msg):
    payload = msg.payload.decode('utf-8')
    print(f"[MQTT] LOGGED: {msg.topic} -> {payload}")
    log_event(msg.topic, payload)

mqtt_client = mqtt.Client()
mqtt_client.tls_set(ca_certs=CA_CERTS)
mqtt_client.tls_insecure_set(True) # Skip CN check, just like --insecure
mqtt_client.username_pw_set("python_backend", "backend123")
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message

def on_disconnect(client, userdata, rc):
    print(f"[MQTT] Disconnected from broker (rc={rc}). Will keep retrying...")

mqtt_client.on_disconnect = on_disconnect
mqtt_client.reconnect_delay_set(min_delay=1, max_delay=30)

def start_mqtt():
    # connect_async + loop_start keeps retrying in the background, even if the
    # broker is down when the backend starts (connect() + loop_forever would give up).
    mqtt_client.connect_async(MQTT_BROKER, MQTT_PORT, 60)
    mqtt_client.loop_start()

def publish_command(payload):
    """Publish to the lock; returns (ok, error_message)."""
    if not mqtt_client.is_connected():
        return False, f"MQTT broker {MQTT_BROKER}:{MQTT_PORT} is not connected. Is Mosquitto running?"
    info = mqtt_client.publish("smart_lock/command", payload, qos=1)
    if info.rc != mqtt.MQTT_ERR_SUCCESS:
        return False, f"Publish failed: {mqtt.error_string(info.rc)}"
    return True, None

# --- 3. WEB USER INTERFACE (HTML) ---
LOGIN_HTML = """
<!DOCTYPE html>
<html>
<head><title>Admin Login</title><link href="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css" rel="stylesheet"></head>
<body class="bg-dark text-white d-flex align-items-center justify-content-center" style="height: 100vh;">
    <div class="card bg-secondary p-5">
        <h2 class="mb-4">Smart Lock Admin</h2>
        <form method="POST" action="/login">
            <input type="password" name="password" class="form-control mb-3" placeholder="Enter Admin Token" required>
            <button type="submit" class="btn btn-primary w-100">Login</button>
        </form>
    </div>
</body>
</html>
"""

DASHBOARD_HTML = """
<!DOCTYPE html>
<html>
<head>
    <title>Secure Dashboard</title>
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css" rel="stylesheet">
</head>
<body class="bg-dark text-white p-4">
    <div class="container">
        <div class="d-flex justify-content-between align-items-center mb-4">
            <h1>🛡️ Secure Smart Lock Dashboard</h1>
            <a href="/logout" class="btn btn-outline-danger">Logout</a>
        </div>
        
        <div class="row mb-4">
            <div class="col-md-4"><button onclick="sendCommand('UNLOCK')" class="btn btn-success w-100 p-4 fs-4">🔓 Remote Unlock</button></div>
            <div class="col-md-4"><button onclick="sendCommand('LOCK')" class="btn btn-warning w-100 p-4 fs-4">🔒 Force Lock</button></div>
            <div class="col-md-4"><button onclick="sendCommand('RESET')" class="btn btn-danger w-100 p-4 fs-4">🔄 Reset Alarm</button></div>
        </div>

        
        <div class="card bg-secondary mb-4 p-3">
            <h5 class="mb-3">Change Door PIN</h5>
            <div class="d-flex">
                <input type="text" id="newPin" class="form-control me-2" placeholder="Enter new PIN (e.g. 1234)">
                <button onclick="changePin()" class="btn btn-info whitespace-nowrap">Update PIN</button>
            </div>
        </div>

        <div class="card bg-secondary">
            <div class="card-header d-flex justify-content-between">
                <h4 class="mb-0">Audit Log (Database)</h4>
                <button onclick="fetchLogs()" class="btn btn-sm btn-light">Refresh Logs</button>
            </div>
            <div class="card-body p-0" style="height: 400px; overflow-y: scroll;">
                <table class="table table-dark table-striped mb-0">
                    <thead><tr><th>Time</th><th>Topic</th><th>Message/Event</th></tr></thead>
                    <tbody id="logTable"></tbody>
                </table>
            </div>
        </div>
    </div>

    <script>
        
        function changePin() {
            let pin = document.getElementById('newPin').value;
            if (pin.length < 1) return alert("Enter a PIN!");
            if (confirm("Are you sure you want to change the door PIN to " + pin + "?")) {
                fetch('/api/change_pin', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({pin: pin})
                }).then(response => response.json())
                  .then(data => { alert(data.status); document.getElementById('newPin').value = ''; });
            }
        }

        function sendCommand(cmd) {
            if (confirm("SECURITY WARNING:\\nAre you sure you want to send the " + cmd + " command to the lock?")) {
                fetch('/api/command', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({command: cmd})
                }).then(response => response.json())
                  .then(data => alert(data.status));
            }
        }

        function fetchLogs() {
            fetch('/api/logs')
                .then(response => response.json())
                .then(logs => {
                    const tbody = document.getElementById('logTable');
                    tbody.innerHTML = '';
                    logs.forEach(log => {
                        let msg = log[3];
                        let colorClass = "";
                        
                        // Check keywords to apply Bootstrap color classes
                        if (msg.includes("ALARM") || msg.includes("DENIED") || msg.includes("OFFLINE") || msg.includes("LOCKED_OUT")) {
                            colorClass = "text-danger"; // Red
                        } else if (msg.includes("UNLOCK") || msg.includes("ONLINE")) {
                            colorClass = "text-success"; // Green
                        } else if (msg.includes("ENTERING_PIN")) {
                            colorClass = "text-warning"; // Yellow
                        }

                        tbody.innerHTML += `<tr><td>${log[1]}</td><td>${log[2]}</td><td class="${colorClass}"><strong>${msg}</strong></td></tr>`;
                    });
                });
        }
        
        // Auto-refresh logs every 2 seconds
        setInterval(fetchLogs, 2000);
        fetchLogs();
    </script>
</body>
</html>
"""

# --- 4. FLASK ROUTES (API) ---
@app.route('/')
def index():
    if not session.get('logged_in'):
        return render_template_string(LOGIN_HTML)
    return render_template_string(DASHBOARD_HTML)

@app.route('/login', methods=['POST'])
def login():
    if request.form['password'] == ADMIN_PASSWORD:
        session['logged_in'] = True
    return redirect('/')

@app.route('/logout')
def logout():
    session.pop('logged_in', None)
    return redirect('/')

@app.route('/api/logs')
def get_logs():
    if not session.get('logged_in'): return jsonify([])
    conn = sqlite3.connect('smart_lock.db')
    c = conn.cursor()
    c.execute("SELECT * FROM audit_log ORDER BY id DESC LIMIT 50")
    logs = c.fetchall()
    conn.close()
    return jsonify(logs)


import hashlib
@app.route('/api/change_pin', methods=['POST'])
def change_pin():
    if not session.get('logged_in'): return jsonify({"status": "Unauthorized!"}), 401
    pin = request.json.get('pin')
    # Hash the pin using SHA-256 just like the ESP32 does
    new_hash = hashlib.sha256(pin.encode('utf-8')).hexdigest()
    # Send the hash to the ESP32 (NEVER send the plaintext PIN!)
    ok, err = publish_command(f"UPDATE_HASH:{new_hash}")
    if not ok:
        return jsonify({"status": f"FAILED: {err}"}), 503
    return jsonify({"status": "Success! The new password hash was transmitted securely over TLS."})


@app.route('/api/command', methods=['POST'])
def send_command():
    if not session.get('logged_in'): return jsonify({"status": "Unauthorized!"}), 401
    cmd = request.json.get('command')
    if cmd not in ("UNLOCK", "LOCK", "RESET"):
        return jsonify({"status": f"Unknown command '{cmd}'"}), 400
    # Publish the command to Mosquitto over TLS!
    ok, err = publish_command(cmd)
    if not ok:
        return jsonify({"status": f"FAILED: {err}"}), 503
    return jsonify({"status": f"Command '{cmd}' successfully transmitted over TLS."})

if __name__ == '__main__':
    init_db()
    # Start MQTT client in the background (loop_start spawns its own thread and auto-reconnects)
    start_mqtt()
    # Start Web Server
    app.run(host='0.0.0.0', port=5000, debug=False)
