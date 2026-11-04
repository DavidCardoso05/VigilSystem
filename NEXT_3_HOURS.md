# VigilSystem: next 3 hours (cloud + app)

**Your part:** EMQX broker, local Mosquitto test broker, the MQTT message contract, the Pi's communication module (`NetworkMQTTThread` and the classes under it) and the Flutter app.

**Goal for today:** by the end, the cloud works for real. EMQX and a local Mosquitto both accept the two users and enforce the ACL. The message contract is written down. The Pi's MQTT client connects over TLS with the Last Will. A first Flutter app connects and shows the retained status.

**Sources:** the presentation comes first (`docs/design/G8_VigilSystem_D_Presentation.pdf`). The broker-selection Word file is used only where the slides say nothing. The table at the end lists every place where the Word file or your notes disagree with the slides.

---

## Timeline

| Time | Block | Done when |
|------|-------|-----------|
| 0:00 – 0:20 | 1. Install the toolchain | Packages installed, Flutter downloading in the background |
| 0:20 – 0:50 | 2. EMQX Serverless deployment | 2 users, 13 ACL rules, CA saved, ACL test passes |
| 0:50 – 1:20 | 3. Local Mosquitto test broker | TLS on 8883 with the same users and ACL as EMQX |
| 1:20 – 1:40 | 4. Message contract | `docs/protocol.md` + sample JSON files |
| 1:40 – 2:30 | 5. Pi: first MQTT client | `pi/` builds, connects with LWT, "online" seen in `mosquitto_sub` |
| 2:30 – 2:55 | 6. Flutter app skeleton | App connects and prints the retained `status` |
| 2:55 – 3:00 | 7. Wrap up | `git commit`, next steps noted |

Time is tight. If a block runs over, cut **block 6** first (it can be the first job next session), never blocks 2–4: everything else depends on them.

---

## 1. Install the toolchain (0:00 – 0:20)

Missing on this PC right now: CMake, Mosquitto, SQLite, Paho, nlohmann-json and Flutter.

```bash
sudo apt update && sudo apt install -y cmake ninja-build pkg-config mosquitto mosquitto-clients sqlite3 libsqlite3-dev libssl-dev libpaho-mqtt-dev libpaho-mqttpp-dev nlohmann-json3-dev clang libgtk-3-dev
```

```bash
sudo systemctl disable --now mosquitto
```
(Stops the system Mosquitto so it does not take port 1883. You'll run your own with TLS in block 3.)

```bash
sudo snap install flutter --classic
```
Start this one early: it is a large download. Then run `flutter doctor`.

- **Tip:** you don't need Android Studio today. Develop the app on the **Linux desktop target** first (`flutter run -d linux`), since `mqtt5_client` runs there too. Install the Android SDK later for the real phone and the foreground service.
- If `libpaho-mqttpp-dev` is missing or too old (you need ≥ 1.3 for MQTT 5), build `paho.mqtt.c` and then `paho.mqtt.cpp` from source with `-DPAHO_WITH_SSL=ON`.

## 2. EMQX Serverless deployment (0:20 – 0:50)

1. Create the EMQX Cloud account and a **Serverless** deployment. **Set the spend limit to 0** so you can never be charged.
2. Download the **CA certificate** from the deployment overview and save it as `secrets/emqx-ca.pem`. Note the host name; the port is 8883.
3. **Authentication** → add two users: `vigil-device` and `vigil-app`. Use long random passwords (`openssl rand -base64 24`). Save them in `secrets/vigil-device.pass` and `secrets/vigil-app.pass`.
4. **Authorization** → add the rules below, then the **deny-all rule last**. Serverless allows anything not matched, so without deny-all the ACL does nothing.

   | User | Topic | Action |
   |------|-------|--------|
   | vigil-device | `vigil/home1/status`, `alert`, `event`, `response`, `connection` | Publish, Allow |
   | vigil-device | `vigil/home1/cmd` | Subscribe, Allow |
   | vigil-app | `vigil/home1/cmd` | Publish, Allow |
   | vigil-app | `vigil/home1/status`, `alert`, `event`, `response`, `connection` | Subscribe, Allow |
   | All users | `#` | Publish + Subscribe, **Deny** |

5. Write the same table into `cloud/emqx/acl-rules.md`, so it can be copied to Mosquitto or a new deployment.
6. **Test it** (everything is MQTT 5, TLS, port 8883):

```bash
mosquitto_sub -V mqttv5 -h <EMQX_HOST> -p 8883 --cafile secrets/emqx-ca.pem -u vigil-app -P "$(cat secrets/vigil-app.pass)" -t 'vigil/home1/#' -v -d
```

```bash
mosquitto_pub -V mqttv5 -h <EMQX_HOST> -p 8883 --cafile secrets/emqx-ca.pem -u vigil-device -P "$(cat secrets/vigil-device.pass)" -t vigil/home1/status -r -q 1 -m '{"armed_state":"DISARMED","fire_state":"NORMAL","power_source":"GRID","battery_pct":100,"timestamp":"2026-11-03T15:00:00Z"}'
```

   - ✅ The subscriber receives the status message.
   - ✅ **ACL test:** publish on `vigil/home1/alert` with the **app** user. It must be rejected (`-d` shows a PUBACK reason code ≥ 0x80). This is the "ACL Enforcement" test case.
   - ⏳ **Open point from the Word file:** connect the app user with `-D connect session-expiry-interval 604800 -c -i vigil-app-test0001` and check whether the broker caps the session expiry. If `-d` doesn't show the CONNACK properties, leave this check for the Flutter or Paho client.

## 3. Local Mosquitto test broker (0:50 – 1:20)

You need this for every "broker lost" test, because EMQX can't be switched off on demand. Put everything in `cloud/mosquitto/`:

1. `make-certs.sh`: creates a test CA and a server certificate with `openssl`. The certificate's CN/SAN must be your laptop's IP or host name, because the Pi checks the host name. Output goes to `cloud/mosquitto/certs/` (ignored by git).
2. `mosquitto.conf`: `listener 8883`, `cafile`, `certfile`, `keyfile`, `allow_anonymous false`, `password_file`, `acl_file`, `log_type all`.
3. `acl`: the same rules as EMQX (in Mosquitto, anything not allowed is denied by default).
4. Create the password file: `mosquitto_passwd -c cloud/mosquitto/passwd vigil-device`, then `mosquitto_passwd cloud/mosquitto/passwd vigil-app` (no `-c` the second time).
5. Start it with `mosquitto -c cloud/mosquitto/mosquitto.conf -v`, then repeat the step 2 tests against `localhost` using the test CA.

## 4. Message contract (1:20 – 1:40)

Write `docs/protocol.md` from **slides 20–31**. The Pi and the app both code against this one file, so you and Ana never disagree on a field name.

- **Topics** (all QoS 1): `vigil/home1/` + `status` (retained), `connection` (retained), `alert`, `event`, `response`, `cmd` (never retained; Message Expiry 30 s).
- **cmd** (app → Pi): `{"request_id", "type": "arm" | "disarm" | "change_pin" | "get_events", "pin"?, "new_pin"?, "since"?}`
- **response** (Pi → app): `{"request_id", "result", "detail"?}`. Possible results:
  - all commands: `OK`
  - arm: `REJECTED_FIRE_ACTIVE`, `ALREADY_ARMED`
  - disarm: `REJECTED_FIRE_ACTIVE`, `ALREADY_DISARMED`, `LOCKED_OUT`, `INVALID_PIN`
  - change_pin: `LOCKED_OUT`, `INVALID_PIN`, `INVALID_NEW_PIN`, `SAVE_FAILED`
  - get_events: `OK` + `events[]` (the 20 newest with `event_id > since`, oldest first, in one message)
- **status**: `armed_state` (DISARMED / ARMED / INTRUSION_ALARM), `fire_state` (NORMAL / FIRE_ALARM / SILENCED), `power_source` (GRID / BATTERY), `battery_pct` (0–100, 10 % steps), `timestamp`. Published at startup, after every reconnect, and on every change. Never periodic.
- **connection**: `{"state":"online","timestamp"}`; the Last Will `{"state":"offline"}` (no timestamp); clean shutdown `{"state":"offline","reason":"LOW_BATTERY_SHUTDOWN","timestamp"}`.
- **alert** (stored in the outbox, resent after reconnect): `INTRUSION_ALARM`, `FIRE_ALARM` (DETECTED / REACTIVATED), `ALARM_SILENCED` (FIRE), `HAZARD_CLEARED`, `POWER_LOST`, `POWER_RESTORED`, `LOW_BATTERY`, `SHUTDOWN`, `FAULT` (IR1 / IR2 / MQ2 / SPEAKER / PWM_DRIVER / DATABASE), `PIN_LOCKOUT` (LOCAL / REMOTE).
- **event** (**not** stored in the outbox; the app recovers missed ones with `get_events`): `ARMED`, `DISARMED`, `WRONG_PIN` (LOCAL / REMOTE), `PIN_CHANGED`, `ALARM_SILENCED` (INTRUSION).
- Every alert and event has the fields `event_id`, `event_type`, `timestamp`, `details`.

Also save one JSON file per message in `cloud/samples/`, for testing with `mosquitto_pub -f`.

## 5. Pi: first MQTT client (1:40 – 2:30)

Only the connection layer today. The command logic, outbox and thread come next session.

1. `pi/CMakeLists.txt`: C++17, targets `vigil` + `vigil_comm_test`. Find `PahoMqttCpp`, `nlohmann_json`, `SQLite3`, `Threads`.
2. `include/Topics.hpp`: topic constants.
3. `include/BrokerConfig.hpp`: an INI parser for `host`, `port` (8883), `ca_file`, `client_id`, `username`, `secret_file`, `keepalive_s` (30). The password is read from `secret_file`, never from the config itself.
4. `config/broker.conf.example`: committed. Your real `broker.conf` is ignored by git.
5. `include/MqttClient.hpp` + `src/MqttClient.cpp`:
   - MQTT 5 over TLS, with server certificate **and host name** checked.
   - `client_id = vigil-device-home1`, `clean_start = true`, `session_expiry = 0`, `keepalive = 30 s`.
   - Last Will: `{"state":"offline"}` on `connection`, retained, QoS 1.
   - After CONNACK: subscribe to `cmd`, publish `"online"` (retained), then the status.
   - `publish()` returns `true` only after a PUBACK with reason code < 0x80.
6. **Test:** run `vigil_comm_test` against the local Mosquitto. `mosquitto_sub` must show `online`. Kill the process with `kill -9` and `offline` must appear after about 45 s (1.5 × keep-alive).

Next session: `Protocol.hpp` (strict validation), `PendingNotificationStore` (SQLite outbox), `NetworkMQTTThread` (reconnect backoff 1 s → 60 s, auth errors every 5 min, resend at most 5 msg/s, drop `cmd` messages without Message Expiry), and `FakeSystem` (PIN 1234).

## 6. Flutter app skeleton (2:30 – 2:55)

1. Create the project in the existing folder: `flutter create --org pt.uminho.vigil --platforms=android,linux app`
2. Add the packages: `flutter pub add mqtt5_client shared_preferences`
3. Copy `assets/config/broker.example.json` (committed) to `assets/config/broker.json`, which is ignored by git. It holds the host, port, user, password and CA path.
4. The client ID is `vigil-app-XXXXXXXX` (8 random hex characters). Generate it once at first launch and keep it in `shared_preferences`.
5. Connect with MQTT 5 over TLS, `clean_start = false`, keep-alive 60 s, the longest session expiry, and subscribe to the 5 topics at QoS 1. For now, just print what arrives. You should see the retained `status` and `connection` right away.
6. Theme (Material 3): primary `#1A4D8F`; states: armed `#2E7D32`, disarmed `#5F6368`, warning `#B26A00`, alarm `#C62828`.

Next session: the status card from slide 46, the Arm / Disarm / Change PIN dialogs, `request_id` matching with a 32 s timeout, `get_events` on every "online", deduplication by `event_id`, the last 20 alerts, and the foreground service with notifications.

## 7. Wrap up (2:55 – 3:00)

```bash
git add -A && git status
```
Check that **no password, `.pem` file or `broker.conf`** shows up in the list, then commit.

---

## Where the sources disagree (follow the presentation)

| Topic | Presentation (follow this) | Word file / your notes |
|-------|----------------------------|------------------------|
| `get_events` reply | **20 newest** events with `event_id > since`, oldest first, **in one message** (slides 20, 30) | Word: batches of ~50 · notes: "batches" |
| Reconnect backoff | **1 s → 60 s**; auth errors (0x86/0x87) every 5 min; never stop retrying (slide 27, Network FSM) | Notes: up to 30 s |
| New PIN length | **4–6 digits** (slide 20) | Notes: 4–8 |
| Request ID field | **`request_id`** | Notes: `req_id` |
| Communication thread | **`NetworkMQTTThread`**, lowest priority group (with StorageThread and SupervisorThread); the outbox and EVENT_LOG belong to **StorageThread**, which the network thread asks for pending rows (slides 34, 36) | Word / notes: `CommunicationThread`, SCHED_FIFO 40 |
| What goes in the outbox | **Alerts only.** Log events are not buffered; they are recovered with `get_events` (slides 25–26) | Word: "pending notifications" in general |
| Broker-loss detection | ≤ 60 s (2 × 30 s keep-alive); Wi-Fi loss ≤ 2 s; Last Will after 45 s | Same in the Word file ✓ |
| Red LED pin | **GPIO13 (PWM1)**. GPIO26 is the RGB LED's **green** channel (slide 13) | Notes: red LED on GPIO26 ⚠️ |
| Driver design | **Platform driver on the kernel PWM framework** (pwm-bcm2835), pins in a device tree overlay (slide 44) | Notes: char device with ioremap + hrtimer software PWM |

The last two rows are Ana's area, not yours, but the notes would wire the red LED onto the RGB LED's pin. Confirm both with her before anyone writes the driver.

**Thread priority:** your notes give the communication thread SCHED_FIFO 40. That's fine only if the alarm, MQ-2 and output threads run *above* 40, since the slides put networking in the lowest group. Agree on the numbers with Ana.
