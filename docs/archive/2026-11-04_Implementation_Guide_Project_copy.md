> **ARCHIVED — superseded on 2026-11-09 by `VIGILSYSTEM_MASTER_CONTEXT.md` v1.0. Kept for history only. Do not implement from this document.**
>
> Original location: Claude Project: `claude/VigilSystem – Implementation Guide.md` · Document date: 4 Nov 2026 · Superseded by: `docs/IMPLEMENTATION_GUIDE.md` (repository copy with validity box) and Master Context.
> The text below this box is an unchanged copy of the original.

---

# VigilSystem — Implementation Guide

Group 8 · David Cardoso (PG63873) · Ana Sofia Cunha (A107017) · MEEIC, Universidade do Minho (ESRG) · SEA + STR

Compiled on 4 Nov 2026 from every design document of the project and from the full design deck (`docs/design/G8_VigilSystem_D_Presentation.pdf`, 61 slides). It is the single reference for implementing the system. It says **what** to build, **how** to build it, and **why** each decision was taken, so that an AI assistant (Claude) or a team member can implement any part without re-reading the slides.

---

## 0. How to use this document

### 0.1 For Claude (and any AI assistant) working in this repository

1. **Read §1–§6 before writing code.** They hold the architecture and the behaviour rules. The other sections are references: open the one for the component you are working on.
2. **The design is decided.** Do not redesign threads, drivers, topics, messages, the database or the state machines. If something seems wrong, say so and point to the rule; do not silently change it. The list of options that were declined on purpose is in §2.4.
3. **Keep the names.** Class, thread, state, event, topic, table and field names come from the UML diagrams and the message specification, and they appear in the report. Use them exactly (e.g. `IRSensorsThread`, not `PIRSensorsThread`; `NetworkMQTTThread`, not `CommunicationThread`).
4. **When the design is silent**, use the default given in §18 (“Open issues and default decisions”) and mention in your answer that you used a default. If §18 has nothing, ask the user before inventing behaviour.
5. **Rules have IDs**: behaviour rules `BR-…`, FSM transitions `O-`/`F-`/`P-`/`N-`, real-time rules `RT-…`, decisions `D-…`, default decisions `G…` and issue numbers (§18). Quote them in code comments and tests where it helps, e.g. `// BR-PIN-4: no PIN is checked during a lockout`.
6. **Never commit secrets** (passwords, keys, certificates, `broker.conf`, `wpa_supplicant.conf`). `.gitignore` already covers them; check `git status` before every commit.
7. **Ownership.** David: cloud, MQTT contract, `NetworkMQTTThread` and its classes, Flutter app. Ana: hardware, the `vigil_pwm` driver and the hardware threads. Tell the user when a change crosses into the other person's area.

### 0.2 The 25 rules that matter most

| # | Rule |
|---|------|
| 1 | One C++17 process `vigil` on the Pi, **9 pthreads**, one **Event Queue** into `AlarmControllerThread`, one `CommandQueue<T>` per consumer (Output, Storage, Network). |
| 2 | `AlarmControllerThread` owns **all** system logic: 4 FSMs (Operational, Fire, Network, Power), 4 timers (`timerfd`), the wrong-PIN counter and the cached PIN hash. It never blocks on I/O. |
| 3 | Exactly **one custom kernel driver**: `/dev/vigil_pwm` (PWM0/GPIO18 = speaker, PWM1/GPIO13 = red LED), a platform driver on the kernel PWM framework, with tone/blink patterns stepped by hrtimers. Everything else uses **libgpiod** (keypad, button, IR, MQ-2, RGB LED) and **i2c-dev** (INA219 at `0x42`). |
| 4 | Scheduling: `SCHED_FIFO` 80 = MQ2SensorThread, AlarmControllerThread, OutputThread (the fire path); 70 = IRSensorsThread; 50 = LocalInputThread, PowerMonitorThread; `SCHED_OTHER` = StorageThread, NetworkMQTTThread, SupervisorThread. |
| 5 | The **local alarm never depends on the network**. |
| 6 | Fire is monitored 24/7 and **stays on until a valid keypad PIN** silences it. Remote disarm is **rejected** during a fire alarm. |
| 7 | A valid keypad PIN during a fire alarm: tone off, 10 min silence period, system **DISARMED** (never re-arms). One valid PIN silences fire and intrusion together. |
| 8 | Alarm priority: **fire > intrusion > fire silenced > none** (status card). On the single speaker: fire tone > intrusion tone > entry warning > beeps; beeps are dropped while an alarm sounds. |
| 9 | Arm button → 60 s exit delay. Remote Arm → armed at once (also cancels a running exit delay). Both rejected while a fire alarm is active, allowed during the silence period. |
| 10 | IR (door/window) ignored when disarmed and during the exit delay; when armed → 30 s entry delay with warning tone → intrusion alarm + alert. |
| 11 | **One** wrong-PIN counter shared by keypad and app. 5 consecutive wrong PINs → 5 min lockout + `PIN_LOCKOUT` alert. No PIN is checked during a lockout, not even during a fire alarm. |
| 12 | Remote Disarm checks, in order: fire alarm → `REJECTED_FIRE_ACTIVE`; disarmed → `ALREADY_DISARMED`; lockout → `LOCKED_OUT`; PIN → `OK` / `INVALID_PIN`. The first three never test the PIN. |
| 13 | The PIN is validated **only on the Pi**, stored only as `pbkdf2_sha256$<iterations>$<salt>$<hash>`, never logged, never published. |
| 14 | Cloud = **EMQX Serverless broker only** (MQTT 5 over TLS, port 8883). No backend, no cloud DB, no push service. The Pi is the source of truth. |
| 15 | Topics `vigil/home1/` + `status` and `connection` (retained), `alert`, `event`, `response`, `cmd`. All QoS 1. `cmd` is never retained and carries a 30 s Message Expiry. |
| 16 | **Outbox:** every *alert* gets a `PENDING_NOTIFICATION` row **before** it is published; it becomes `SENT` only after a PUBACK with reason `0x00` or `0x10`; pending rows are resent oldest first, at most 5/s. *Log events* are not buffered (recovered with `get_events`). |
| 17 | After every CONNACK the Pi publishes, in order: `"online"` (retained) → full status (retained) → pending alerts. |
| 18 | Pi connects with `client_id = vigil-device-home1`, Clean Start = 1, session expiry 0, keep-alive 30 s, Last Will `{"state":"offline"}` (retained, QoS 1). It **never stops** reconnecting: backoff 1 s → 60 s; auth errors (`0x86`/`0x87`) every 5 min. |
| 19 | `get_events` returns the **20 newest** events with `event_id > since`, oldest first, in **one** message. |
| 20 | Status is published at start-up, after every reconnection and on every change — **never periodically**. Exit delay is reported as `DISARMED`, entry delay as `ARMED`; battery only in 10 % steps. |
| 21 | Power: INA219 polled every 1 s with 3-sample hysteresis; low battery at ≤ 30 % (≈ 6.7 V) → one alert per outage; ≤ 6.4 V → clean shutdown (alert, retained offline with reason, DISCONNECT `0x00`, sync SQLite, power off). |
| 22 | GPIO13 is the **red LED (PWM1)**. GPIO26 is the RGB **green** channel. Never wire or code the red LED on GPIO26. |
| 23 | Never leave GPIO18 (speaker) at a DC level: stopping a tone must leave the pin LOW; the speaker duty cycle is at most 50 %. |
| 24 | Target uses **libgpiod v1 (1.6.x)**, because the Bootlin stable toolchain has kernel headers 5.4 (libgpiod v2 needs ≥ 5.10). Hide it behind `GpioLines`. |
| 25 | `SupervisorThread` checks heartbeats from the 8 worker threads, restarts a hung thread and feeds `/dev/watchdog`; if it cannot recover it stops feeding and the Pi resets. |

---

## 1. Project summary

### 1.1 Problem and motivation

Commercial home alarms are expensive, depend on proprietary hardware and monthly subscriptions, and become vulnerable during power or internet outages. VigilSystem is a real-time embedded home-security and hazard-monitoring system on a Raspberry Pi 4 that:

1. Detects intrusion (two IR door/window sensors, only when armed) and smoke/gas (MQ-2, 24/7, also when disarmed).
2. Sounds distinct local alarms (different PWM tone patterns for intrusion, fire, entry/exit warning, key and error beeps) and shows status on a red LED and an RGB LED.
3. Is controlled locally (arm button + 3×4 keypad for the PIN) and remotely (Android app over MQTT 5/TLS through EMQX Serverless).
4. Keeps running through mains failures on a UPS HAT with two 18650 Li-ion cells, and shuts down cleanly when the battery is nearly empty.
5. Tolerates network failures: alerts are kept in a local SQLite outbox and delivered in order when the broker is reachable again.

### 1.2 Requirements (analysis phase, still valid)

**Functional:** dual detection; distinct alarms; flexible control (local + remote); configurable entry/exit delays (remote arm has no exit delay); fire-alarm silence period; instant alerts (intrusion, hazard, grid failure/restoration); power resilience; visual status (red LED = disarmed/armed/alarm, RGB = power source + broker connectivity); connectivity monitoring of the **broker** (not only Wi-Fi); PIN change only from the app; pending-notification handling in chronological order; event logging with timestamps.

**Non-functional:** deterministic low latency for sensor events and local commands; no reset or interruption when switching grid ↔ battery; authenticated and encrypted remote communication, PIN validated only on the device and never stored in plain text.

**Technical constraints (course):** Raspberry Pi 4 Model B; C++ object-oriented (SOLID); custom Linux image built with **Buildroot**; at least one **Linux device driver** of our own (character device, `.ko`); **pthreads**; UML-based analysis and design.

**Non-technical:** two-person group, limited budget, deadline at the end of the semester.

### 1.3 Schedule (Gantt, slide 60)

| Phase | Period | Content |
|-------|--------|---------|
| 1 Project outline | mid/late Sep 2026 | Problem, requirements, constraints, overview |
| 2 Analysis | late Sep – early Oct | Market study, system and hardware architecture |
| 3 Design | Oct | Analysis review, HW/SW specification, interfaces, error handling, design verification |
| **4 Implementation** | **≈ 1 Nov – 27 Dec 2026** | Design review (6 days), then hardware implementation, software implementation and testing in parallel (51 days each) |
| 5 Verification | ≈ 1 – 15 Jan 2027 | Verification |

---

## 2. Sources, precedence and superseded information

### 2.1 Source documents

| Source | Where | What it covers |
|--------|-------|----------------|
| Design deck (61 slides, exported 3 Nov 2026) | `docs/design/G8_VigilSystem_D_Presentation.pdf` (also in the Claude project) | Hardware, cloud, messages, MQTT sequences, package/class diagrams, tasks, FSMs, flowcharts, driver, parameters, GUI, tests, error handling, Gantt |
| Design Decisions Record (2 Nov 2026) | Claude project | Every design-review decision and its reason; **wins over older documents** |
| Class Diagram (Design Phase) | Claude project | 18 classes, 5 enumerations, PlantUML source |
| Package Diagram (Design Phase) | Claude project | Packages, driver layer, PlantUML source |
| Design-Phase Diagrams Work Log | Claude project | FSM/flowchart rationale and the 28 open issues (§18) |
| Data Storage, ERD and Cloud Architecture Updates | Claude project | Slides 17–19, ERD constraints |
| Hardware Context | Claude project | Final hardware decisions, circuits, GPIO table, power budget |
| MQTT Broker Selection (EMQX + Mosquitto) | Claude project, `docs/design/*.docx` | Broker choice, ACL, delivery rules, Mosquitto test broker, Buildroot packages |
| Mobile App GUI Design | Claude project | App GUI, states, dialogs, notifications, Flutter plan |
| Analysis deck (`G8_VigilSystem_A_Presentation.pptx`) | Claude project | Requirements, system events table, analysis diagrams (superseded by the design deck) |
| `NEXT_3_HOURS.md`, `buildroot/README.md`, `cloud/README.md` | Repository | Work plan and the state of the cloud and Buildroot setup |

### 2.2 Precedence when sources disagree

1. **This guide** (it consolidates everything below and resolves the known gaps, see §18).
2. Design Decisions Record (2 Nov 2026).
3. Design deck PDF (3 Nov 2026). Known stale labels in the deck: slides 35–36 still say `PIRSensorsThread`; slide 36 has no priority numbers; some diagram fixes are pending (§18.3).
4. Class / Package diagram documents, Diagrams Work Log, Data Storage/ERD document.
5. Hardware Context.
6. MQTT Broker Selection and Mobile App GUI documents (older; partly superseded, see §2.3).
7. Claude project instructions (Portuguese context): sections on the four custom drivers, the old class names and the old pin table are **obsolete**.

> Slide numbers in this guide refer to the PDF in `docs/design/`. The Decisions Record sometimes uses older numbers (e.g. it calls the driver slide 47 and the GUI slides 44–46; in the PDF they are 44 and 46–48).

### 2.3 Superseded information (old → current)

| Topic | Old (do **not** use) | Current |
|-------|----------------------|---------|
| Architecture | `VigilSystem`/`VigilSystemController` god class, `SensorMonitorThread`, `DelayTimer`, `AlarmActuator`, `LocalInterfaceThread` | Event-queue architecture with 9 threads (§5) |
| Drivers | Four custom drivers: `KeypadDriver`, `GPIODriver`, `LedDriver`, `BuzzerDriver`; `UpsHatDriver` class | One custom driver `/dev/vigil_pwm`; libgpiod; i2c-dev wrapped by `Ina219` |
| Intrusion sensor | “PIR” motion sensor (HC-SR501/AM312), `PIRSensorsThread` | IR reflective door/window sensors ×2, `IRSensorsThread` (the hardware slides keep the label “PIR”/“HC-SR501”) |
| Sound | Passive buzzer | FoneStar EP-36 8 Ω speaker + NPN driver (slides call it “Buzzer Module”) |
| Red LED pin | GPIO26 | **GPIO13 (PWM1)**; GPIO26 = RGB green |
| RGB pins | GPIO4 / 13 / 19 | R = GPIO4, G = GPIO26, B = GPIO19 |
| Communication thread | `CommunicationThread`, `SCHED_FIFO` 40 | `NetworkMQTTThread`, `SCHED_OTHER` |
| Outbox | Pending row only while offline | Row for **every alert** before publishing |
| `get_events` | Batches of ~50, paging | 20 newest with id > since, one message |
| Alert list | “~20” | Exactly at most 20 |
| New PIN length | 4–8 digits | **4–6 digits** |
| Request field | `req_id` | `request_id` |
| Reconnect backoff | up to 30 s | 1 s → 60 s; auth errors every 5 min; never stop |
| Fire silence | Returns to the previous armed state (GUI doc §9) | Always leaves the system **DISARMED** |
| Device offline in app | “Offline – last seen HH:MM” | “Offline” with no time (the Last Will cannot carry a time) |
| Cloud | Backend/REST API, cloud DB, Firebase push | Broker only |
| PIN storage | Plain hash | Salted PBKDF2-SHA256 string |
| RTC | DS3231 | No RTC: NTP (chrony) + saved-date script |

### 2.4 Declined on purpose (do not reopen without a new reason)

Cloud backend / REST API / cloud database / Firebase push · Last Will carrying a disconnection time · a “skipped events” count in `get_events` · changing the app command timeout vs Message Expiry, PUBACK `0x10` offline detection or the timeout wording · extra GUI slides · ONT box and “recommended small UPS” on slide 19 · per-phone broker credentials and a PIN for Remote Arm (described as hardening, not adopted) · the four custom drivers.

---

## 3. Repository layout and current state

### 3.1 State on 4 Nov 2026

```
VigilSystem/
├── README.md                 folder overview
├── NEXT_3_HOURS.md           work plan of 3 Nov (cloud + app)
├── .gitignore                secrets, build outputs, Flutter/Android, Buildroot output
├── app/                      EMPTY (.gitkeep) — Flutter app goes here
├── buildroot/
│   ├── README.md             Buildroot usage notes (tree in ~/buildroot/buildroot-2026.08)
│   ├── configs/vigil_fragment.config    VigilSystem additions (readable)
│   ├── configs/vigil_rpi4_defconfig     full saved defconfig
│   └── overlay/etc/          chrony.conf, wpa_supplicant.conf(.example), init.d/S40wifi, init.d/S99vigil,
│                             vigil/broker.conf.example
├── cloud/                    DONE: Mosquitto test broker (setup/run/certs scripts, conf, ACL),
│   │                         EMQX ACL rules + env template, test-broker.sh (13 checks)
│   └── samples/              EMPTY — JSON samples for mosquitto_pub (to do, §10.11)
├── docs/design/              G8_VigilSystem_D_Presentation.pdf, broker-selection .docx
├── driver/                   EMPTY — vigil_pwm kernel module (to do, §8)
├── pi/{config,include,src,tests}/   EMPTY — C++ application (to do, §7)
└── secrets/                  git-ignored: emqx-ca.pem, local-*.pass, local.env
```

Cloud status: the local Mosquitto broker is set up (`secrets/local.env` exists) and the EMQX CA is downloaded. `secrets/vigil-device.pass`, `secrets/vigil-app.pass` and `secrets/emqx.env` were not found, so EMQX steps A3–A5 of `cloud/README.md` are probably still to do. `docs/protocol.md`, the `pi/` client and the app skeleton planned in `NEXT_3_HOURS.md` have not been started.

### 3.2 Target layout

```
pi/                                   C++17, CMake
├── CMakeLists.txt                    targets: vigil_core (lib), vigil (exe), vigil_tests (host only)
├── config/vigil.conf.example         timing/hardware/paths config (§11)
├── include/vigil/
│   ├── core/      Event.hpp EventType.hpp EventQueue.hpp CommandQueue.hpp Commands.hpp States.hpp
│   │              PiMutex.hpp Config.hpp TimeUtil.hpp Log.hpp
│   ├── threads/   WorkerThread.hpp SupervisorThread.hpp LocalInputThread.hpp IRSensorsThread.hpp
│   │              MQ2SensorThread.hpp PowerMonitorThread.hpp AlarmControllerThread.hpp
│   │              OutputThread.hpp StorageThread.hpp NetworkMQTTThread.hpp
│   ├── logic/     AlarmLogic.hpp PinHasher.hpp OutputPolicy.hpp StatusMapper.hpp   (pure, unit-tested)
│   ├── hw/        IGpio.hpp GpioLines.hpp Ina219.hpp PwmDevice.hpp Watchdog.hpp sim/…  (real + fakes)
│   ├── storage/   Database.hpp Schema.hpp
│   └── net/       MqttClient.hpp Protocol.hpp Topics.hpp BrokerConfig.hpp LinkMonitor.hpp
├── src/           mirrors include/
├── tests/         host unit tests (logic, protocol, storage with a temp DB, queues)
└── tools/         vigil-pinhash (prints a PBKDF2 hash string for the default PIN)

driver/
├── Makefile                          Kbuild + out-of-tree build against the Buildroot kernel
├── vigil_pwm.c                       platform driver + char device
├── include/uapi/vigil_pwm.h          command struct, channels, patterns (shared with pi/)
├── dts/vigil-pwm-overlay.dts         device-tree overlay (claims GPIO18 + GPIO13)
└── test/vigil_pwm_test.c             user-space test tool

buildroot/external/                   BR2_EXTERNAL tree (§12.3): packages vigil + vigil-pwm, board/config.txt
app/                                  Flutter project (§13)
docs/protocol.md                      optional extract of §10 for the app side (must match §10 exactly)
cloud/samples/*.json                  one file per message type (§10.11)
```

---

## 4. Hardware reference

### 4.1 Components

| # | Component | Role | Notes |
|---|-----------|------|-------|
| 1 | Raspberry Pi 4 Model B | CPU | BCM2711, 4× Cortex-A72; GPIO 3.3 V, **not 5 V tolerant**, 8 mA default drive; no RTC; **no ARMv8 crypto extensions** (SHA-256 runs in software) |
| 2 | MicroSD 16/32 GB, Class 10/A1, high endurance | Image + SQLite | FAT32 boot + ext4 rootfs |
| 3 | UPS HAT (Botnroll 4106) | 5 V to the Pi through the header, up to 2.5 A | INA219 at I²C `0x42` (bus voltage, current, power). No “mains lost” signal |
| 4 | 2× 18650 Li-ion 2600 mAh (BAT03005), 2S | Backup | 6.0 V empty – 7.2/7.4 V nominal – 8.4 V full. Switch the HAT OFF before inserting |
| 5 | 8.4 V DC wall adapter | Mains + charging | |
| 6 | Keypad 3×4 (Ref. 434) | PIN entry | 4 rows + 3 columns, passive matrix |
| 7 | Push-button (Ref. 716 / 1188) | Local arming | SPST-NO to GND, active-low |
| 8 | IR reflective proximity sensor ×2 (SEN10019 on slide 12; “PIR”/“HC-SR501” on other slides) | Door/window opening | 5 V supply, output active-low (LOW = leaf in front = closed), < 2 ms, ≤ 15° |
| 9 | MQ-2 module (Ref. 939) | Smoke/gas 24/7 | 5 V, heater 120–150 mA; only DO used (LM393, **active-low**), 5 V → 3.3 V divider |
| 10 | FoneStar EP-36 speaker 8 Ω 0.3 W (“Buzzer Module”) + 2N2222A/BC337 | Tones | PWM0 on GPIO18 |
| 11 | Red LED 5 mm (Ref. 2629) + 220 Ω | Armed/alarm | PWM1 on GPIO13, ≈ 6 mA |
| 12 | RGB LED common anode (CEL03119 piranha) + 390/220/220 Ω | Power source + broker status | Anode to +5 V; GPIO **sinks** (LOW = on) |

### 4.2 GPIO allocation (authoritative)

| BCM | Pin | Function | Direction / bias | Active level | Owner (thread) |
|-----|-----|----------|------------------|--------------|----------------|
| 17 | 11 | Keypad row 1 (keys 1 2 3) | Input, pull-up | LOW = key in active column | LocalInputThread |
| 27 | 13 | Keypad row 2 (4 5 6) | Input, pull-up | LOW | LocalInputThread |
| 22 | 15 | Keypad row 3 (7 8 9) | Input, pull-up | LOW | LocalInputThread |
| 23 | 16 | Keypad row 4 (* 0 #) | Input, pull-up | LOW | LocalInputThread |
| 24 | 18 | Keypad column 1 (1 4 7 *) | Output, scan | driven LOW when scanned | LocalInputThread |
| 25 | 22 | Keypad column 2 (2 5 8 0) | Output, scan | LOW when scanned | LocalInputThread |
| 5 | 29 | Keypad column 3 (3 6 9 #) | Output, scan | LOW when scanned | LocalInputThread |
| 6 | 31 | Arm button | Input, pull-up | LOW = pressed | LocalInputThread |
| 16 | 36 | IR sensor 1 (door/window) | Input, pull-up (10 kΩ external on slide 12) | LOW = closed, **HIGH = open** | IRSensorsThread |
| 20 | 38 | IR sensor 2 (door/window) | Input, pull-up | LOW = closed, HIGH = open | IRSensorsThread |
| 21 | 40 | MQ-2 DO via 10 k/20 k divider | Input, **no pull** (bias disabled) | **LOW = gas/smoke** | MQ2SensorThread |
| 13 | 33 | Red LED (PWM1, ALT0) | PWM | HIGH = on | OutputThread via `/dev/vigil_pwm` |
| 18 | 12 | Speaker (PWM0, ALT5) | PWM | — | OutputThread via `/dev/vigil_pwm` |
| 4 | 7 | RGB red cathode | Output | **LOW = on** | OutputThread (libgpiod) |
| 26 | 37 | RGB green cathode | Output | **LOW = on** | OutputThread (libgpiod) |
| 19 | 35 | RGB blue cathode | Output | **LOW = on** | OutputThread (libgpiod) |
| 2 / 3 | 3 / 5 | I²C-1 SDA/SCL → UPS HAT INA219 `0x42` | I²C (ALT0) | — | PowerMonitorThread |

Power: 5 V on pins 2/4 from the HAT, GND on pin 6. GPIO0/1 reserved (HAT EEPROM), GPIO14/15 kept for the serial console, GPIO7–12 free.

Keypad map, indexed `[row][col]`: `{"123", "456", "789", "*0#"}`. Verify the keypad connector order with a continuity tester before wiring; the 7 pins are not always in row/column order.

### 4.3 Electrical facts that the software must respect

- **Default pulls at power-on:** BCM GPIO0–8 have pull-ups, GPIO9–27 pull-downs. So the keypad rows (17, 22, 23, 27) and IR inputs (16, 20) must get an explicit pull-up, and the MQ-2 input (21) must have its pull **disabled**: the internal ≈ 50 kΩ pull-down in parallel with the 20 kΩ divider resistor lowers the HIGH level to ≈ 2.1 V, too close to the input threshold (same reason the 10 k/15 k divider was rejected).
- **Set safe levels before Linux starts** with `config.txt` `gpio=` lines (§12.2): rows/button/IR pulled up, MQ-2 no pull, RGB channel outputs (GPIO4/26/19) driven **LOW** (BC337 switches off → LED off, decision D2), GPIO13/18 driven LOW. The application also sets the bias through libgpiod; the `config.txt` lines are the safety net if a bias flag is not supported.
- **Keypad columns:** request them **open-drain** (libgpiod emulates it): `1` = released (high-Z), `0` = drive LOW. Two keys pressed in the same row then cannot short a HIGH column to a LOW one.
- **RGB LED:** common anode on +5 V, GPIO sinks; request the lines with the active-low flag so logical 1 = on. With a GPIO at 3.3 V only ≈ 1.7 V is left across the red die; if it glows faintly, add a 1N4148 in series with the red channel. **Boot (with the BC337 low-side switches, D2):** GPIO19/26 boot pull-downs keep green/blue off; GPIO4's boot pull-up (≈ 50 kΩ) against the 1 kΩ/10 kΩ base network gives ≈ 0.5 V on the base, below V_BE, so red stays off; the `config.txt` line then drives all three LOW.
- **Speaker:** GPIO18 → 470 Ω → base of 2N2222A/BC337 (10 kΩ base pull-down); +5 V → speaker → 33 Ω/0.5 W → collector; 1N4148 flyback across the speaker. Peak ≈ 122 mA. **Never leave GPIO18 HIGH** (DC heats the coil and resistor): stopping a tone must leave the pin LOW, and the speaker channel never uses more than 50 % duty. Check the transistor pinout of the part bought (PN2222A E-B-C; P2N2222A/BC337 C-B-E).
- **MQ-2:** DO → 10 kΩ → GPIO21, 20 kΩ from GPIO21 to GND (HIGH ≈ 2.5 V with the module's on-board pull-up, ≈ 3.3 V without). It also reacts to alcohol vapours; it is not a certified smoke detector (educational prototype). Inhibit detection during warm-up and require N consecutive reads.
- **IR sensors:** mounted on the frame, pointing at the leaf. Closed → reflection → LOW. Open → HIGH → intrusion trigger. Verify the output type before wiring: open collector (use the pull-up) or 5 V push-pull (needs a divider like the MQ-2).
- **PWM block:** analogue audio also uses the PWM peripheral; do **not** enable `dtparam=audio=on`. Do not load the stock `pwm-2chan` overlay together with ours (its default maps PWM1 to GPIO19, the RGB blue channel).

### 4.4 Tone plan (tune by ear) and LED semantics

| Pattern | Channel | Definition (proposal from the Hardware Context) | Used for |
|---------|---------|--------------------------------------------------|----------|
| KEY_BEEP | speaker | 50 ms at 2 kHz, once | Each key, accepted arm button press |
| ERROR_BEEP | speaker | 2 × 150 ms at ≈ 700 Hz (100 ms gap), once | Wrong PIN, rejected local action, PIN during lockout |
| WARNING | speaker | 100 ms at 1 kHz every 1 s, repeat | Entry delay |
| INTRUSION | speaker | 800 Hz / 1200 Hz alternating, 250 ms each, repeat | Intrusion alarm |
| FIRE | speaker | 2 kHz / 3 kHz alternating, 150 ms each, repeat | Fire alarm |
| LED_ON | red LED | constant 100 % duty | Armed (also exit and entry delay) |
| LED_FLASH | red LED | 250 ms on / 250 ms off, repeat | Any active alarm (fire or intrusion) |
| LED_BLINK | red LED | 3 × (100 ms on / 100 ms off), once, then back to the base state | Wrong PIN |

Red LED: OFF = disarmed; steady ON = armed; flashing = alarm; short blink = wrong PIN.

RGB LED (pure red is reserved for the red LED):

| Colour | Power source | Broker connection |
|--------|--------------|-------------------|
| Green | Mains | Connected |
| Yellow (R+G) | Mains | Lost |
| Blue | Battery | Connected |
| Magenta (R+B) | Battery | Lost |
| Current colour blinking at 1 Hz | — | A device fault is active (default, §18 G6) |

### 4.5 UPS HAT / INA219

- Bus `/dev/i2c-1` (`dtparam=i2c_arm=on`, module `i2c-dev`), address `0x42`, registers big-endian.
- Vendor configuration (the HAT's example code, “32 V 2 A”): calibration register `0x05` = `4096` (shunt 0.1 Ω, current LSB 0.1 mA); config register `0x00` = `0x3EEF` (32 V range, ÷8 gain, 12-bit 32-sample averaging for bus and shunt, continuous). Write the calibration again if a read returns 0.
- Bus voltage = `(reg 0x02 >> 3) × 4 mV` (battery pack voltage). Current = signed `reg 0x04 × 0.1 mA`.
- **Mains detection** (the HAT has no signal): current < −100 mA for 3 consecutive samples → mains lost; > +50 mA for 3 samples → mains back. Near 0 mA (full battery on mains) keeps the previous state. At boot: mains = median of 3 quick samples > −100 mA. *Verify the sign on the board* (expected: negative = discharging).
- **Battery %** (estimate, vendor mapping): `pct = clamp((V − 6.0) / 2.4 × 100, 0, 100)`, reported in 10 % steps. Low battery ≤ 30 % (≈ 6.7 V); clean shutdown at ≤ 6.4 V (3.2 V/cell); both need 3 consecutive samples and only apply on battery.

### 4.6 Power budget

Typical ≈ 0.77 A (Pi 0.60, MQ-2 0.12, IR 0.04, LEDs 0.014), maximum ≈ 1.64 A (with speaker 0.12 A) against the HAT's 2.5 A. Autonomy ≈ 19.2 Wh (≈ 16 Wh usable) → ≈ 4 h typical, ≈ 2 h at maximum; acceptance test ≥ 3 h.

### 4.7 To verify on the hardware

1. `i2cdetect -y 1` shows `0x42`; sign of the INA219 current with the charger unplugged.
2. IR sensor model, output type, distance to the leaf.
3. GPIO21 voltage with the divider connected and no gas (≥ ≈ 2.5 V); MQ-2 trimmer threshold against a controlled smoke source.
4. Transistor pinout; speaker volume with 33 Ω; GPIO18 LOW when silent (scope).
5. RGB red channel fully off with the GPIO HIGH.
6. Both PWM channels on GPIO18 and GPIO13 with our overlay, without touching GPIO19.
7. Whether the Pi 4 restarts by itself when mains returns after a low-battery shutdown (the HAT keeps 5 V on, so a halted Pi may stay halted — §18 issue 7).
8. Measured autonomy.

---

## 5. Software architecture

### 5.1 Process and thread model

One process, `/usr/bin/vigil`, started by `/etc/init.d/S99vigil`, running as root (needed for `SCHED_FIFO`, `/dev/watchdog`, GPIO, I²C and `/dev/vigil_pwm`). It has 9 pthreads plus the main thread and the Paho library's internal threads.

```
Producers ──push Event──► Event Queue ──epoll──► AlarmControllerThread ──commands──► Consumers

Producers (push into the Event Queue: std::queue + mutex + condvar + eventfd):
  LocalInputThread    key digit · PIN submit · button press
  IRSensorsThread     motion (sensor 1 | 2)
  MQ2SensorThread     smoke
  PowerMonitorThread  power lost/restored · battery low/critical/level
  NetworkMQTTThread   net lost/restored · remote arm/disarm/change_pin
  Output / Storage    faults · PIN-save result

AlarmControllerThread: 4 FSMs (Operational, Fire, Network, Power) · 4 timerfds · wrong-PIN counter · PIN hash

Consumers (one CommandQueue<T> each):
  OutputCmd  → OutputThread       → /dev/vigil_pwm (speaker, red LED) · libgpiod (RGB LED)
  StorageCmd → StorageThread      → SQLite ── committed event with its event_id ──► NetworkMQTTThread
  NetCmd     → NetworkMQTTThread  → EMQX (MQTT 5 / TLS); reads StorageThread directly for the outbox and get_events

SupervisorThread: heartbeats of the 8 workers → restarts a hung thread → feeds /dev/watchdog (bcm2835_wdt)
```

**Why (D-01, D-02):** the older class diagram had a central controller plus `DelayTimer`/`AlarmActuator`, which contradicted the event-queue model in the same deck. One consumer owning all logic makes the behaviour deterministic and testable; the producers only report facts, the consumers only execute commands, and the controller never waits for I/O.

### 5.2 Threads

| Thread | Policy / prio | Activation | Interfaces | Produces | Heartbeat timeout |
|--------|---------------|------------|------------|----------|-------------------|
| `LocalInputThread` | FIFO 50 | periodic **15 ms** (absolute-time sleep) | libgpiod: rows + button (1 request), columns (1 request) | `EVT_KEY_DIGIT`, `EVT_PIN_SUBMIT`(pin), `EVT_BUTTON_PRESS` | 1 s |
| `IRSensorsThread` | FIFO 70 | blocking edge wait on GPIO16+20, 1 s timeout | libgpiod (1 request, 2 lines) | `EVT_MOTION_DETECTED`(sourceId 1/2), `EVT_FAULT`(IR1/IR2) | 3 s |
| `MQ2SensorThread` | FIFO 80 | periodic **100 ms** | libgpiod: GPIO21 | `EVT_SMOKE_DETECTED`, `EVT_FAULT`(MQ2); exposes `isSmokePresent()` | 1 s |
| `PowerMonitorThread` | FIFO 50 | periodic **1 s** | i2c-dev → `Ina219` | `EVT_POWER_LOST`/`RESTORED`, `EVT_BATTERY_LOW`, `EVT_BATTERY_CRITICAL`, `EVT_BATTERY_LEVEL` | 3 s |
| `AlarmControllerThread` | FIFO 80 | `epoll_wait` on the Event Queue eventfd + 4 timerfds, **1 s timeout** | none (commands only) | internal events; `OutputCmd`, `StorageCmd`, `NetCmd` | 3 s |
| `OutputThread` | FIFO 80 | `CommandQueue<OutputCmd>` (`popFor` ≤ 500 ms) | `PwmDevice` (`/dev/vigil_pwm`), libgpiod RGB | `EVT_FAULT`(SPEAKER/PWM_DRIVER) | 3 s |
| `StorageThread` | OTHER | `CommandQueue<StorageCmd>` (`popFor` 1 s) | `Database` (SQLite) | `EVT_PIN_SAVE_RESULT`, `EVT_FAULT`(DATABASE); hands logged events to Network | 10 s |
| `NetworkMQTTThread` | OTHER | `CommandQueue<NetCmd>` (`popFor` 100 ms) + non-blocking netlink poll | `MqttClient` (Paho C++), netlink | `EVT_NET_DISCONNECTED`/`RESTORED`, `EVT_REMOTE_ARM`/`DISARM`/`CHANGE_PIN` | 10 s |
| `SupervisorThread` | OTHER | periodic 1 s | `/dev/watchdog` | — | covered by the watchdog |

**Why these priorities (D-05):** MQ-2 → controller → output is the fire path; an alarm cannot sound faster than its slowest stage, so all three share the top priority. IR is next (intrusion has a 30 s entry delay anyway). Keypad and power are human/slow events. Storage, network and the supervisor are best-effort; the supervisor is deliberately low so that, if lower threads are starved, it stops feeding the watchdog and the Pi resets.

The main thread does the start-up sequence (§6.8), starts the threads, then waits in `sigwait()` for `SIGTERM`/`SIGINT` and stops everything in reverse order. All other threads block all signals.

### 5.3 Queues

**`EventQueue`** (all producers → `AlarmControllerThread`): `std::queue<Event>` + mutex + condition variable + `eventfd` (`EFD_NONBLOCK | EFD_CLOEXEC`).

- `push(e)`: lock, push, unlock, `write(efd, 1)`. Never blocks for long.
- `tryPop(e)`: non-blocking.
- `getFd()`: the controller registers it in its epoll set.
- The controller reads the eventfd once (this resets the counter) and then **drains the queue completely** with `tryPop` before waiting again (BR-LOOP-2), because one eventfd read can stand for several events.

**`CommandQueue<T>`** (one per consumer: `OutputCmd`, `StorageCmd`, `NetCmd`): `std::queue<T>` + mutex + condition variable, `push(cmd)`, blocking `pop()`. Implementation additions: `popFor(timeout)` (so every consumer can send heartbeats and run its own timers) and `close()` (wakes the consumer for shutdown).

**Mutexes:** the queues and the `Database` are shared between `SCHED_FIFO` and `SCHED_OTHER` threads, so use priority-inheritance mutexes (`PTHREAD_PRIO_INHERIT`). Implement a small `PiMutex` class (BasicLockable: `lock()`/`unlock()`) and use it with `std::unique_lock` and `std::condition_variable_any`. Keep critical sections to a copy in or out.

### 5.4 `Event` and the event catalogue

```cpp
struct Event {
    EventType   type;
    int         sourceId  = 0;   // IR sensor 1/2, FaultComponent, etc.
    time_t      timestamp = 0;   // CLOCK_REALTIME, for logs and messages
    std::string payload;         // small text/JSON; see table (never log it for PIN events)
    uint64_t    monoNs    = 0;   // implementation addition: CLOCK_MONOTONIC at detection, for latency tests
};
```

| EventType | Producer | Payload / sourceId | Handled by |
|-----------|----------|--------------------|------------|
| `EVT_KEY_DIGIT` | LocalInput | none (do **not** send the digit) | controller: key beep |
| `EVT_PIN_SUBMIT` | LocalInput | the typed PIN (secret) | local PIN validation (§6.6) |
| `EVT_BUTTON_PRESS` | LocalInput | — | Operational FSM |
| `EVT_MOTION_DETECTED` | IR | sourceId 1 or 2 | Operational FSM |
| `EVT_SMOKE_DETECTED` | MQ2 | — | Fire FSM |
| `EVT_POWER_LOST` | Power | battery % (10 % step) | Power FSM |
| `EVT_POWER_RESTORED` | Power | — | Power FSM |
| `EVT_BATTERY_LOW` | Power | battery % | Power FSM |
| `EVT_BATTERY_CRITICAL` | Power | voltage | Power FSM |
| `EVT_NET_DISCONNECTED` / `EVT_NET_RESTORED` | Network | — | Network FSM (RGB) |
| `EVT_REMOTE_ARM` | Network | `{"request_id"}` | `handleRemoteArm()` |
| `EVT_REMOTE_DISARM` | Network | `{"request_id","pin"}` | `handleRemoteDisarm()` |
| `EVT_REMOTE_CHANGE_PIN` | Network | `{"request_id","pin","new_pin"}` | `handleChangePin()` |
| *Raised inside the controller:* `EVT_SYSTEM_STARTUP`, `EVT_PIN_CORRECT`, `EVT_PIN_WRONG`, `EVT_EXIT_DELAY_TIMEOUT`, `EVT_ENTRY_DELAY_TIMEOUT`, `EVT_SMOKE_SILENCE_TIMEOUT`, `EVT_LOCKOUT_TIMEOUT` | controller (timerfd expiry, PIN check) | — | FSMs |
| `EVT_RETRY_TIMER` | **internal to NetworkMQTTThread** (reconnect backoff); never queued | — | Network thread only |
| *Additions (close documented gaps, §18):* `EVT_BATTERY_LEVEL` | Power | battery % (10 % step) | status update (issue 10) |
| `EVT_FAULT` | IR, MQ2, Output, Storage | sourceId = `FaultComponent` | FAULT alert once per component (issue 24) |
| `EVT_PIN_SAVE_RESULT` | Storage | `{"request_id","ok"}` | completes `change_pin` (issue 28) |

`FaultComponent`: `IR1, IR2, MQ2, SPEAKER, PWM_DRIVER, DATABASE`.

`get_events` never enters the Event Queue: `NetworkMQTTThread` reads `StorageThread::getEventsSince()` directly and answers.

### 5.5 Commands

```cpp
enum class SpeakerPattern : uint8_t { OFF, KEY_BEEP, ERROR_BEEP, WARNING, INTRUSION, FIRE };  // mapped to vigil_pwm.h by OutputThread (§7.7)
enum class RedLedMode     : uint8_t { OFF, ON, FLASH };
enum class RgbColour      : uint8_t { OFF, GREEN, YELLOW, BLUE, MAGENTA };

struct OutputCmd {               // AlarmController → Output
    enum class Kind { SpeakerBase, SpeakerBeep, RedLed, RedLedBlink, Rgb } kind;
    SpeakerPattern speaker = SpeakerPattern::OFF;   // SpeakerBase / SpeakerBeep
    RedLedMode     led     = RedLedMode::OFF;       // RedLed
    RgbColour      colour  = RgbColour::OFF;        // Rgb
    bool           faultBlink = false;              // Rgb
};

struct StorageCmd {              // AlarmController → Storage
    enum class Kind { LogEvent, SavePinHash, Backup, CloseForShutdown } kind;
    std::string eventType;               // LogEvent: EVENT_LOG.event_type
    std::optional<std::string> details;  // LogEvent
    bool        isAlert = false;         // LogEvent: true → also an outbox row + topic alert
    time_t      timestamp = 0;           // LogEvent
    bool        thenShutdown = false;    // LogEvent of SHUTDOWN: Network runs the shutdown sequence after it
    std::string pinHash, requestId;      // SavePinHash
    std::shared_ptr<std::promise<bool>> done;  // CloseForShutdown (and optional acks)
};

struct NetCmd {                  // AlarmController / Storage / Paho callbacks → Network
    enum class Kind {
        LoggedEvent,   // from Storage: event committed with its event_id (alert → drain outbox; log event → publish once)
        Status,        // from controller: new status snapshot (JSON); cached and published (retained)
        Response,      // from controller: {request_id, result, detail?}
        // internal, posted by Paho callbacks so that only this thread touches MQTT state:
        Connected, ConnectionLost, MessageArrived, Delivered
    } kind;
    std::string json;                    // payload
    int         eventId = 0;
    bool        isAlert = false, thenShutdown = false;
    std::shared_ptr<std::promise<bool>> done;
};
```

**Why Storage hands events to Network (implementation detail, not drawn in the class diagram):** every message carries `event_id` = `EVENT_LOG.id`, which exists only after the insert, and the outbox rule says the `PENDING_NOTIFICATION` row must exist **before** the publish. Making `StorageThread` insert the row(s) in one transaction and then post `NetCmd::LoggedEvent` keeps the controller free of disk I/O and guarantees the order. Implement it through a small interface (`IEventSink`) injected into `StorageThread`, so it stays decoupled (dependency inversion).

### 5.6 Real-time rules

| ID | Rule |
|----|------|
| RT-1 | `mlockall(MCL_CURRENT \| MCL_FUTURE)` at start-up; reserve queue capacity early. |
| RT-2 | Create threads with `pthread_attr_setinheritsched(PTHREAD_EXPLICIT_SCHED)`, `setschedpolicy`, `setschedparam`; check every return code and log the effective policy/priority (`pthread_getschedparam`). Name threads with `pthread_setname_np` (≤ 15 chars). |
| RT-3 | Periodic threads sleep with `clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, …)` on absolute deadlines (no drift). Record the worst-case jitter for the report. |
| RT-4 | Timers use `timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK \| TFD_CLOEXEC)` (immune to NTP steps). Event timestamps use `CLOCK_REALTIME`, ISO 8601 UTC. |
| RT-5 | No disk or network I/O in `SCHED_FIFO` threads. The controller only computes and enqueues. |
| RT-6 | PI mutexes on everything shared across priority levels (§5.3). |
| RT-7 | PBKDF2 runs in the controller: choose the iteration count so one hash takes ≤ ≈ 50 ms on the Pi 4 (§9.4). |
| RT-8 | Every blocking wait has a timeout so each thread sends a heartbeat at least once per second. |
| RT-9 | Leave the kernel RT throttling at the default (95 %). For measurements, set the CPU governor to `performance`. |

**Latency budgets (slide 58):** sensor event → speaker ≤ 1 s (MQ-2 confirmation ≈ 0.5 s + a few ms); key press → beep ≤ 100 ms (2 scans of 15 ms + a few ms); alarm → phone notification ≤ 5 s; fire alarm ≤ 1 s under CPU stress, DB writes and a resend backlog.

### 5.7 OOP / SOLID mapping (for the report)

- **S**: each thread has one job; the FSM logic lives in a pure `AlarmLogic` class used by `AlarmControllerThread`; hashing in `PinHasher`; JSON in `Protocol`.
- **O**: new tone patterns are table entries in the driver; new events/commands extend enums without touching producers.
- **L**: the 8 workers are interchangeable `WorkerThread`s for `SupervisorThread` (`start()`, `stop()`, `run()`, `heartbeat()`).
- **I**: small hardware interfaces (`IGpioInput`, `IGpioOutput`, `IPwm`, `IPowerSensor`) instead of one big HAL.
- **D**: threads receive their hardware objects and queues by constructor injection from `main()`, so the same code runs with real hardware on the Pi and with fakes/simulation on the laptop.

---

## 6. Behaviour specification (AlarmControllerThread)

All rules in this section are implemented in one pure class (`AlarmLogic`, no threads, no I/O, time passed in) that `AlarmControllerThread` drives. `AlarmLogic::handle(event)` returns a list of actions (start/stop timer, `OutputCmd`, `StorageCmd`, `NetCmd`); the thread executes them. This keeps the class diagram's `AlarmControllerThread` interface (`handleEvent`, `onTimerExpired`, `verifyPin`, `handleRemoteArm`, `handleRemoteDisarm`, `handleChangePin`) and makes every rule below unit-testable on the laptop.

### 6.1 State held by the controller

| Variable | Values / meaning |
|----------|------------------|
| `opState` | `OperationalState`: DISARMED, EXIT_DELAY, ARMED, ENTRY_DELAY, INTRUSION_ALARM |
| `fireState` | `FireState`: NORMAL, FIRE_ALARM, SILENCED |
| `powerState` | `PowerState`: AC_POWERED, BATTERY_MODE, LOW_BATTERY, SHUTDOWN (final) |
| `netState` | `NetState`: NET_CONNECTED, NET_LOST |
| timers | `exitTimerFd` 60 s, `entryTimerFd` 30 s, `silenceTimerFd` 10 min, `lockoutTimerFd` 5 min (values from config) |
| `wrongPinCount`, `lockoutUntil` | shared keypad + app counter; lockout end (monotonic) |
| `pinHash` | loaded once at start-up; replaced only after Storage confirms a PIN change |
| `pendingPinChange` | `{request_id, newHash}` while a save is in progress |
| `entrySensor` | 1 or 2: the IR sensor that started the entry delay |
| `batteryPct` | last 10 % step |
| `activeFaults` | set of `FaultComponent` already reported |
| smoke level | read from `MQ2SensorThread::isSmokePresent()` (atomic, debounced) at the silence timeout |

### 6.2 Control loop (slide 42) — `AlarmControllerThread::run()`

| ID | Rule |
|----|------|
| BR-LOOP-1 | `epoll_wait` on the Event Queue eventfd and the 4 timerfds with a **1 s timeout**; call `heartbeat()` on every pass. |
| BR-LOOP-2 | Eventfd ready → read it once, then **drain** the Event Queue with `tryPop()` and handle each event in FIFO order. |
| BR-LOOP-3 | Timerfd ready → read the 8-byte expiry count and handle the matching `EVT_*_TIMEOUT`. |
| BR-LOOP-4 | Dispatch every event to **every FSM that handles it, Fire FSM first** (a keypad PIN during a fire alarm touches the Fire and the Operational FSM; Remote Arm/Disarm check the fire state first). |
| BR-LOOP-5 | After each event, recompute the outputs (§6.10) and the status (§6.11) and send `OutputCmd`/`NetCmd::Status` only for what changed. |
| BR-LOOP-6 | Never block: storage and network work leaves as commands. The only allowed waits are the PBKDF2 hash (≤ ≈ 50 ms) and the terminal shutdown sequence. |

### 6.3 Operational FSM (slide 37)

| ID | From | Event [guard] | Actions | To |
|----|------|---------------|---------|----|
| O-1 | start | `EVT_SYSTEM_STARTUP` | red LED off, no tone; **no** DISARMED event at boot | DISARMED |
| O-2 | DISARMED | `EVT_BUTTON_PRESS` [fireState ≠ FIRE_ALARM] | KEY_BEEP, start exit timer, red LED on. No log event yet; status stays DISARMED | EXIT_DELAY |
| O-3 | DISARMED | `EVT_REMOTE_ARM` [fireState ≠ FIRE_ALARM] | red LED on, reply OK, ARMED (REMOTE) event, status ARMED | ARMED |
| O-4 | EXIT_DELAY | `EVT_EXIT_DELAY_TIMEOUT` [fireState ≠ FIRE_ALARM] | ARMED (LOCAL) event, status ARMED | ARMED |
| O-5 | EXIT_DELAY | `EVT_REMOTE_ARM` [fireState ≠ FIRE_ALARM] | stop exit timer, reply OK, ARMED (REMOTE) event, status ARMED | ARMED |
| O-6 | ARMED | `EVT_MOTION_DETECTED` | start entry timer, WARNING tone, remember `entrySensor`. No notification, no log event | ENTRY_DELAY |
| O-7 | ENTRY_DELAY | `EVT_ENTRY_DELAY_TIMEOUT` | INTRUSION tone, red LED flashing, INTRUSION_ALARM alert (details `IR1`/`IR2`), status | INTRUSION_ALARM |
| O-8 | EXIT_DELAY, ARMED, ENTRY_DELAY, INTRUSION_ALARM | `EVT_PIN_CORRECT` (keypad) — **no fire guard** | stop exit/entry timers, stop tone, red LED off, DISARMED (LOCAL) event; from INTRUSION_ALARM also ALARM_SILENCED (INTRUSION) event; status | DISARMED |
| O-9 | same four states | `EVT_REMOTE_DISARM` [fireState ≠ FIRE_ALARM ∧ not locked out ∧ PIN valid] | stop timers, stop tone, red LED off, reply OK, DISARMED (REMOTE) event, status | DISARMED |
| O-10 | any | `EVT_PIN_WRONG` | §6.6 (error feedback, WRONG_PIN, counter, maybe lockout) | unchanged |
| O-11 | DISARMED, EXIT_DELAY | `EVT_MOTION_DETECTED` | ignored | — |
| O-12 | ENTRY_DELAY, INTRUSION_ALARM | `EVT_MOTION_DETECTED` | ignored | — |
| O-13 | EXIT_DELAY | `EVT_EXIT_DELAY_TIMEOUT` [fireState = FIRE_ALARM] | guard blocks: stay in EXIT_DELAY (the keypad PIN that silences the fire will disarm) | — |
| O-14 | DISARMED [fireState = FIRE_ALARM] or any other state | `EVT_BUTTON_PRESS` | rejected/ignored; ERROR_BEEP (dropped while an alarm sounds) — default G12 | — |

The guard `[fireState ≠ FIRE_ALARM && PIN valid]` belongs **only** to Remote Disarm (the slide still prints it next to both events; that fix is pending, §18.3).

### 6.4 Fire FSM (slide 38)

| ID | From | Event [guard] | Actions | To |
|----|------|---------------|---------|----|
| F-1 | start | `EVT_SYSTEM_STARTUP` | none (MQ-2 monitored 24/7 after warm-up) | NORMAL |
| F-2 | NORMAL | `EVT_SMOKE_DETECTED` | FIRE tone (has priority), red LED flashing, FIRE_ALARM alert (details `DETECTED`), status | FIRE_ALARM |
| F-3 | FIRE_ALARM | `EVT_SMOKE_DETECTED` | ignored (issue 2) | — |
| F-4 | FIRE_ALARM | `EVT_REMOTE_DISARM` | reply `REJECTED_FIRE_ACTIVE`; PIN not checked; no WRONG_PIN | — |
| F-5 | FIRE_ALARM | `EVT_PIN_CORRECT` (**keypad only**) | fire tone off, start silence timer (10 min), ALARM_SILENCED (FIRE) alert; Operational FSM → DISARMED (O-8, also silences an intrusion alarm and stops its timers). Log order: §6.6 | SILENCED |
| F-6 | SILENCED | `EVT_SMOKE_DETECTED` | ignored | — |
| F-7 | SILENCED | `EVT_SMOKE_SILENCE_TIMEOUT` [smoke present] | FIRE_ALARM alert (details `REACTIVATED`), fire tone, LED flashing, status. Operational state is not changed | FIRE_ALARM |
| F-8 | SILENCED | `EVT_SMOKE_SILENCE_TIMEOUT` [no smoke] | HAZARD_CLEARED alert (details null), status | NORMAL |

There is **no** FIRE_ALARM → NORMAL transition (fail-loud). Arming is blocked in FIRE_ALARM and allowed in SILENCED. During a PIN lockout no PIN is accepted and the alarm keeps sounding.

The `[smoke present]` guard needs the current level, not an edge: `MQ2SensorThread` keeps a debounced `std::atomic<bool> smokePresent` and the controller reads it at the timeout (default G2).

### 6.5 Power FSM (slide 40) and Network FSM (slide 39)

| ID | From | Event [guard] | Actions | To |
|----|------|---------------|---------|----|
| P-1 | start | `EVT_SYSTEM_STARTUP` [mains at boot] | RGB mains colour; if the last power event in EVENT_LOG is SHUTDOWN or POWER_LOST → POWER_RESTORED alert (details `BOOT`) (issue 6) | AC_POWERED |
| P-2 | start | `EVT_SYSTEM_STARTUP` [on battery at boot] | RGB battery colour; if the last power event is not POWER_LOST → POWER_LOST alert (issue 9, default) | BATTERY_MODE |
| P-3 | AC_POWERED | `EVT_POWER_LOST` | POWER_LOST alert (details battery %), RGB battery colour, status | BATTERY_MODE |
| P-4 | BATTERY_MODE, LOW_BATTERY | `EVT_POWER_RESTORED` | POWER_RESTORED alert (details null), RGB mains colour, status | AC_POWERED |
| P-5 | BATTERY_MODE | `EVT_BATTERY_LOW` [≤ 30 %] | LOW_BATTERY alert (details battery %) — once per outage | LOW_BATTERY |
| P-6 | LOW_BATTERY | `EVT_BATTERY_CRITICAL` [≤ 6.4 V] | shutdown sequence (§6.9) | SHUTDOWN |
| P-7 | BATTERY_MODE | `EVT_BATTERY_CRITICAL` [≤ 6.4 V] | shutdown sequence (voltage sag under load; issue 8, default) | SHUTDOWN |
| P-8 | AC_POWERED | `EVT_BATTERY_LOW` / `EVT_BATTERY_CRITICAL` | ignored | — |
| P-9 | any | `EVT_BATTERY_LEVEL` | update `batteryPct`; status if it changed (issue 10) | — |

| ID | From | Event | Actions (in the controller) | To |
|----|------|-------|-----------------------------|----|
| N-1 | start | `EVT_SYSTEM_STARTUP` | RGB “lost” colour until the first CONNACK | NET_LOST |
| N-2 | NET_LOST | `EVT_NET_RESTORED` | RGB normal colour. (NetworkMQTTThread has already published `online`, the status and started the outbox resend.) | NET_CONNECTED |
| N-3 | NET_CONNECTED | `EVT_NET_DISCONNECTED` | RGB “lost” colour. New alerts stay PENDING (automatic: the outbox is always used) | NET_LOST |
| N-4 | NET_LOST | `EVT_RETRY_TIMER` | **inside NetworkMQTTThread only**: reconnect with backoff 1 s → 60 s; auth errors every 5 min | NET_LOST |

The local alarm never depends on the network state.

### 6.6 PIN rules

| ID | Rule |
|----|------|
| BR-PIN-1 | One `wrongPinCount` for keypad and app. |
| BR-PIN-2 | The 5th consecutive wrong PIN starts a 5 min lockout (`lockoutTimerFd`) and a PIN_LOCKOUT alert with details `LOCAL` or `REMOTE` (where the 5th came from). |
| BR-PIN-3 | The counter resets after any valid PIN and when the lockout ends (`EVT_LOCKOUT_TIMEOUT`). |
| BR-PIN-4 | During a lockout **no PIN is hashed or compared**, not even during a fire alarm (the alarm keeps sounding). Keypad → ERROR_BEEP. Remote → `LOCKED_OUT` (detail = remaining minutes, rounded up). |
| BR-PIN-5 | Verification = PBKDF2-HMAC-SHA256 with the salt and iteration count stored in `pinHash`, compared with `CRYPTO_memcmp` (constant time). |
| BR-PIN-6 | Each PIN that was actually checked and is wrong logs WRONG_PIN (`LOCAL`/`REMOTE`) and increments the counter. The PIN itself is never logged, published, or written to syslog; zero the buffers after use. |
| BR-PIN-7 | Local wrong PIN → ERROR_BEEP + red LED blink (both dropped while an alarm sound/flash is active). Remote wrong PIN → reply `INVALID_PIN` only. |
| BR-PIN-8 | Valid PIN format: 4–6 digits `[0-9]`. A keypad entry of another length is simply a wrong PIN. |
| BR-PIN-9 | The PIN can be **changed only from the app** (`change_pin`). |

**Keypad entry (LocalInputThread, defaults G3):** digits are appended to `pinBuffer` (at most 8 kept) and each one sends `EVT_KEY_DIGIT`; `#` submits (`EVT_PIN_SUBMIT` with the buffer, then clears it; `#` on an empty buffer does nothing); `*` clears the buffer; the buffer is cleared silently after 10 s without a key (PIN entry timeout).

**Local PIN (`EVT_PIN_SUBMIT`, slide 43):**

1. Lockout active → ERROR_BEEP; stop (nothing logged).
2. Hash and compare. Wrong → BR-PIN-6/7, BR-PIN-2. Correct → reset the counter, raise `EVT_PIN_CORRECT`:
   - fireState = FIRE_ALARM → Fire FSM F-5 (→ SILENCED);
   - opState ≠ DISARMED → Operational O-8 (→ DISARMED);
   - log in this order: DISARMED (LOCAL) [if opState changed] → ALARM_SILENCED (INTRUSION) [if it was INTRUSION_ALARM] → ALARM_SILENCED (FIRE) [if the fire alarm was silenced];
   - if nothing was active (already DISARMED, no fire alarm): no event at all (issue 18), just a KEY_BEEP as acknowledgement.

**Remote Arm (`handleRemoteArm`, slides 20/29) — no PIN:**

1. fireState = FIRE_ALARM → `REJECTED_FIRE_ACTIVE`.
2. opState ∈ {ARMED, ENTRY_DELAY, INTRUSION_ALARM} → `ALREADY_ARMED`.
3. Otherwise (DISARMED or EXIT_DELAY; also allowed in SILENCED) → stop the exit timer if running → ARMED → reply `OK` → ARMED (REMOTE) event → status.

**Remote Disarm (`handleRemoteDisarm`) — checked in this order:**

1. fireState = FIRE_ALARM → `REJECTED_FIRE_ACTIVE` (PIN not checked, no WRONG_PIN).
2. opState = DISARMED → `ALREADY_DISARMED` (PIN not checked).
3. Lockout → `LOCKED_OUT` (PIN not checked).
4. PIN valid → reset counter, stop delay timers, DISARMED (this also silences an intrusion alarm), reply `OK`, DISARMED (REMOTE) event, status. PIN wrong → reply `INVALID_PIN`, WRONG_PIN (REMOTE), counter (5th → lockout + PIN_LOCKOUT (REMOTE)).

A remote disarm of an intrusion alarm logs only DISARMED (REMOTE): the ALARM_SILENCED (INTRUSION) event is defined for the keypad (slide 26, wording kept on purpose).

**Change PIN (`handleChangePin`):**

1. Lockout → `LOCKED_OUT` (PIN not checked).
2. `pin` ≠ current PIN → `INVALID_PIN` + WRONG_PIN (REMOTE), counts toward the lockout. Correct → reset the counter.
3. `new_pin` not 4–6 digits → `INVALID_NEW_PIN`.
4. A save is already pending → `SAVE_FAILED` (detail `BUSY`) (default G13).
5. Hash `new_pin` with a new random salt, send `StorageCmd::SavePinHash`, remember `pendingPinChange`.
6. `EVT_PIN_SAVE_RESULT` ok → replace `pinHash` in RAM, reply `OK`, PIN_CHANGED event (details null); StorageThread backs up the database (§9.5). Failed → reply `SAVE_FAILED`; the old PIN stays active (issue 28: memory is updated only after Storage confirms).

Messages for a remote command go out in this order (slide 29): response first, then the log event, then the status (OK only).

### 6.7 Other event handling

| Event | Handling |
|-------|----------|
| `EVT_KEY_DIGIT` | KEY_BEEP (dropped while an alarm sound is active). Nothing logged. |
| `EVT_FAULT` | If the component is not in `activeFaults`: add it, FAULT alert (details = component), RGB fault blink on. Sent once per component per boot. |
| `EVT_BATTERY_LEVEL` | P-9. |
| `EVT_LOCKOUT_TIMEOUT` | lockout over; counter reset. |

### 6.8 Start-up sequence (slide 41)

1. Read `/etc/vigil/broker.conf` (`-c` option) and `/etc/vigil/vigil.conf` (`-C` option; defaults if missing); `openlog("vigil", LOG_PID, LOG_DAEMON)`; `mlockall`; block signals.
2. Create the Event Queue and the three Command Queues.
3. Open `/dev/watchdog` early (issue 16): set a 15 s timeout; `main` feeds it until `SupervisorThread` takes over.
4. Open `/dev/vigil_pwm` and request all GPIO lines (each thread object requests its lines in its constructor). A failure does not stop the start-up: queue `EVT_FAULT` (`PWM_DRIVER`, `IR1`, `IR2`, `MQ2`) and run degraded (issue 15).
5. Database:
   - opens and `PRAGMA quick_check` = `ok` → load the PIN hash; make a fresh backup (§9.5);
   - else, if `vigil.db.bak` has a PIN row → restore it (copy over the bad file, reopen) → load the PIN hash;
   - else → rename the bad file to `vigil.db.corrupt-<time>`, create a new database (issue 11), store the default PIN hash from `/etc/vigil/secrets/default_pin_hash`, queue `EVT_FAULT(DATABASE)`, write “PIN change required” to syslog;
   - first boot (no database file at all) → create it and store the default PIN hash, without FAULT.
   The PIN hash is loaded **once**, on every path, before any thread starts.
6. Read the INA219 (3 quick samples) → `mainsAtBoot`, `batteryPct`; read the last power event (POWER_LOST/POWER_RESTORED/SHUTDOWN) from EVENT_LOG.
7. Start the threads in this order: Storage, Output, AlarmController, NetworkMQTT, IRSensors, MQ2Sensor, LocalInput, PowerMonitor, Supervisor.
8. The controller handles `EVT_SYSTEM_STARTUP`: DISARMED, NORMAL, power state from step 6 (P-1/P-2), NET_LOST; red LED off, speaker off, RGB from power and network.
9. MQ-2 warm-up runs **in parallel** inside `MQ2SensorThread` (no smoke events during it; fire detection inactive, issue 14). IR needs no warm-up.
10. Normal operation; `main` waits in `sigwait()`.

`S99vigil` starts the application without waiting for NTP: the local alarm must not depend on the network. Until the clock is right, TLS fails and the network thread simply retries.

### 6.9 Low-battery shutdown sequence (slides 23, 31, 40; issue 27)

1. Controller enters SHUTDOWN and sends `StorageCmd::LogEvent{SHUTDOWN, details "LOW_BATTERY", isAlert, thenShutdown}`.
2. StorageThread inserts EVENT_LOG + PENDING row (one transaction) and posts `NetCmd::LoggedEvent{thenShutdown}`.
3. NetworkMQTTThread, if connected: publishes the pending alerts, waits **≤ 5 s** for the SHUTDOWN alert's PUBACK (mark SENT; otherwise the row stays PENDING and is sent after the next boot); publishes `{"state":"offline","reason":"LOW_BATTERY_SHUTDOWN","timestamp":…}` on `connection` (retained, QoS 1) and waits briefly for its PUBACK (sends DISCONNECT anyway on timeout); sends **DISCONNECT 0x00** (the broker discards the Last Will). Then it fulfils the promise. If not connected it fulfils it at once.
4. Controller waits for that promise (≤ 10 s), then `StorageCmd::CloseForShutdown` (WAL checkpoint, close) and waits (≤ 3 s).
5. `main` stops the threads, turns the speaker and LEDs off, **disables the watchdog** (magic close `V`) so the Pi cannot reboot in a loop on an empty battery, calls `sync()` and `reboot(RB_POWER_OFF)`.
6. Next boot on mains: P-1 sends POWER_RESTORED (delivered as an alert after `online` and the status).

### 6.10 Output arbitration (one speaker, one red LED, one RGB LED)

Recomputed after every event from the combined state (issue 20):

```
speakerBase = fireState == FIRE_ALARM        ? FIRE
            : opState   == INTRUSION_ALARM   ? INTRUSION
            : opState   == ENTRY_DELAY       ? WARNING
            :                                  OFF
redLed      = (fireState == FIRE_ALARM || opState == INTRUSION_ALARM) ? FLASH
            : opState in {EXIT_DELAY, ARMED, ENTRY_DELAY}             ? ON
            :                                                           OFF
rgb         = colour(powerState == AC_POWERED ? MAINS : BATTERY, netState)   // §4.4 table
faultBlink  = !activeFaults.empty()
```

- One-shot beeps (KEY_BEEP, ERROR_BEEP) and the red LED blink are **dropped** while `speakerBase` is FIRE or INTRUSION, or the LED is flashing. They play **over** WARNING and then the warning resumes (needed during the entry delay, when the owner types the PIN).
- `OutputThread` applies the same rule again as a safety net.
- Fire has priority over intrusion: with both active the fire tone plays; one valid keypad PIN silences both.

### 6.11 Status snapshot (slide 21)

```
armed_state  = DISARMED        if opState in {DISARMED, EXIT_DELAY}
             = ARMED           if opState in {ARMED, ENTRY_DELAY}
             = INTRUSION_ALARM if opState == INTRUSION_ALARM
fire_state   = NORMAL | FIRE_ALARM | SILENCED
power_source = GRID if powerState == AC_POWERED else BATTERY
battery_pct  = batteryPct (multiple of 10)
timestamp    = time of the change (ISO 8601 UTC)
```

Send `NetCmd::Status` whenever one of the first four fields changes. NetworkMQTTThread caches the last snapshot and republishes it (retained) after every reconnection. Never publish it periodically.

### 6.12 What is logged and published

| event_type | Topic | Outbox | details | When |
|------------|-------|--------|---------|------|
| ARMED | event | no | `LOCAL` / `REMOTE` | exit-delay timeout / Remote Arm |
| DISARMED | event | no | `LOCAL` / `REMOTE` | valid PIN that leaves a non-disarmed state |
| WRONG_PIN | event | no | `LOCAL` / `REMOTE` | each checked wrong PIN |
| PIN_CHANGED | event | no | null | PIN change saved (the PIN is never logged) |
| ALARM_SILENCED | event | no | `INTRUSION` | valid keypad PIN during an intrusion alarm, after DISARMED |
| ALARM_SILENCED | **alert** | yes | `FIRE` | valid keypad PIN during a fire alarm |
| INTRUSION_ALARM | alert | yes | `IR1` / `IR2` (default G4) | entry-delay timeout |
| FIRE_ALARM | alert | yes | `DETECTED` / `REACTIVATED` | smoke in NORMAL / smoke at the silence timeout |
| HAZARD_CLEARED | alert | yes | null | silence timeout without smoke |
| POWER_LOST | alert | yes | battery %, e.g. `"80"` (default G4) | grid failure (and boot on battery, P-2) |
| POWER_RESTORED | alert | yes | null, or `BOOT` | grid back; boot on mains after SHUTDOWN/POWER_LOST |
| LOW_BATTERY | alert | yes | battery %, e.g. `"30"` | ≤ 30 % on battery, once per outage |
| SHUTDOWN | alert | yes | `LOW_BATTERY` | ≤ 6.4 V |
| FAULT | alert | yes | `IR1` `IR2` `MQ2` `SPEAKER` `PWM_DRIVER` `DATABASE` | first failure of a component |
| PIN_LOCKOUT | alert | yes | `LOCAL` / `REMOTE` | 5th consecutive wrong PIN |

Not logged in EVENT_LOG: key digits, button presses, the start of an entry delay, connectivity changes and broker connection rejections (these two go to syslog).

---

## 7. Component specifications (Pi application)

Public names follow the class diagram (slide 34). Private helpers and the internal `AlarmLogic` class are allowed. Every thread receives its queues and hardware objects through its constructor (from `main.cpp`).

### 7.1 `WorkerThread` (abstract) and `SupervisorThread`

```cpp
class WorkerThread {                         // pthread wrapper
public:
    WorkerThread(std::string name, int policy, int priority, std::chrono::milliseconds heartbeatTimeout);
    virtual ~WorkerThread();
    void start();                            // pthread_create with explicit sched attrs; running = true
    void stop();                             // running = false; wake(); timed join
    uint64_t lastHeartbeatNs() const;        // read by SupervisorThread
protected:
    virtual void run() = 0;                  // while (running) { wait with timeout; work; heartbeat(); }
    virtual void wake() {}                   // unblock the wait (close queue, write eventfd, …)
    void heartbeat();                        // atomic store of CLOCK_MONOTONIC "now" (→ onHeartbeat)
    pthread_t tid{};
    std::atomic<bool> running{false};
};
```

`SupervisorThread` (pthread, not a `WorkerThread`; owns `wdtFd`):

- Every 1 s compare each worker's last heartbeat with its timeout (§5.2).
- A missed heartbeat → syslog → `restartThread(id)`: `stop()` with a 2 s `pthread_timedjoin_np`; if the thread ended, `start()` it again.
- `feedWatchdog()` (`WDIOC_KEEPALIVE`) only while every worker is healthy or was restarted. If a thread cannot be joined, or one thread needed more than 3 restarts in 10 min, log it and **stop feeding**: `bcm2835_wdt` resets the Pi (timeout 15 s, the maximum of this watchdog).
- On a clean stop write the magic character `V` before closing, so the watchdog is disabled.
- Known limit: after a watchdog reset the system starts DISARMED, as the start-up flowchart defines.

### 7.2 `LocalInputThread` (keypad + arm button)

- Two `GpioLines` requests: inputs = rows 17, 27, 22, 23 + button 6 (pull-up); outputs = columns 24, 25, 5 (**open-drain**, initial 1 = released).
- Every 15 ms: for each column, drive it 0, wait ≈ 20 µs, read the 4 rows, release it; then read the button.
- Debounce (2 stable reads): a key is accepted when the **same single key** is seen in 2 consecutive scans and has not been reported since the last full release; nothing is reported while 2 or more keys are pressed (no ghost keys). Button: LOW for 2 scans after being HIGH for 2 scans (30 ms ≈ the 20–50 ms requirement) → `EVT_BUTTON_PRESS`.
- Keypad entry rules: §6.6 (`#` submit, `*` clear, 10 s entry timeout, at most 8 digits kept). Push `EVT_KEY_DIGIT` (no digit in the payload) for every digit, `EVT_PIN_SUBMIT` with the buffer on `#`, then wipe the buffer.
- Tests (slide 49): all 12 keys correct, no ghosts, single event per press with bouncing contacts, beep ≤ 100 ms.

### 7.3 `IRSensorsThread` (2 door/window sensors)

- One `GpioLines` request: lines 16 and 20, both-edge events, pull-up. Read both levels at start into `lastState[2]` (no event).
- `waitEdge(1 s)`; for each edge update `lastState`. A **rising** edge (LOW → HIGH = door/window opened) pushes `EVT_MOTION_DETECTED` with `sourceId` 1 or 2, unless the same sensor triggered less than 200 ms ago.
- The thread does not look at the alarm state; the Operational FSM ignores motion when it does not apply.
- Request or read failure → `EVT_FAULT(IR1|IR2)` once; retry the request every 10 s.
- Known limit: a door already open when the system arms produces no new edge, so it is only detected after it closes and opens again.

### 7.4 `MQ2SensorThread` (smoke/gas, 24/7)

- One `GpioLines` request: line 21, input, **bias disabled**; active-low.
- Every 100 ms read the line. Debounce: `requiredReads` (default 5) consecutive LOW reads → smoke present; the same number of HIGH reads → clear.
- Warm-up: during the first `warmUpS` seconds (default 60 s, still to be measured) no event is pushed and `smokePresent` stays false. Detection is inactive during this time.
- On the transition clear → present (after warm-up) push **one** `EVT_SMOKE_DETECTED`. Keep `std::atomic<bool> smokePresent` up to date for the controller's silence-timeout guard (`isSmokePresent()`).
- Read failure → `EVT_FAULT(MQ2)` once.

### 7.5 `PowerMonitorThread` (UPS HAT / INA219)

- `Ina219` on `/dev/i2c-1`, address `0x42`; write calibration and configuration at start (§4.5).
- Every 1 s read bus voltage and current; every decision needs **3 consecutive samples**:
  - current < −100 mA → `EVT_POWER_LOST` (payload battery %); current > +50 mA → `EVT_POWER_RESTORED`;
  - 10 % step changed → `EVT_BATTERY_LEVEL`;
  - on battery, ≤ 30 % → `EVT_BATTERY_LOW` (once per outage, re-armed when mains returns);
  - on battery, ≤ 6.4 V → `EVT_BATTERY_CRITICAL` (once).
- Initial state comes from the boot reading done in `main` (§6.8), so the hysteresis starts consistent.
- I²C errors: retry next period; after 10 consecutive failures log an error (there is no `FaultComponent` for the UPS; §18 G8).

### 7.6 `AlarmControllerThread`

Implements §6. Owns `epfd`, the 4 timerfds and an `AlarmLogic` instance; executes the actions it returns (`timerfd_settime` start/stop, push to the three command queues, push internal events back into `AlarmLogic`). `verifyPin()` uses `PinHasher` (§9.4).

### 7.7 `OutputThread`

- Owns `PwmDevice` (`/dev/vigil_pwm`) and a `GpioLines` request for the RGB lines 4, 26, 19 (outputs, **active-low flag**, initial 0 = off). It is the **only** client of `/dev/vigil_pwm`.
- Keeps the current speaker base, red-LED mode, RGB colour and fault-blink flag, and translates commands:

| Command | Driver call (`writeCommand(channel, patternId, frequencyHz, dutyPercent, enable)`) |
|---------|------------------------------------------------------------|
| SpeakerBase OFF | `(SPEAKER, PAT_CONSTANT, 0, 0, 0)` |
| SpeakerBase WARNING / INTRUSION / FIRE | `(SPEAKER, PAT_WARNING / PAT_INTRUSION / PAT_FIRE, 0, 50, 1)` |
| SpeakerBeep KEY / ERROR (dropped if the base is INTRUSION or FIRE) | `(SPEAKER, PAT_KEY_BEEP / PAT_ERROR_BEEP, 0, 50, 1)` |
| RedLed OFF / ON / FLASH | `(LED, PAT_CONSTANT, 0, 0, 0)` / `(LED, PAT_CONSTANT, 1000, 100, 1)` / `(LED, PAT_LED_FLASH, 0, 100, 1)` |
| RedLedBlink (dropped while FLASH) | `(LED, PAT_LED_BLINK, 0, 100, 1)` |
| Rgb | set the three lines; with `faultBlink` toggle the colour every 500 ms (`popFor` timeout) |

- `write()` returns immediately (patterns run in the kernel), so a long tone never blocks anything.
- A failed write → reopen the device and retry once; still failing → `EVT_FAULT(SPEAKER)` for an I/O error on the speaker channel, `EVT_FAULT(PWM_DRIVER)` if the device cannot be opened or the LED channel fails. The RGB LED is then the remaining indicator.

### 7.8 `StorageThread`

- Owns `Database` (`/var/lib/vigil/vigil.db`), schema in §9.
- Commands:
  - `LogEvent` → one transaction: insert EVENT_LOG; if `isAlert`, build the alert JSON with the new `event_id` and insert the PENDING_NOTIFICATION row; commit; then post `NetCmd::LoggedEvent` (via `IEventSink`).
  - `SavePinHash` → `UPDATE PIN_CONFIG … WHERE id = 1`; push `EVT_PIN_SAVE_RESULT`; on success make a backup.
  - `Backup` → SQLite online backup API into `vigil.db.bak.tmp`, `fsync`, `rename()` to `vigil.db.bak`.
  - `CloseForShutdown` → `PRAGMA wal_checkpoint(TRUNCATE)`, close, fulfil the promise.
- Thread-safe public methods (guarded by the `Database` PI mutex), used by NetworkMQTTThread and `main`: `loadPinHash()`, `getPendingAlerts(limit)`, `markSent(eventId)`, `getEventsSince(eventId)` (max 20), `lastPowerEvent()`.
- Database failure while running: first failure → `EVT_FAULT(DATABASE)`; continue in **degraded mode** (event ids continue from the last id in RAM; alerts kept in a RAM outbox so they are still published; lost on reboot); try to reopen or rebuild every 60 s. The alarm logic and the PIN in RAM keep working (test “Database Failure”, slide 57).

### 7.9 `NetworkMQTTThread`

- Owns `MqttClient`, a `LinkMonitor` (netlink), the cached status JSON, the outbox pacing (token bucket: 5 publishes/s, burst 5), the reconnect backoff and the in-flight map `token → event_id`.
- Paho callbacks run on Paho's own thread: they only post `NetCmd::Connected/ConnectionLost/MessageArrived/Delivered` to this thread's queue. Only this thread touches MQTT state.
- **Connect** (§10.2): on CONNACK `0x00` → reset backoff → subscribe `cmd` (QoS 1) → publish `online` (retained) → publish the cached status (retained) → start the outbox drain → push `EVT_NET_RESTORED`.
- **Connect failures:** `0x86` / `0x87` → retry every 5 min and log once per episode; `0x88`, `0x9F`, TLS failure, no CONNACK → backoff 1 → 2 → 4 → 8 → 16 → 32 → 60 → 60 s. Never give up.
- **Loss:** Paho connection lost, or the netlink monitor reports the Wi-Fi interface down → push `EVT_NET_DISCONNECTED` once per transition, clear the in-flight map, schedule a reconnect. Interface back up → reconnect at once (backoff reset). Detection bounds: broker loss ≤ 60 s (2 × keep-alive), Wi-Fi loss ≤ 2 s (issue 3).
- **Outbox drain** (alerts): while connected and tokens are available, take the oldest PENDING rows not in flight (`getPendingAlerts`) and publish them on `alert` (QoS 1, not retained). PUBACK reason `0x00`/`0x10` → `markSent(event_id)`. `0x97` (quota exceeded) → pause the drain (back off) and retry. Disconnected before PUBACK → the row stays PENDING. A `LoggedEvent` alert only triggers a drain, so alerts always leave in `event_id` order.
- **Log events:** publish once on `event` if connected (`0x97` → retry after a back-off); if not connected, drop (the app recovers them with `get_events`).
- **Status / responses:** status cached and published retained when connected; responses published on `response` when connected, otherwise dropped (the app times out).
- **Incoming `cmd`:** ignore retained messages; ignore messages without a Message Expiry property (defensive, from `NEXT_3_HOURS.md`); parse strictly (§10.6). `arm` / `disarm` / `change_pin` → push `EVT_REMOTE_*`; `get_events` → `getEventsSince(since)` → publish the reply. Malformed JSON, unknown type or missing `request_id` → syslog, no reply.
- **Shutdown:** §6.9.

### 7.10 Hardware and service wrappers

| Class | Interface | Notes |
|-------|-----------|-------|
| `GpioLines` | libgpiod **v1** C API (`gpiod_chip_open_by_label("pinctrl-bcm2711")`, `gpiod_chip_get_lines`, `gpiod_line_request_bulk_*_flags`, `gpiod_line_get/set_value_bulk`, `gpiod_line_event_wait_bulk`, `gpiod_line_event_read`) | One object = one line request. Offsets = BCM numbers (check the chip label with `gpiodetect`). Flags: `GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP / BIAS_DISABLE / ACTIVE_LOW / OPEN_DRAIN`. Methods from the diagram: `requestLines(offsets, dir, bias)`, `get(offset)`, `set(offset, value)`, `waitEdge(timeoutNs)`, plus `readEdges()` (offset, rising/falling, time). The diagram's `gpiod_line_request*` type is v2; keep the class API and implement it on v1 (§12.4). |
| `Ina219` | `open("/dev/i2c-1")`, `ioctl(fd, I2C_SLAVE, 0x42)`, write register pointer, read 2 bytes (big-endian) | `readBusVoltage()`, `readCurrent()`; configure in the constructor (§4.5). |
| `PwmDevice` | `/dev/vigil_pwm`, internal mutex | `writeCommand(channel, patternId, frequencyHz, dutyPercent, enable)` fills `struct vigil_pwm_cmd` and calls `write()`; returns 0 or `-errno`. |
| `Database` | SQLite C API (`sqlite3*`), opened with `SQLITE_OPEN_FULLMUTEX`, guarded by a PI mutex | `open(path)`, `exec(sql)`, `prepare(sql)`; prepared statements only (no string-built SQL). |
| `MqttClient` | Eclipse Paho MQTT C++ (`mqtt::async_client`), MQTT 5 | `connect()`, `publish(topic, payload, qos, retain)`, `subscribe(topic, qos)`, `disconnect()`. |
| `Watchdog` | `/dev/watchdog` | `WDIOC_SETTIMEOUT`, `WDIOC_KEEPALIVE`, magic close. |
| `LinkMonitor` | `socket(AF_NETLINK, SOCK_RAW \| SOCK_NONBLOCK, NETLINK_ROUTE)`, group `RTMGRP_LINK` | Reports `IFF_RUNNING` changes of the configured interface (default `wlan0`). |
| `PinHasher` | OpenSSL `PKCS5_PBKDF2_HMAC`, `RAND_bytes`, `CRYPTO_memcmp` | `hash(pin)`, `verify(pin, stored)` (§9.4). |
| `Protocol` | nlohmann::json | Build and validate every MQTT payload (§10). |
| `BrokerConfig`, `Config` | INI parser | §11. |

### 7.11 Simulation build (laptop)

CMake option `VIGIL_SIM=ON` replaces `GpioLines`, `PwmDevice`, `Ina219` and `Watchdog` with simulated classes driven by a small stdin console (`key 1234#`, `button`, `door 1 open`, `smoke on`, `mains off`, `volts 6.3`, …) and prints what the outputs would do. With the local Mosquitto broker this runs the whole system (logic, storage, MQTT, app) before the hardware is ready. The laptop has libgpiod **v2**, so the real `GpioLines` is compiled only for the target.

---

## 8. Custom Linux device driver: `vigil_pwm`

### 8.1 Decision (D-06, D-07, D-08)

Exactly **one** custom driver: a **platform driver on the kernel PWM framework** (consumer of `pwm-bcm2835`), exposed as the character device **`/dev/vigil_pwm`**, with two channels: PWM0/GPIO18 = speaker, PWM1/GPIO13 = red LED. Tone and blink patterns are stepped **in the kernel with hrtimers**; a command carries channel, pattern id, frequency, duty cycle and enable. Pins come from our device-tree overlay, so the driver does not depend on the wiring. A mutex serialises writers and invalid arguments are rejected with error codes. `OutputThread` is the only client, so `AlarmControllerThread` never blocks on sound or light.

**Why:** the course requires at least one driver of our own, and PWM is the only device whose timing benefits from kernel code (exact tone steps independent of user-space scheduling). Keypad, button, IR, MQ-2 and the RGB LED are plain GPIO (libgpiod) and the INA219 is plain I²C (i2c-dev), so custom drivers for them would add risk without benefit. **Consequence:** if the driver fails, the red LED fails with it; the fallback indicator is the RGB LED plus a FAULT alert.

Kernel facts (Raspberry Pi kernel **6.12.61**, commit `21b41014…` used by the Buildroot config; checked in the source):

- `pwm-bcm2835` sets `chip->atomic = true`, so **`pwm_apply_atomic()` may be called from the hrtimer callback**; use `pwm_apply_might_sleep()` in process context. `pwm_apply_state()` no longer exists.
- The PWM node (`pwm: pwm@7e20c000` in `bcm283x.dtsi`) has **`#pwm-cells = <3>`**, a 50 MHz clock (`assigned-clock-rates`) and `status = "disabled"`. Disabling a channel clears only its enable bit; the output then idles LOW (verify with a scope).
- 6.12 API details: `class_create(name)` takes one argument; platform `.remove` returns `void`; hrtimers use `hrtimer_init()` + `.function` (`hrtimer_setup()` came later); the PWM consumer API is `EXPORT_SYMBOL_GPL`, so the module needs `MODULE_LICENSE("GPL")`.

### 8.2 Device-tree overlay (`driver/dts/vigil-pwm-overlay.dts`)

```dts
/dts-v1/;
/plugin/;

/ {
	compatible = "brcm,bcm2711";

	fragment@0 {                               /* pin mux: GPIO18 = PWM0 (ALT5), GPIO13 = PWM1 (ALT0) */
		target = <&gpio>;
		__overlay__ {
			vigil_pwm_pins: vigil_pwm_pins {
				brcm,pins = <18 13>;
				brcm,function = <2 4>;        /* 2 = ALT5, 4 = ALT0 */
				brcm,pull = <1 1>;            /* pull-down: silent / LED off if nothing drives them */
			};
		};
	};

	fragment@1 {                               /* enable the PWM controller on our pins */
		target = <&pwm>;
		__overlay__ {
			pinctrl-names = "default";
			pinctrl-0 = <&vigil_pwm_pins>;
			status = "okay";
		};
	};

	fragment@2 {                               /* our consumer device → vigil_pwm.ko probes on it */
		target-path = "/";
		__overlay__ {
			vigil_pwm: vigil-pwm {
				compatible = "esrg,vigil-pwm";
				pwms = <&pwm 0 1000000 0>, <&pwm 1 1000000 0>;   /* channel, period ns, flags */
				pwm-names = "speaker", "led";
				status = "okay";
			};
		};
	};
};
```

Build: `dtc -@ -I dts -O dtb -o vigil-pwm.dtbo driver/dts/vigil-pwm-overlay.dts`; install into the boot partition's `overlays/`; enable with `dtoverlay=vigil-pwm` in `config.txt`. **Never** also load `pwm-2chan`.

### 8.3 User-space interface (`driver/include/uapi/vigil_pwm.h`, shared with `pi/`)

```c
/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_VIGIL_PWM_H
#define _UAPI_VIGIL_PWM_H
#include <linux/types.h>

#define VIGIL_PWM_DEVICE "/dev/vigil_pwm"

enum vigil_pwm_channel { VIGIL_CH_SPEAKER = 0 /* PWM0, GPIO18 */, VIGIL_CH_LED = 1 /* PWM1, GPIO13 */, VIGIL_CH_COUNT };

enum vigil_pwm_pattern {
	VIGIL_PAT_CONSTANT   = 0,  /* frequency_hz + duty_percent from the command (both channels) */
	VIGIL_PAT_KEY_BEEP   = 1,  /* speaker, one-shot */
	VIGIL_PAT_ERROR_BEEP = 2,  /* speaker, one-shot */
	VIGIL_PAT_WARNING    = 3,  /* speaker, repeat   */
	VIGIL_PAT_INTRUSION  = 4,  /* speaker, repeat   */
	VIGIL_PAT_FIRE       = 5,  /* speaker, repeat   */
	VIGIL_PAT_LED_FLASH  = 6,  /* LED, repeat       */
	VIGIL_PAT_LED_BLINK  = 7,  /* LED, one-shot     */
	VIGIL_PAT_COUNT
};

struct vigil_pwm_cmd {          /* written with write(fd, &cmd, sizeof cmd) — 8 bytes */
	__u8  channel;          /* enum vigil_pwm_channel */
	__u8  pattern_id;       /* enum vigil_pwm_pattern */
	__u8  duty_percent;     /* 0..100; speaker channel 1..50 (never DC on the coil) */
	__u8  enable;           /* 0 = stop the channel (output LOW), 1 = start */
	__u32 frequency_hz;     /* PAT_CONSTANT only: speaker 100..10000 Hz, LED 1..10000 Hz */
};
#endif
```

**Semantics.** Each channel has a **base** (off, constant, or a repeating pattern) and may play a **one-shot** over it:

| Command | Effect |
|---------|--------|
| `enable = 0` | cancel everything on the channel; base = off; PWM disabled (pin LOW) |
| `enable = 1`, `PAT_CONSTANT` | base = constant output at `frequency_hz` / `duty_percent` |
| `enable = 1`, repeating pattern | base = that pattern, from step 0 |
| `enable = 1`, one-shot pattern | play it now; when it ends, resume the base (repeating patterns restart at step 0) |

**Errors** (`write()` returns `-1` and sets `errno`): `EINVAL` — size ≠ `sizeof(struct vigil_pwm_cmd)`, unknown channel or pattern, pattern not allowed on that channel, duty or frequency out of range; `EFAULT` — bad user pointer; `EIO` — `pwm_apply_*()` failed; `EINTR` — interrupted while waiting for the mutex. Success returns `sizeof(cmd)`. Optional `read()` returns one status line per channel (base, active pattern, enabled) for tests.

Policy (which pattern wins) stays in user space (§6.10); the driver only provides the mechanism and protects the hardware (speaker duty ≤ 50 %, output LOW when stopped).

### 8.4 Pattern tables (in the driver)

```c
struct vigil_step { u32 freq_hz; u8 duty; u16 ms; };     /* freq_hz == 0 → output off for ms */
static const struct vigil_step key_beep[]   = { {2000, 50,  50} };
static const struct vigil_step error_beep[] = { { 700, 50, 150}, {0, 0, 100}, {700, 50, 150} };
static const struct vigil_step warning[]    = { {1000, 50, 100}, {0, 0, 900} };
static const struct vigil_step intrusion[]  = { { 800, 50, 250}, {1200, 50, 250} };
static const struct vigil_step fire[]       = { {2000, 50, 150}, {3000, 50, 150} };
static const struct vigil_step led_flash[]  = { {1000, 100, 250}, {0, 0, 250} };
static const struct vigil_step led_blink[]  = { {1000, 100, 100}, {0, 0, 100}, {1000, 100, 100},
                                                {0, 0, 100}, {1000, 100, 100}, {0, 0, 100} };
```

Frequencies and durations are the Hardware Context proposal; tune them by ear (low tones are weak on a 36 mm speaker).

### 8.5 Driver structure

- `probe()`: `devm_pwm_get(dev, "speaker")` and `devm_pwm_get(dev, "led")` (return `dev_err_probe()` on error, which handles `-EPROBE_DEFER`); disable both outputs; `hrtimer_init(…, CLOCK_MONOTONIC, HRTIMER_MODE_REL)` per channel; `mutex_init`; character device: `alloc_chrdev_region` → `cdev_init`/`cdev_add` → `class_create("vigil_pwm")` → `device_create(…, "vigil_pwm")` (devtmpfs creates `/dev/vigil_pwm`).
- `write()`: size check → `copy_from_user` → validate → `mutex_lock_interruptible` → `hrtimer_cancel()` on that channel (waits for a running callback, so the callback and `write()` never touch the channel together) → update base/active → apply the first step with `pwm_apply_might_sleep()` → `hrtimer_start()` if the pattern has more steps → unlock.
- hrtimer callback: advance the step (repeat, or on the end of a one-shot switch back to the base), apply it with **`pwm_apply_atomic()`**, `hrtimer_forward_now()` by the step duration, return `HRTIMER_RESTART` (or `NORESTART` when the base is constant/off).
- `period_ns = NSEC_PER_SEC / freq_hz`; `duty_cycle = period_ns * duty / 100`; `enabled = freq_hz != 0`.
- `release()` leaves the outputs as they are (fail-loud: an alarm keeps sounding if the application dies; the watchdog will reboot). The application stops both channels on a clean exit.
- `remove()` (returns `void`): cancel both timers, disable both PWMs, `device_destroy`, `class_destroy`, `cdev_del`, `unregister_chrdev_region`.
- `of_device_id` = `{ .compatible = "esrg,vigil-pwm" }`, `MODULE_DEVICE_TABLE(of, …)`, `module_platform_driver(…)`, `MODULE_LICENSE("GPL")`.

### 8.6 Build, load, test

```make
# driver/Makefile — works by hand and with Buildroot's kernel-module infrastructure
ifneq ($(KERNELRELEASE),)
obj-m     := vigil_pwm.o
ccflags-y := -I$(src)/include
else
BR            ?= $(HOME)/buildroot/buildroot-2026.08
KDIR          ?= $(BR)/output/build/linux-custom
# check the prefix with: ls $(BR)/output/host/bin/*-gcc   (no comment on the line below: make keeps trailing spaces)
CROSS_COMPILE ?= $(BR)/output/host/bin/aarch64-buildroot-linux-gnu-
all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) ARCH=arm64 CROSS_COMPILE=$(CROSS_COMPILE) modules
clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) ARCH=arm64 clean
endif
```

Load order on the Pi (no automatic module loading in this image): `modprobe pwm-bcm2835` (if it is a module) → `modprobe vigil_pwm` (or `insmod`). Check with `ls -l /dev/vigil_pwm`, `dmesg`, and `cat /sys/kernel/debug/pwm` (after `mount -t debugfs none /sys/kernel/debug`).

`driver/test/vigil_pwm_test.c` (cross-compiled) covers slide 53: every pattern on both channels; out-of-range values → `EINVAL`; several threads writing at once → serialised, state intact; a long pattern while other commands arrive → `write()` returns at once; scope: tone steps match the table and GPIO18 is LOW when stopped; `insmod`/`rmmod` loop without warnings.

---

## 9. Data storage (SQLite)

### 9.1 Files and settings

- Database `/var/lib/vigil/vigil.db` (persistent ext4; in Buildroot's skeleton `/var/log` is volatile but `/var/lib` is not), backup `/var/lib/vigil/vigil.db.bak`.
- On open: `PRAGMA journal_mode=WAL; PRAGMA synchronous=FULL; PRAGMA foreign_keys=ON; PRAGMA busy_timeout=2000;` and `PRAGMA user_version` for schema versioning (start at 1). Writes are a few per minute at most, so `FULL` costs nothing noticeable and protects PIN changes.
- Times are stored as **TEXT, ISO 8601 UTC with `Z`**, second precision (`2026-10-27T21:14:03Z`); SQLite has no DATETIME class.

### 9.2 Schema (slide 18, constraints adopted)

```sql
CREATE TABLE IF NOT EXISTS EVENT_LOG (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,   -- = event_id in every MQTT message; ids only grow
    event_type  TEXT NOT NULL CHECK (event_type IN (
                    'ARMED','DISARMED','WRONG_PIN','PIN_CHANGED','PIN_LOCKOUT','ALARM_SILENCED',
                    'INTRUSION_ALARM','FIRE_ALARM','HAZARD_CLEARED','POWER_LOST','POWER_RESTORED',
                    'LOW_BATTERY','SHUTDOWN','FAULT')),
    timestamp   TEXT NOT NULL,                       -- ISO 8601 UTC
    details     TEXT NULL
);

CREATE TABLE IF NOT EXISTS PENDING_NOTIFICATION (     -- outbox: one row per ALERT
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    event_id    INTEGER NOT NULL UNIQUE REFERENCES EVENT_LOG(id),   -- 1 EVENT_LOG : 0..1 row
    message     TEXT NOT NULL,                       -- exact JSON payload published on vigil/home1/alert
    created_at  TEXT NOT NULL,
    status      TEXT NOT NULL CHECK (status IN ('PENDING','SENT')),
    sent_at     TEXT NULL
);
CREATE INDEX IF NOT EXISTS idx_pending_status ON PENDING_NOTIFICATION(status, id);

CREATE TABLE IF NOT EXISTS PIN_CONFIG (               -- single row, never published
    id          INTEGER PRIMARY KEY CHECK (id = 1),
    pin_hash    TEXT NOT NULL,                       -- pbkdf2_sha256$<iterations>$<salt>$<hash>
    updated_at  TEXT NOT NULL
);
```

**Why:** `id` orders events and drives the app sync, so it must never be reused (`AUTOINCREMENT`). The CHECK list makes SQLite reject an unknown type. `UNIQUE(event_id)` enforces the 1-to-0..1 relation (a retried insert cannot create a duplicate resend) and indexes `markSent`. `CHECK (id = 1)` guarantees a single PIN row, so a PIN change is always one `UPDATE`.

### 9.3 Statements

| Use | SQL |
|-----|-----|
| Log an event (+ outbox for alerts), one transaction | `BEGIN; INSERT INTO EVENT_LOG(event_type,timestamp,details) VALUES(?,?,?);` → `id = sqlite3_last_insert_rowid()` → build the JSON with `id` → `INSERT INTO PENDING_NOTIFICATION(event_id,message,created_at,status) VALUES(?,?,?,'PENDING'); COMMIT;` |
| Pending alerts, oldest first | `SELECT event_id, message FROM PENDING_NOTIFICATION WHERE status='PENDING' ORDER BY id LIMIT ?;` |
| Mark sent | `UPDATE PENDING_NOTIFICATION SET status='SENT', sent_at=? WHERE event_id=? AND status='PENDING';` |
| `get_events` (20 newest after `since`, oldest first) | `SELECT id,event_type,timestamp,details FROM (SELECT * FROM EVENT_LOG WHERE id > ? ORDER BY id DESC LIMIT 20) ORDER BY id ASC;` |
| Last power event (start-up) | `SELECT event_type FROM EVENT_LOG WHERE event_type IN ('POWER_LOST','POWER_RESTORED','SHUTDOWN') ORDER BY id DESC LIMIT 1;` |
| Load PIN | `SELECT pin_hash FROM PIN_CONFIG WHERE id = 1;` |
| Change PIN | `UPDATE PIN_CONFIG SET pin_hash = ?, updated_at = ? WHERE id = 1;` |
| Seed PIN (first boot / rebuilt DB) | `INSERT OR REPLACE INTO PIN_CONFIG(id,pin_hash,updated_at) VALUES(1,?,?);` |

Always use prepared statements with bound parameters.

### 9.4 PIN hash

- Format: `pbkdf2_sha256$<iterations>$<salt>$<hash>` — salt = 16 random bytes (`RAND_bytes`), hash = 32-byte PBKDF2-HMAC-SHA256 output, both Base64 without newlines. The salt is inside the string, so there is no extra column.
- Iterations: stored in the string, so it can change later without breaking old hashes. Start with **20 000** and **measure on the Pi 4** (no SHA crypto extensions); keep one verification at ≈ 20–50 ms because it runs in `AlarmControllerThread` (issue 21).
- Verify: parse the string → `PKCS5_PBKDF2_HMAC(pin, salt, iterations, EVP_sha256(), 32)` → `CRYPTO_memcmp` with the stored hash.
- **Why PBKDF2 and salt (D-39):** a plain hash of a 4–6 digit PIN is reversed instantly by anyone holding the SD card; the salt prevents precomputed tables. The online guessing protection is the lockout.
- **Default PIN:** `/etc/vigil/secrets/default_pin_hash` (root-only, mode 600), created with the small tool `pi/tools/vigil-pinhash <PIN>`. It is used only on the first boot and when both the database and the backup have no PIN (then with FAULT (DATABASE) and a required PIN change).

### 9.5 Backup and recovery (issues 11, 12)

- After a successful start-up check and after every PIN change, StorageThread writes `vigil.db.bak` with the SQLite online backup API (to a temp file, then `rename()`), so the backup always holds the current PIN.
- Start-up decision tree: §6.8 step 5. A corrupt file is kept as `vigil.db.corrupt-<time>` for analysis.
- Restoring from the backup loses the events written after it; that is accepted and written to syslog.
- Retention (optional, not required): SENT rows and old events may be pruned later; at a few KB per day it is not needed for the prototype.

---

## 10. MQTT protocol contract (Pi ⇄ broker ⇄ app)

The Pi and the app both code against this section. If `docs/protocol.md` is created for the app side, it must say exactly the same.

### 10.1 Broker, accounts and ACL

- **Production:** EMQX Serverless deployment `vigil-home1` (set up as in `cloud/README.md`), MQTT over TLS on **8883**, TLS 1.2/1.3 with the provider's certificate, spend limit 0. **Tests:** local Mosquitto (`cloud/mosquitto/`) with the same users, the same ACL and the same port, using our test CA. Only the broker configuration file changes between them.
- **Users:** `vigil-device` (the Pi; password in `/etc/vigil/secrets/mqtt_pass`, mode 600) and `vigil-app` (all phones; password in the app's config file). EMQX and Mosquitto use different passwords.
- **ACL (least privilege, D-20):**

| User | Publish | Subscribe |
|------|---------|-----------|
| `vigil-device` | `vigil/home1/status`, `alert`, `event`, `response`, `connection` | `vigil/home1/cmd` |
| `vigil-app` | `vigil/home1/cmd` | `vigil/home1/status`, `alert`, `event`, `response`, `connection` |
| All users | **deny `#`** (last rule — EMQX Serverless allows anything not matched) | **deny `#`** |

- **EMQX Serverless limits that shape the design:** 10 publishes/s per client (resend paced at ≤ 5/s); 1000 queued messages per offline client; at most 10 subscriptions per client (the app uses 5); 1 MB per message; free quota 1 M session minutes and 1 GB per month (≈ 13 % used by the Pi and two phones).

### 10.2 Connection parameters (slide 27)

| | Raspberry Pi | App (each phone) |
|---|---|---|
| `client_id` | `vigil-device-home1` | `vigil-app-XXXXXXXX` (8 random hex, generated at first launch, stored, reused) |
| Clean Start | 1 | 0 |
| Session Expiry Interval | 0 | the maximum EMQX accepts (request 604 800 s = 7 days; read the value back from CONNACK) |
| Keep-alive | 30 s | 60 s |
| Last Will | topic `connection`, `{"state":"offline"}`, QoS 1, **retained** | none |
| Subscriptions (QoS 1) | `cmd` | `status`, `connection`, `alert`, `event`, `response` |
| TLS | CA file, server certificate **and host name** verified; never fall back to plain MQTT | same |
| Reconnect | never stops: backoff 1 s → 60 s; `0x86`/`0x87` every 5 min (log once) | app's own policy |

| CONNACK / failure | Pi action |
|-------------------|-----------|
| `0x00` | connected: subscribe, `online`, status, pending alerts |
| `0x86` Bad user name or password, `0x87` Not authorized | retry every 5 min, log once to syslog |
| `0x88` Server unavailable, `0x9F` Connection rate exceeded | retry with backoff |
| TLS failure before CONNACK (wrong CA, expired, wrong host) | retry with backoff; usually the clock (NTP not synced yet) |
| No CONNACK (timeout) | retry with backoff |

Paho C++ (1.3.x) sketch — check the exact builder names against the version in Buildroot:

```cpp
mqtt::async_client cli("ssl://" + cfg.host + ":" + std::to_string(cfg.port), cfg.clientId,
                       mqtt::create_options(MQTTVERSION_5));        // no client-side persistence
auto ssl = mqtt::ssl_options_builder()
               .trust_store(cfg.caFile).enable_server_cert_auth(true).verify(true)   // verify = host name check
               .finalize();
auto opts = mqtt::connect_options_builder()
               .mqtt_version(MQTTVERSION_5).clean_start(true)
               .properties({ {mqtt::property::SESSION_EXPIRY_INTERVAL, 0} })
               .keep_alive_interval(std::chrono::seconds(cfg.keepaliveS))
               .user_name(cfg.username).password(readSecret(cfg.secretFile))
               .will(mqtt::message(topics::CONNECTION, R"({"state":"offline"})", 1, true))
               .ssl(ssl).automatic_reconnect(false)                  // we run our own backoff
               .finalize();
```

### 10.3 Topics

| Topic | Direction | QoS | Retained | Outbox | Content |
|-------|-----------|-----|----------|--------|---------|
| `vigil/home1/status` | Pi → app | 1 | **yes** | no (republished after each reconnect) | current system state |
| `vigil/home1/connection` | Pi → app | 1 | **yes** | no | `online`, clean `offline` with reason, or the Last Will |
| `vigil/home1/alert` | Pi → app | 1 | no | **yes** | alerts → phone notification |
| `vigil/home1/event` | Pi → app | 1 | no | no | log events (history only) |
| `vigil/home1/response` | Pi → app | 1 | no | no | command replies and `get_events` batches |
| `vigil/home1/cmd` | app → Pi | 1 | **never** | — | commands, Message Expiry 30 s; never stored |

### 10.4 Payloads

All payloads are compact UTF-8 JSON objects. Times are ISO 8601 UTC strings with `Z`. `event_id` is `EVENT_LOG.id`.

**Status** (retained):

```json
{"armed_state":"ARMED","fire_state":"NORMAL","power_source":"GRID","battery_pct":100,"timestamp":"2026-11-03T15:00:00Z"}
```

`armed_state` ∈ `DISARMED | ARMED | INTRUSION_ALARM`; `fire_state` ∈ `NORMAL | FIRE_ALARM | SILENCED`; `power_source` ∈ `GRID | BATTERY`; `battery_pct` ∈ 0..100 in steps of 10. Mapping from the FSMs: §6.11.

**Connection** (retained):

```json
{"state":"online","timestamp":"2026-11-03T15:00:05Z"}
{"state":"offline"}
{"state":"offline","reason":"LOW_BATTERY_SHUTDOWN","timestamp":"2026-11-03T19:42:10Z"}
```

The second line is the Last Will: it has **no timestamp** because its payload is fixed at CONNECT, so the app shows “Offline” without a time. The third is published before a planned shutdown and followed by DISCONNECT `0x00`, which makes the broker discard the Last Will.

**Alert** and **log event** (same shape, different topic):

```json
{"event_id":57,"event_type":"FIRE_ALARM","timestamp":"2026-11-03T15:01:00Z","details":"DETECTED"}
{"event_id":58,"event_type":"DISARMED","timestamp":"2026-11-03T15:02:10Z","details":"LOCAL"}
{"event_id":59,"event_type":"HAZARD_CLEARED","timestamp":"2026-11-03T15:12:10Z","details":null}
```

| Alerts (`alert`, outbox) | details |
|--------------------------|---------|
| `INTRUSION_ALARM` | `IR1` / `IR2` (default G4) |
| `FIRE_ALARM` | `DETECTED` / `REACTIVATED` |
| `ALARM_SILENCED` | `FIRE` |
| `HAZARD_CLEARED` | null |
| `POWER_LOST` | battery %, e.g. `"80"` (default G4) |
| `POWER_RESTORED` | null, or `BOOT` after a shutdown/outage reported earlier |
| `LOW_BATTERY` | battery %, e.g. `"30"` |
| `SHUTDOWN` | `LOW_BATTERY` |
| `FAULT` | `IR1` `IR2` `MQ2` `SPEAKER` `PWM_DRIVER` `DATABASE` |
| `PIN_LOCKOUT` | `LOCAL` / `REMOTE` |

| Log events (`event`, no outbox) | details |
|---------------------------------|---------|
| `ARMED` | `LOCAL` / `REMOTE` |
| `DISARMED` | `LOCAL` / `REMOTE` |
| `WRONG_PIN` | `LOCAL` / `REMOTE` |
| `PIN_CHANGED` | null |
| `ALARM_SILENCED` | `INTRUSION` |

**Commands** (`cmd`, app → Pi, QoS 1, not retained, Message Expiry 30 s):

```json
{"request_id":"vigil-app-1a2b3c4d-17","type":"arm"}
{"request_id":"vigil-app-1a2b3c4d-18","type":"disarm","pin":"1234"}
{"request_id":"vigil-app-1a2b3c4d-19","type":"change_pin","pin":"1234","new_pin":"5678"}
{"request_id":"vigil-app-1a2b3c4d-20","type":"get_events","since":42}
```

`request_id` = `<client_id>-<counter>`.

**Responses** (`response`, Pi → app):

```json
{"request_id":"vigil-app-1a2b3c4d-17","result":"OK"}
{"request_id":"vigil-app-1a2b3c4d-18","result":"LOCKED_OUT","detail":"4"}
{"request_id":"vigil-app-1a2b3c4d-20","result":"OK","events":[
  {"event_id":43,"event_type":"ARMED","timestamp":"2026-11-03T15:00:00Z","details":"REMOTE"},
  {"event_id":44,"event_type":"INTRUSION_ALARM","timestamp":"2026-11-03T15:01:30Z","details":"IR1"}]}
```

| Command | Possible `result` (checked in this order) |
|---------|-------------------------------------------|
| `arm` | `REJECTED_FIRE_ACTIVE` → `ALREADY_ARMED` → `OK` |
| `disarm` | `REJECTED_FIRE_ACTIVE` → `ALREADY_DISARMED` → `LOCKED_OUT` → `INVALID_PIN` / `OK` |
| `change_pin` | `LOCKED_OUT` → `INVALID_PIN` → `INVALID_NEW_PIN` → `SAVE_FAILED` / `OK` |
| `get_events` | `OK` + `events` (0–20 items, the newest with `event_id > since`, oldest first, one message) |

`detail` is optional (absent or null when unused). Defaults: `LOCKED_OUT` → remaining lockout minutes, rounded up (for “Try again in N min”); `INVALID_NEW_PIN` → `"4-6 digits"`; `SAVE_FAILED` → `"BUSY"` when another save is pending (G5, G13).

### 10.5 PUBACK handling

| Message | `0x00` / `0x10` (no matching subscribers = success) | `0x97` (quota exceeded) | No PUBACK (link lost) |
|---------|------------------|------------------|------------------|
| Alert | mark row SENT | back off and retry; row stays PENDING | row stays PENDING; resent after reconnection (SHUTDOWN: after the next boot) |
| Log event | done | back off and retry | not buffered; recovered by `get_events` |
| Status | done | back off and retry | republished on reconnect anyway |
| `online` | done | — | published again on reconnect |
| Clean `offline` | send DISCONNECT `0x00` | — | send DISCONNECT `0x00` anyway |

### 10.6 Validation of incoming commands (Pi)

- Ignore retained `cmd` messages and messages without a Message Expiry property.
- Payload must be a JSON object with a string `request_id` (1–64 characters) and a `type` in `arm | disarm | change_pin | get_events`. `disarm` needs a string `pin`; `change_pin` needs string `pin` and `new_pin`; `get_events` takes an integer `since` ≥ 0 (missing → 0). Unknown fields are ignored.
- Anything else → syslog, **no reply** (the app is ours, so this only happens with a bug or an attack).
- Never log `pin` or `new_pin`.

### 10.7 Ordering rules

- After every CONNACK: subscribe `cmd` → `online` (retained) → full status (retained) → pending alerts, oldest first, ≤ 5/s (slides 21, 28, 30). The app shows the current state before older alerts arrive.
- Remote command: response → log event → updated status (slide 29).
- New alert: outbox row → publish alert → updated status (slide 28).
- Both alarms silenced by one keypad PIN: DISARMED (`event`) → ALARM_SILENCED/INTRUSION (`event`) → ALARM_SILENCED/FIRE (`alert`) (slide 26).
- Alerts always leave in `event_id` order, live or resent.

### 10.8 Delivery guarantees and app sync

- **Outbox (D-12):** the Pi needs up to 60 s to notice a dead link and connects with Clean Start = 1, so anything published in that window would be lost; therefore every alert is stored before it is published and confirmed by PUBACK.
- **Phone offline:** the broker keeps up to 1000 QoS-1 messages per phone session. Publish only meaningful messages (no periodic status, no key digits) so that this covers weeks.
- **Sync:** after every CONNACK, and after every `online` it receives, the app publishes `get_events` with its last `event_id`; the Pi answers with the 20 newest newer events. Each phone keeps its own last id, so the Pi stores nothing per phone.
- **Duplicates:** QoS 1 and the sync can deliver the same event twice; the app drops any `event_id` it already has. Replies with another phone's `request_id` are ignored, except event batches, which are merged.

### 10.9 Security notes

TLS protects each hop separately (Pi ↔ broker, broker ↔ phone): the broker decrypts and therefore sees `cmd` payloads, including the PIN. This is an accepted trade-off; `cmd` is never retained or stored. A leaked app credential can only send commands (disarm still needs the PIN, which is rate-limited by the lockout); it cannot fake alerts, status or `online`, nor read other phones' commands. A leaked device credential requires physical access to the SD card. Per-phone credentials (to revoke a lost phone) were described but not adopted.

### 10.10 Detection bounds and timing

Broker loss detected by the Pi ≤ 60 s (2 × 30 s keep-alive) · Wi-Fi loss ≤ 2 s (link monitor) · the broker publishes the Last Will ≈ 45 s after the Pi goes silent (1.5 × keep-alive) · command Message Expiry 30 s · app command timeout 32 s (default: longer than the expiry, so “The command was not executed” is true) · resend ≤ 5 alerts/s · shutdown waits ≤ 5 s for the SHUTDOWN PUBACK.

### 10.11 Sample messages for `mosquitto_pub` (`cloud/samples/`)

Create one file per message: `status.json`, `connection-online.json`, `connection-offline-will.json`, `connection-offline-shutdown.json`, `alert-intrusion.json`, `alert-fire-detected.json`, `alert-fire-reactivated.json`, `alert-fire-silenced.json`, `alert-hazard-cleared.json`, `alert-power-lost.json`, `alert-power-restored.json`, `alert-low-battery.json`, `alert-shutdown.json`, `alert-fault.json`, `alert-pin-lockout.json`, `event-armed.json`, `event-disarmed.json`, `event-wrong-pin.json`, `event-pin-changed.json`, `event-intrusion-silenced.json`, `cmd-arm.json`, `cmd-disarm.json`, `cmd-change-pin.json`, `cmd-get-events.json`, `response-*.json`, `response-get-events.json`. Example:

```bash
source secrets/local.env
mosquitto_pub -V mqttv5 -h "$HOST" -p "$PORT" --cafile "$CAFILE" -u "$APP_USER" -P "$(cat "$APP_PASS_FILE")" \
  -i vigil-test-app1 -q 1 -t vigil/home1/cmd -D publish message-expiry-interval 30 -f cloud/samples/cmd-disarm.json
```

---

## 11. Configuration files and secrets

### 11.1 `/etc/vigil/broker.conf` (exists as `buildroot/overlay/etc/vigil/broker.conf.example`)

```ini
; Copy to broker.conf (ignored by git). Read by NetworkMQTTThread at start-up (S99vigil passes -c).
[broker]
host        = xxxxxxxx.ala.eu-central-1.emqxsl.com   ; or the laptop's IP for the local Mosquitto
port        = 8883
ca_file     = /etc/ssl/certs/ca-certificates.crt      ; EMQX (public CA); the test ca.crt for Mosquitto
client_id   = vigil-device-home1
username    = vigil-device
secret_file = /etc/vigil/secrets/mqtt_pass            ; chmod 600 — the password is never in this file
keepalive_s = 30
```

Switching EMQX ↔ Mosquitto is only a change of this file (the code is broker-agnostic). For Mosquitto, the server certificate must contain the laptop IP that the Pi uses (`FORCE=1 cloud/mosquitto/setup.sh <ip>`).

### 11.2 `/etc/vigil/vigil.conf` (new, optional; path overridable with `-C <file>`; built-in defaults when missing)

```ini
[timing]
exit_delay_s        = 60
entry_delay_s       = 30
silence_period_s    = 600
lockout_s           = 300
wrong_pin_limit     = 5
pin_entry_timeout_s = 10      ; default G3
mq2_warmup_s        = 60      ; to be measured (§15)
mq2_required_reads  = 5

[security]
pbkdf2_iterations   = 20000   ; measure on the Pi (§9.4)

[paths]
database            = /var/lib/vigil/vigil.db
default_pin_hash    = /etc/vigil/secrets/default_pin_hash

[network]
link_iface          = wlan0   ; interface watched for Wi-Fi loss (eth0 when testing on cable)
```

The delays and the silence period are “configurable in the device config file” (Decisions Record §8); they are not configurable from the app.

### 11.3 Secrets and permissions

| Secret | Location | Rule |
|--------|----------|------|
| Device MQTT password | `/etc/vigil/secrets/mqtt_pass` (from `buildroot/overlay/etc/vigil/secrets/`, git-ignored) | dir 700, file 600, root |
| Default PIN hash | `/etc/vigil/secrets/default_pin_hash` | 600, root; hash only, never the PIN |
| Wi-Fi | `buildroot/overlay/etc/wpa_supplicant.conf` (git-ignored; use `wpa_passphrase` to store a `psk=` hash) | — |
| EMQX / Mosquitto test passwords, EMQX CA, env files | `secrets/` at the repo root (git-ignored) | 600 |
| App broker credentials | `app/assets/config/broker.json` (git-ignored; template `broker.example.json` committed) | — |

Buildroot copies overlay files with their modes, but enforce them with a permission table (`BR2_ROOTFS_DEVICE_TABLE` extra file, e.g. `/etc/vigil/secrets d 700 0 0 - - - - -`, `/etc/vigil/secrets/mqtt_pass f 600 0 0 - - - - -`). Before every commit, check that no password, `.pem`, `.key`, `broker.conf` or `broker.json` appears in `git status`.

---

## 12. Buildroot and operating-system integration

### 12.1 Current base (`buildroot/README.md`, `buildroot/configs/`)

- Buildroot **2026.08** in `~/buildroot/buildroot-2026.08` (`~/workspace/buildroot/` is an identical copy). Base `raspberrypi4_64_defconfig`; full config saved in `buildroot/configs/vigil_rpi4_defconfig`; readable changes in `vigil_fragment.config`; root filesystem overlay `buildroot/overlay/`.
- Toolchain: Bootlin aarch64 glibc **stable** (GCC 14, kernel headers 5.4). Kernel: Raspberry Pi **6.12.61**, `bcm2711_defconfig`. Rootfs ext4, 256 MB.
- Packages already selected: OpenSSL, ca-certificates, Paho MQTT C/C++, wpa_supplicant (nl80211, CLI), brcmfmac firmware, chrony, SQLite, json-for-modern-cpp, libgpiod (v1) + tools, i2c-tools, Dropbear **and** OpenSSH (keep only Dropbear; they fight over port 22), Avahi, dhcpcd, kmod.
- Everyday commands (in the Buildroot tree): `make menuconfig` → `make savedefconfig` (writes into the repo) → `make -j$(nproc) 2>&1 | tee build.log`; restore with `make BR2_DEFCONFIG=/home/david/VigilSystem/buildroot/configs/vigil_rpi4_defconfig defconfig`; flash with `sudo dd if=output/images/sdcard.img of=/dev/sdX bs=4M conv=fsync status=progress` (check the device with `lsblk` first). Removing a package needs `make clean all`.

### 12.2 `config.txt` additions

Copy `board/raspberrypi4-64/config_4_64bit.txt` to `buildroot/board/config.txt`, add the lines below, and point `BR2_PACKAGE_RPI_FIRMWARE_CONFIG_FILE` to it.

```ini
# --- VigilSystem ---  (config.txt only accepts comments on their own line)
# UPS HAT INA219 on I2C-1 (0x42)
dtparam=i2c_arm=on
# our overlay: GPIO18 = PWM0 speaker, GPIO13 = PWM1 red LED (never load pwm-2chan as well)
dtoverlay=vigil-pwm
# keep analogue audio off (no dtparam=audio=on): it uses the PWM block
# Safe levels before Linux starts (firmware gpio= syntax: ip/op, pu/pd/pn, dh/dl)
# keypad rows, arm button, IR sensors (confirm the IR output type first): input + pull-up
gpio=17,22,23,27=ip,pu
gpio=6=ip,pu
gpio=16,20=ip,pu
# MQ-2 DO through the divider: no pull
gpio=21=ip,pn
# RGB channels drive BC337 low-side switches (D2): LOW = transistor off = LED off
gpio=4,19,26=op,dl
# red LED and speaker LOW until the PWM driver takes the pins
gpio=13,18=op,dl
```

Check that `/dev/watchdog` exists on the Pi (`bcm2835_wdt`); add `dtparam=watchdog=on` only if it does not.

### 12.3 Packaging the application, the module and the overlay (BR2_EXTERNAL)

So that `sdcard.img` contains everything, add a BR2_EXTERNAL tree:

```
buildroot/external/
├── external.desc                 name: VIGIL
├── external.mk                   include $(sort $(wildcard $(BR2_EXTERNAL_VIGIL_PATH)/package/*/*.mk))
├── Config.in                     source the two package Config.in files
├── package/vigil/                Config.in + vigil.mk      (cmake-package, local source ../../pi)
└── package/vigil-pwm/            Config.in + vigil-pwm.mk  (kernel-module, local source ../../driver)
```

```make
# package/vigil/vigil.mk
VIGIL_VERSION = 1.0
VIGIL_SITE = $(BR2_EXTERNAL_VIGIL_PATH)/../../pi
VIGIL_SITE_METHOD = local
VIGIL_DEPENDENCIES = paho-mqtt-cpp json-for-modern-cpp sqlite libgpiod openssl
VIGIL_CONF_OPTS = -DVIGIL_TARGET=ON -DVIGIL_BUILD_TESTS=OFF
$(eval $(cmake-package))

# package/vigil-pwm/vigil-pwm.mk
VIGIL_PWM_VERSION = 1.0
VIGIL_PWM_SITE = $(BR2_EXTERNAL_VIGIL_PATH)/../../driver
VIGIL_PWM_SITE_METHOD = local
VIGIL_PWM_DEPENDENCIES = rpi-firmware
define VIGIL_PWM_INSTALL_OVERLAY
	mkdir -p $(BINARIES_DIR)/rpi-firmware/overlays
	$(LINUX_DIR)/scripts/dtc/dtc -@ -I dts -O dtb \
		-o $(BINARIES_DIR)/rpi-firmware/overlays/vigil-pwm.dtbo $(@D)/dts/vigil-pwm-overlay.dts
endef
VIGIL_PWM_POST_INSTALL_TARGET_HOOKS += VIGIL_PWM_INSTALL_OVERLAY
$(eval $(kernel-module))
$(eval $(generic-package))
```

Use it with `make BR2_EXTERNAL=/home/david/VigilSystem/buildroot/external menuconfig`, enable both packages, `make savedefconfig`. Rebuild one part with `make vigil-rebuild` / `make vigil-pwm-rebuild`, then `make` to regenerate the image. (Check that the RPi genimage configuration copies `rpi-firmware/overlays` into the boot partition.)

### 12.4 libgpiod v1 on the target (decision to confirm)

`libgpiod2` needs kernel headers ≥ 5.10 and the Bootlin **stable** toolchain has 5.4, so the image has libgpiod **1.6.x**. Recommendation: **stay on v1** and keep it inside `GpioLines` (the class diagram's `gpiod_line_request*` becomes an implementation detail). Switching to the bleeding-edge toolchain for v2 means a full rebuild and a riskier toolchain for no functional gain. The laptop has v2, which is why the real `GpioLines` is only compiled for the target.

### 12.5 Boot sequence and init scripts

`S40network` (eth0) → **`S40wifi`** (modprobe brcmfmac, `wpa_supplicant -B -D nl80211 -i wlan0`) → `S41dhcpcd` → `S49chronyd` (`makestep 1 3`) → `S50dropbear` → **`S99vigil`**.

- `S99vigil` (update the existing script): this image has no automatic module loading (S40wifi already loads `brcmfmac` by hand), so before starting the daemon load `modprobe -q i2c-bcm2835; modprobe -q i2c-dev; modprobe -q pwm-bcm2835; modprobe -q vigil_pwm` (a built-in driver makes its `modprobe` a no-op), check that `/dev/i2c-1`, `/dev/vigil_pwm` and `/dev/watchdog` exist, then `start-stop-daemon -S -b -m -p /var/run/vigil.pid -x /usr/bin/vigil -- -c /etc/vigil/broker.conf`. It does not wait for NTP.
- Add `S01savedate` (no RTC, no fake-hwclock package): at boot, set the clock from `/var/lib/vigil/saved-date` if it is later than the current clock; save the date every 10 min and on stop. With it, TLS can succeed even before NTP, and timestamps are roughly right after a reboot without internet. Event order never depends on the clock (it uses `event_id`).
- NTP must be synced before the TLS handshake (certificate validity); until then the network thread retries with backoff.

### 12.6 Development workflow

```bash
# Laptop: unit tests + simulation (host libs: libpaho-mqtt-dev libpaho-mqttpp-dev nlohmann-json3-dev
#         libsqlite3-dev libssl-dev libgtest-dev cmake ninja-build)
cmake -S pi -B pi/build -G Ninja -DVIGIL_SIM=ON -DVIGIL_BUILD_TESTS=ON
cmake --build pi/build && ctest --test-dir pi/build --output-on-failure
cloud/mosquitto/run.sh &                        # local broker
# pi/config/broker.conf (git-ignored): host = localhost, ca_file = cloud/mosquitto/certs/ca.crt,
#   secret_file = secrets/local-vigil-device.pass; pi/config/vigil.conf: database = ./vigil.db
pi/build/vigil -c pi/config/broker.conf -C pi/config/vigil.conf   # simulated hardware console + real MQTT

# Cross-compile for the Pi with the Buildroot toolchain
BR=$HOME/buildroot/buildroot-2026.08
cmake -S pi -B pi/build-rpi -G Ninja -DCMAKE_TOOLCHAIN_FILE=$BR/output/host/share/buildroot/toolchainfile.cmake -DVIGIL_TARGET=ON
cmake --build pi/build-rpi
scp pi/build-rpi/vigil root@vigil.local:/usr/bin/vigil && ssh root@vigil.local /etc/init.d/S99vigil restart

# Driver
make -C driver && scp driver/vigil_pwm.ko root@vigil.local:/root/
```

`vigil.local` works through Avahi; otherwise use the Pi's IP. The boot partition is not mounted by default: `mount /dev/mmcblk0p1 /boot` to copy an overlay or edit `config.txt` on the Pi.

### 12.7 Other OS items

- Change the root password (`BR2_TARGET_GENERIC_ROOT_PASSWD="root"`) before the demo: this is a security product.
- `country=PT` in `wpa_supplicant.conf`.
- For the real-time measurements add `rt-tests` (cyclictest) and `stress-ng` to the image; PREEMPT_RT is optional (mainline in 6.12) and only worth trying if cyclictest shows a problem.

---

## 13. Mobile app (Flutter, Android)

### 13.1 Scope (D-22, D-23)

A **thin remote control**: the Pi validates the PIN and holds the state and the history; the app sends commands on `cmd`, shows the replies from `response`, and never validates anything itself. Flutter, Android first, portrait, one phone, **one screen**, Material Design 3 (Roboto, Material Symbols Rounded), light theme only. No login screen (there are no user accounts because there is no backend; disarm and PIN change need the PIN and the ACL limits the app credential).

Out of scope: iOS, Firebase push, a full history screen, configuring the delays/silence period from the app, multiple homes or users.

**Why a foreground service instead of push:** the broker only delivers to an app with an open MQTT connection. An Android foreground service keeps it open even when the app is closed, so the design stays broker-only (no Firebase project, keys or extra cloud component). Firebase could be added later only as a wake-up signal.

### 13.2 Structure

| Layer | Responsibility |
|-------|----------------|
| `MqttService` (inside the foreground-service isolate) | TLS connection with the persistent session (§10.2), QoS 1 subscriptions, publishing `cmd` with Message Expiry 30 s, `get_events` after every CONNACK and every `online` |
| `AppState` (`ChangeNotifier`) | status, connection, alerts, the pending command |
| `AlertStore` (`shared_preferences`) | `client_id`, `recent_events` (≤ 20, ordered by `event_id`), `last_event_id` |
| `NotificationService` | heads-up alerts, reconnection summary group, permanent monitoring notification |
| UI widgets | `StatusCard`, `ActionRow`, `AlertList`, dialogs; colours defined once in a theme extension |

`flutter_foreground_task` runs its task in a separate Dart isolate: the MQTT connection lives in the service and the UI talks to it through the plugin's task ↔ UI messaging, not by calling `MqttService` directly. Read the plugin's example before writing the MQTT layer.

Packages: `mqtt5_client`, `flutter_foreground_task`, `flutter_local_notifications`, `shared_preferences`. Create the project with `flutter create --org pt.uminho.vigil --platforms=android,linux app`. Broker settings in `assets/config/broker.json` (git-ignored; host, port, user, password, optional CA), template `broker.example.json` committed.

Android: `POST_NOTIFICATIONS` (13+); foreground-service type **`specialUse`** and its permission (14+), to avoid the Android 15 `dataSync` 6 h limit; start at boot; ask for the battery-optimisation exemption; a permanent, silent, low-priority notification “VigilSystem – monitoring” / “Connected to your home system”. To verify during implementation: that `mqtt5_client` exposes the session expiry returned in CONNACK (needed for the expiry-cap test) and that the plugin allows `specialUse`.

### 13.3 Status card (priority: fire > intrusion > fire silenced > armed/disarmed)

| Condition | Card | Colour | Icon | Text |
|-----------|------|--------|------|------|
| `connection` = offline | Device offline | amber | `cloud_off` | Alarm and Power rows show the last known values in grey with “(last known)”; Device row “Offline” (no time), or “Shut down – low battery” when `reason` = `LOW_BATTERY_SHUTDOWN` |
| `fire_state` = FIRE_ALARM | FIRE / GAS ALARM | red | `local_fire_department` | “Smoke or gas detected at HH:MM. Silence the alarm at the keypad (this also disarms the system). Remote disarm is not allowed during a fire alarm.” |
| `armed_state` = INTRUSION_ALARM | INTRUSION ALARM | red, icon pulses slowly | `directions_run` | “Motion detected at HH:MM” |
| `fire_state` = SILENCED | Fire alarm silenced | amber | `notifications_off` | silence period active |
| `armed_state` = ARMED | Armed | green | `shield` / `lock` | Alarm “None” |
| otherwise | Disarmed | grey | `lock_open` | Alarm “None” |

Rows: Alarm, Power (“Mains” with a green plug, or “Battery – NN%” in amber), Device (Online/Offline), plus “Updated HH:MM” from the status timestamp. Light-tint background with a 4 dp left border in the state colour; 200 ms transition on change.

### 13.4 Buttons

| State | Arm | Disarm | Change PIN |
|-------|-----|--------|------------|
| Disarmed (also with the fire alarm silenced) | enabled | disabled | enabled |
| Armed | disabled | enabled (tonal) | enabled |
| Intrusion alarm | disabled | enabled, **filled red** | enabled |
| Fire alarm (alone or with intrusion) | disabled | enabled (tonal) — the Pi rejects it | enabled |
| Device offline | disabled | disabled | disabled, helper text “Commands are unavailable while the device is offline.” |

### 13.5 Commands, dialogs and replies

Every command: `request_id` = `<client_id>-<counter>`, publish on `cmd` (QoS 1, Message Expiry 30 s), show “Waiting for device…” with a progress indicator, wait for the reply with the same `request_id` for **32 s**.

| Command | Dialog | Reply → UI |
|---------|--------|-----------|
| Arm | “Arm the system?” – “The system arms immediately, with no exit delay.” (Cancel / Arm) | `OK` → snackbar “System armed” · `ALREADY_ARMED` → dialog closes, snackbar “The system is already armed” · `REJECTED_FIRE_ACTIVE` → alert dialog “Arm rejected – a fire alarm is active” (default text) |
| Disarm | “Disarm system”, one masked numeric PIN field with show/hide (Cancel / Disarm) | `OK` → closes, snackbar “System disarmed” · `INVALID_PIN` → stays open, field error “Wrong PIN. Try again.” · `REJECTED_FIRE_ACTIVE` → alert dialog with a red fire icon “Disarm rejected” – “A fire alarm is active. Silence it at the keypad.” · `ALREADY_DISARMED` → closes, snackbar “The system is already disarmed” · `LOCKED_OUT` → dialog error “Too many wrong PINs. Try again in N min.” (N from `detail`) |
| Change PIN | “Change PIN”, masked fields Current PIN and New PIN (Cancel / Save) | `OK` → snackbar “PIN changed” · `INVALID_PIN` → error on Current PIN “Current PIN is wrong.” · `INVALID_NEW_PIN` → error on New PIN “Use 4 to 6 digits.” (default text) · `SAVE_FAILED` → dialog-level error with an icon “The device could not save the new PIN. Try again.” (no field marked) · `LOCKED_OUT` → as above |
| any | — | no reply in 32 s → “No response from the device. The command was not executed.” |

`ALREADY_ARMED` / `ALREADY_DISARMED` are not errors (the app view can be stale); the card is then updated from the retained status.

### 13.6 Recent alerts list and notifications

At most **20** items, ordered by `event_id`, newest first, grouped “Today” / “Yesterday”, scrolling independently; the oldest item is dropped when a new one arrives; duplicates (same `event_id`) are discarded.

| event_type (details) | Title | Detail | Icon | Colour | Phone notification |
|----------------------|-------|--------|------|--------|--------------------|
| INTRUSION_ALARM | Intrusion alarm | Motion detected at home | `directions_run` | red | “Intrusion alarm” – “Motion detected at home. Tap to open.” |
| FIRE_ALARM (DETECTED) | Fire / gas alarm | Smoke detected by the MQ-2 sensor | `local_fire_department` | red | “Fire / gas alarm” – “Smoke or gas detected at home.” |
| FIRE_ALARM (REACTIVATED) | Fire / gas still detected | Smoke persists after the silence period | `local_fire_department` | red | critical: “Fire / gas still detected” – “The fire alarm has been reactivated.” |
| ALARM_SILENCED (FIRE) | Fire alarm silenced | Alarm silenced at the keypad | `notifications_off` | amber | yes |
| ALARM_SILENCED (INTRUSION) | Intrusion alarm silenced | Alarm silenced at the keypad | `notifications_off` | grey | no (log event) |
| HAZARD_CLEARED | Hazard cleared | No smoke detected after the silence period | `check_circle` | green | “Hazard cleared” – “No more smoke or gas detected.” |
| POWER_LOST | Power outage | Running on battery (NN%) | `battery_alert` | amber | “Power outage” – “The system is running on battery (NN%).” |
| POWER_RESTORED | Power restored | Back on mains power | `power` | green | “Power restored” – “The system is back on mains power.” |
| LOW_BATTERY | Low battery | Battery at NN% – power not restored | `battery_alert` | amber | yes |
| SHUTDOWN | System shut down | Battery empty – the device switched off safely | `power_settings_new` | red | yes |
| FAULT (component) | Device fault | e.g. “Speaker not responding – check the device” | `error` | red | yes |
| PIN_LOCKOUT | PIN entry locked | Too many wrong PINs – locked for 5 min | `lock_clock` | amber | yes |
| ARMED | System armed | Armed at the arm button / Armed remotely from the app | `lock` | grey | no |
| DISARMED | System disarmed | Disarmed at the keypad / Disarmed remotely from the app | `lock_open` | grey | no |
| WRONG_PIN | Wrong PIN | Incorrect PIN entered at the keypad / in the app | `key` | amber | no |
| PIN_CHANGED | PIN changed | PIN updated from the app | `key` | grey | no |

Only messages on `alert` create phone notifications: heads-up, high-priority channel, coloured accent, tapping opens the app at the top of the list. When several alerts arrive right after an `online` (resend after reconnection), show **one** summary “N alerts received” – “Events detected while the device was offline.” (red accent and “Including: …” when the batch has an intrusion or fire alarm), implemented as an Android notification group with a summary.

### 13.7 Theme tokens

| Token | Main | Light tint | Used for |
|-------|------|------------|----------|
| Armed / OK | `#2E7D32` | `#E8F5E9` | armed, mains, online, restored/cleared |
| Disarmed / neutral | `#5F6368` | `#F1F3F4` | disarmed, arm/disarm/PIN events |
| Warning | `#B26A00` | `#FFF4E0` | battery, offline, outage, wrong PIN, fire silenced |
| Alarm | `#C62828` | `#FDECEA` | intrusion and fire alarms |
| Primary | `#1A4D8F` | — | Arm button, dialog buttons |
| Background | `#FAFAFA` | — | screen (cards white) |

Layout: 16 dp side margins, 8 dp grid, card radius 16 dp, dialog radius 28 dp, fully rounded buttons 48 dp high, minimum touch target 48 dp, elevation ≤ 1 except dialogs; reference frame 412 × 917 dp. Colour is always paired with an icon and a label.

### 13.8 Milestones

1. Static UI with fake data and a debug switch through all states (develop on the Linux desktop target first: `flutter run -d linux`).
2. TLS connection to the local Mosquitto (test CA); `mosquitto_pub` plays the Pi (retained status, alerts).
3. Arm / Disarm / Change PIN with the waiting state, the 32 s timeout and every reply text.
4. Foreground service and notifications; tests: alert with the app closed, after a phone reboot, after more than 24 h.
5. `get_events` sync, duplicate discarding, 20-item limit, reconnection summary.
6. Switch to EMQX through the config file; repeat connection, ACL rejection, end-to-end latency and ordered delivery.

---

## 14. Error handling and fault model (slide 59)

| Area | Error | Detection | Response |
|------|-------|-----------|----------|
| Network | Wi-Fi or broker lost | netlink link down (≤ 2 s); keep-alive / socket error (≤ 60 s) | reconnect with backoff; RGB yellow/magenta; alerts stay PENDING; local alarm unaffected |
| Security | TLS or authentication failure | handshake error; CONNACK `0x86`/`0x87` | refuse the connection, **never fall back to plain MQTT**, retry (backoff / every 5 min), log once |
| Power | grid failure, low battery | INA219 current sign and voltage, 3 samples | run on battery, POWER_LOST/LOW_BATTERY alerts, clean shutdown at ≤ 6.4 V |
| Storage | database failure | open/`quick_check`/write error | alarm logic and PIN stay in RAM; backup or rebuilt database; FAULT (DATABASE) |
| Outputs | speaker or PWM driver fails | `write()`/`open()` error on `/dev/vigil_pwm` | retry; else RGB fault blink + FAULT (SPEAKER / PWM_DRIVER) |
| Sensors | IR or MQ-2 stuck, GPIO fails | line request/read error (plausibility checks optional) | mark the sensor faulty, ignore it, FAULT (IR1 / IR2 / MQ2) |
| Keypad | 5 consecutive wrong PINs | shared counter | 5 min lockout, PIN_LOCKOUT alert |
| System | thread hang | missed heartbeat | Supervisor restarts the thread; otherwise the watchdog resets the Pi |
| Start-up | missing device, line or database | return codes in `main` | degraded start (§6.8): never refuse to start the alarm because one part failed |
| Clock | not synced after boot | TLS certificate “not yet valid” | network keeps retrying; local alarm runs; saved-date script reduces the gap |

A FAULT alert is sent **once per component** (per boot). All errors are also written to syslog (`logger`/`syslog()`), which is volatile in this image (`/var/log` is in RAM).

---

## 15. Parameters (single table)

“Decided” values come from the slides/Decisions Record; “default” values fill a gap and can be changed by the team (§18).

| Parameter | Value | Status |
|-----------|-------|--------|
| Exit delay / entry delay / fire silence period | 60 s / 30 s / 10 min (config file) | decided |
| Wrong-PIN limit / lockout | 5 / 5 min | decided |
| PIN length | 4–6 digits | decided |
| Keypad scan / debounce | 15 ms (10–20 ms) / 2 stable reads | decided |
| Button debounce | 20–50 ms (2 scans = 30 ms) | decided |
| PIN entry timeout | 10 s | default |
| MQ-2 sampling / consecutive reads | 100 ms / 5 | default |
| MQ-2 warm-up | 60 s | default — measure |
| IR re-trigger guard | 200 ms | default |
| INA219 polling / hysteresis | 1 s / 3 samples | decided |
| Mains lost / back thresholds | current < −100 mA / > +50 mA | proposed — validate on the board |
| Battery estimate | 6.0 V = 0 %, 8.4 V = 100 %, linear, 10 % steps | vendor mapping |
| Low battery / shutdown | ≤ 30 % (≈ 6.7 V) / ≤ 6.4 V, 3 samples | decided |
| MQTT keep-alive Pi / app | 30 s / 60 s | decided |
| Last Will published by the broker | ≈ 45 s (1.5 × keep-alive) | decided |
| Pi broker-loss / Wi-Fi-loss detection | ≤ 60 s / ≤ 2 s | decided |
| Reconnect backoff / after auth error | 1 s → 60 s / every 5 min, never stop | decided |
| `cmd` Message Expiry | 30 s | decided |
| App command timeout | 32 s | default |
| Session expiry Pi / app | 0 / maximum accepted (request 7 days) | decided |
| Outbox resend pacing | ≤ 5 alerts/s | decided |
| Shutdown wait for PUBACK | 5 s | decided |
| `get_events` reply / app list | 20 newest / 20 items | decided |
| Heartbeat timeouts | 1–10 s per thread (§5.2), checked every 1 s | default |
| Watchdog timeout | 15 s (bcm2835 maximum) | default |
| PBKDF2-SHA256 iterations | 20 000, target 20–50 ms per hash on the Pi | default — measure |
| Latency targets | sensor → speaker ≤ 1 s; key → beep ≤ 100 ms; alarm → app ≤ 5 s | decided (slide 58) |
| Autonomy | ≥ 3 h measured (≈ 4 h theoretical) | decided (slide 54) |

---

## 16. Testing and verification

### 16.1 Levels

| Level | Where | What |
|-------|-------|------|
| L1 unit | laptop, `ctest` | `AlarmLogic` (every rule in §6), `PinHasher`, `Protocol`, `Database` on a temp file, queues, output/status mapping |
| L2 integration | laptop, simulation build + local Mosquitto | complete flows with `mosquitto_pub`/`mosquitto_sub` and the app; `cloud/test-broker.sh secrets/local.env` (13 broker checks) |
| L3 hardware-in-the-loop | Raspberry Pi | driver, sensors, outputs, power, latency, 24 h run |
| L4 cloud | Pi + EMQX | connection, ACL, end-to-end latency, ordered delivery; broker loss simulated with an `iptables` rule dropping the broker's address |

### 16.2 Unit scenarios for `AlarmLogic` (minimum set)

1. Start-up → DISARMED, NORMAL, NET_LOST, no DISARMED event; power state and boot alerts per P-1/P-2 (incl. last event SHUTDOWN → POWER_RESTORED).
2. Button → EXIT_DELAY (beep, LED on, status still DISARMED) → timeout → ARMED + ARMED(LOCAL) + status ARMED.
3. Button during FIRE_ALARM → rejected; during SILENCED → allowed.
4. Remote Arm in DISARMED / EXIT_DELAY (cancels the timer) / ARMED / ENTRY_DELAY / INTRUSION_ALARM / FIRE_ALARM / SILENCED → OK / OK / ALREADY_ARMED ×3 / REJECTED_FIRE_ACTIVE / OK.
5. Motion in DISARMED and EXIT_DELAY ignored; in ARMED → ENTRY_DELAY with WARNING, no notification.
6. Entry timeout → INTRUSION_ALARM + alert (details IR1/IR2) + flashing LED.
7. Keypad PIN in EXIT_DELAY / ENTRY_DELAY / INTRUSION_ALARM → DISARMED (+ ALARM_SILENCED INTRUSION only from INTRUSION_ALARM).
8. Keypad PIN while already DISARMED with no alarm → no event (issue 18).
9. Smoke in every operational state → FIRE_ALARM alert DETECTED, fire tone has priority over intrusion.
10. Keypad PIN during fire alarm (armed) → SILENCED, DISARMED, events in the order DISARMED → ALARM_SILENCED(FIRE); with intrusion also active → DISARMED → ALARM_SILENCED(INTRUSION) → ALARM_SILENCED(FIRE).
11. Smoke during SILENCED ignored; silence timeout with smoke → FIRE_ALARM REACTIVATED (operational state unchanged); without smoke → NORMAL + HAZARD_CLEARED.
12. Remote Disarm order: fire → REJECTED_FIRE_ACTIVE (no WRONG_PIN, counter unchanged); disarmed → ALREADY_DISARMED; lockout → LOCKED_OUT with remaining minutes; wrong → INVALID_PIN + WRONG_PIN(REMOTE); right → OK, DISARMED(REMOTE), timers stopped.
13. Lockout: 5 wrong PINs mixing keypad and app → lockout + PIN_LOCKOUT(details = source of the 5th); during lockout nothing is hashed (keypad → error beep, remote → LOCKED_OUT), even during a fire alarm; counter reset by a valid PIN and at lockout end.
14. Change PIN: LOCKED_OUT; INVALID_PIN (counts); INVALID_NEW_PIN for 3 and 7 digits and non-digits; save OK → new hash active only after `EVT_PIN_SAVE_RESULT`; save failed → old PIN still works; second change while pending → SAVE_FAILED.
15. Power: lost → POWER_LOST(pct) + battery colour + status BATTERY; low → one LOW_BATTERY per outage; critical from LOW_BATTERY and from BATTERY_MODE → shutdown actions; restored → POWER_RESTORED + status GRID; battery level change → status only.
16. Network events change only the RGB colour; status mapping per §6.11 (exit delay → DISARMED, entry delay → ARMED).
17. Output arbitration: beeps dropped during FIRE/INTRUSION, played over WARNING; LED blink dropped while flashing.
18. FAULT sent once per component; RGB blink while any fault is active.

### 16.3 Test tables in the deck (slides 49–58) — all must pass

- **Local interface (49):** button press (event + beep), debounce (single event), keypad scan (12 keys, no ghosts), digit entry (beep per digit), correct PIN (disarmed), wrong PIN (error beep, state unchanged).
- **Sensors (50):** IR1 / IR2 detection when armed (event with sensor id), ignored when disarmed, ignored in exit delay, smoke on DO transition, smoke in any state → fire alarm, one event while smoke persists, smoke cleared (DO back to normal), no false alarm during warm-up.
- **Alarm logic (51–52):** start-up (DISARMED, PIN loaded, RGB set), exit delay, disarm in exit delay, entry delay (warning tone), disarm in entry delay, intrusion alarm (tone, flashing, notification), alarm persists until disarmed, distinct tones, fire alarm, local fire silence (silence period, disarmed), arming allowed during silence, timeout with smoke (reactivated + critical notification), timeout without smoke, remote arm (immediate, OK), remote disarm of intrusion, remote disarm rejected during fire, intrusion + fire (independent FSMs, fire tone plays).
- **Alarm output (53):** speaker tones at the set frequency, pattern steps on a scope, red LED off/on/flashing, RGB colour for every power × network combination, driver valid command, invalid arguments rejected, concurrent access serialised, long tone does not block the controller.
- **Power (54):** INA219 matches a multimeter, grid failure (POWER_LOST, RGB, notification), seamless switch (no reset), restoration, charging current positive, hysteresis (one transition), autonomy ≥ 3 h, low battery → safe shutdown with the database intact.
- **Communication (55–56):** TLS connect + `online`, invalid credentials (retry every 5 min, never stop), wrong CA/expired → refused, Last Will after 1.5 × keep-alive, retained status for a new subscriber, alert notification with the app closed, remote disarm wrong PIN, PIN change (works after restart), PIN change wrong PIN, broker loss with Wi-Fi up (RGB, alerts stored), Wi-Fi loss (RGB, local functions work), pending delivery once and in order after PUBACK, resend pacing ≤ 5/s with a ~50-alert backlog, stale command (30 s expiry) not executed, event sync (20 newest, one message), ACL enforcement.
- **Data storage (57):** PIN persistence across restart, only a salted hash stored, pending rows survive a restart and are delivered, database failure (alarm works, PIN from memory), history query (20 newest after `since`, in order).
- **Real time (58):** local alarm latency ≤ 1 s, notification latency ≤ 5 s, key → beep ≤ 100 ms, periodic tasks stable (keypad 10–20 ms, INA219 ≈ 1 Hz), fire alarm ≤ 1 s under CPU stress + DB writes + backlog, no deadlock/race/lost event with simultaneous events, 24 h run without crash or memory growth.

### 16.4 Test rows still to add to the deck (Decisions Record §8, ≈ 35)

Local interface: lockout, PIN during lockout, lockout end, arm button during a fire alarm · Sensors: sensor fault · Alarm logic: remote arm in exit delay / in fire alarm / when armed, remote disarm when disarmed, fire silence while armed, fire + intrusion with one PIN, smoke during silence, lockout during fire · Output: PWM driver failure, speaker idle level · Power: low-battery alert, critical shutdown, boot after shutdown · Communication: locked out, invalid new PIN, save failed, undetected link loss, reconnection order, clean shutdown message, two phones, unique client ID, fire-silenced notification · Data storage: outbox row, database restore, no database and no backup, not logged · Real time: broker-loss and Wi-Fi-loss detection times. Corrections: slide 57 Event Logging must not mention connectivity, and History Query must say “20 newest”.

### 16.5 How to measure the real-time figures

- Stamp `Event::monoNs` at detection and log the time when the matching output command reaches `OutputThread` (and when the PUBACK arrives) → latency histograms.
- For end-to-end figures toggle a spare GPIO (GPIO7–12) at detection and watch it and the speaker line on a logic analyser/scope.
- Baseline scheduling latency with `cyclictest -m -p 80 -t4 -i 1000`; load with `stress-ng --cpu 4 --io 2 --vm 1` while triggering smoke; record worst cases.
- 24 h run: watch `VmRSS` of `vigil` (`/proc/<pid>/status`) and the outbox size.

---

## 17. Decision log (what was decided and why)

| ID | Decision | Why |
|----|----------|-----|
| D-01 | Event-queue architecture is the only architecture; `VigilSystemController`, `SensorMonitorThread`, `DelayTimer`, `AlarmActuator` dropped | Two architectures in one deck contradicted each other; one owner of all logic is deterministic and testable |
| D-02 | One Event Queue (all producers → controller) and one `CommandQueue<T>` per consumer; timer expiries and PIN results are raised inside the controller | The controller waits on one epoll set and never blocks on I/O; producers and consumers stay simple |
| D-03 | Nine threads: LocalInput, IRSensors, MQ2Sensor, PowerMonitor, AlarmController, Output, Storage, NetworkMQTT, Supervisor | Each has one job (SRP); the hardware threads isolate their timing |
| D-04 | Abstract `WorkerThread` for the 8 workers; `SupervisorThread` is not a worker and owns `/dev/watchdog` | One type to monitor, one place for heartbeats; the supervisor must not monitor itself, the watchdog covers it |
| D-05 | FIFO 80 MQ2/Controller/Output, 70 IR, 50 LocalInput/Power, OTHER Storage/Network/Supervisor | The fire path is as fast as its slowest stage; a starved supervisor stops feeding the watchdog |
| D-06 | Exactly one custom driver: PWM platform driver on `pwm-bcm2835`, `/dev/vigil_pwm`, 2 channels, hrtimer patterns, mutex, error codes | Course needs ≥ 1 driver; PWM is the only device whose timing benefits from kernel code |
| D-07 | Our own overlay claims GPIO18/GPIO13; `pwm-2chan` never loaded with it | Avoid pin conflicts (default `pwm-2chan` puts PWM1 on the RGB blue pin) |
| D-08 | If the PWM driver fails: RGB LED + FAULT alert | The red LED depends on the same driver |
| D-09 | libgpiod for keypad, button, IR, MQ-2, RGB; i2c-dev for the INA219 | Mainline interfaces; no driver risk where none is needed |
| D-10 | Broker only (EMQX Serverless); no backend, REST API, cloud DB or push service | No subscription, nothing extra to secure, the PIN never leaves the device in stored form; the Pi is the source of truth |
| D-11 | EMQX Serverless over HiveMQ Cloud, AWS IoT Core and a self-hosted Mosquitto | Only free option with per-user/client ACL rules and X.509 support; ≈ 13 % of the free quota; MQTT 5; no server to run |
| D-12 | Outbox row for every alert before publishing; SENT only after PUBACK; resend oldest first ≤ 5/s | Up to 60 s to notice a dead link + Clean Start = 1 would lose alerts; 5/s stays under EMQX's 10/s |
| D-13 | Log events are not buffered; recovered with `get_events` | Keeps the outbox small; the app can ask for history |
| D-14 | `get_events` → 20 newest with id > since, oldest first, one message, no paging | The app keeps only 20 |
| D-15 | Reconnection order: `online` → full status → pending alerts | The app shows the current state before older alerts |
| D-16 | App client ID `vigil-app-<8 hex>` generated once; `request_id` = `<client_id>-<counter>`; replies for other phones ignored (event batches merged) | Two phones with one client ID would kick each other off; all phones share `response` |
| D-17 | The Pi never stops reconnecting; `0x86`/`0x87` every 5 min, log once; other errors backoff 1 → 60 s | A stopped Pi would stay offline after the cause is fixed elsewhere (e.g. password changed in the console) |
| D-18 | Detection bounds: broker loss ≤ 60 s, Wi-Fi loss ≤ 2 s, Last Will after ≈ 45 s | Gives the connectivity requirement a number |
| D-19 | TLS 1.2/1.3 on each hop with certificate and host-name checks; the broker can see `cmd` payloads (accepted); `cmd` never retained/stored | Stated openly so nobody assumes end-to-end encryption |
| D-20 | Users `vigil-device` / `vigil-app`, least-privilege ACL + deny-all; no login screen; per-phone users not adopted | Leaked app credential can only send commands; no accounts without a backend |
| D-21 | `cmd` Message Expiry 30 s; Pi clean session (expiry 0) | A stale disarm must never run later |
| D-22 | Android foreground service holds the MQTT connection | Keeps the system broker-only; no Firebase |
| D-23 | Flutter, Android first, one screen, Material 3, app never validates | Small app; the course assesses the embedded system |
| D-24 | Button → 60 s exit delay; Remote Arm immediate (cancels exit delay); both rejected during a fire alarm, allowed during the silence period | Nobody should arm while a fire alarm sounds; the silence period is no longer an alarm |
| D-25 | IR ignored when disarmed and in the exit delay; armed → 30 s entry delay with warning tone and no notification; timeout → intrusion alarm + alert | Lets the owner enter and disarm |
| D-26 | Fire monitored 24/7; the alarm stays until silenced; no automatic return to NORMAL | Fail-loud |
| D-27 | A valid keypad PIN during a fire alarm stops the tone, starts the 10 min silence period and leaves the system DISARMED | People inside looking for the cause would trigger the IR sensors |
| D-28 | Silence timeout: no smoke → NORMAL + HAZARD_CLEARED; smoke → FIRE_ALARM (REACTIVATED); smoke ignored during the period | Analysis requirement |
| D-29 | Priority fire > intrusion > fire silenced > none; one valid keypad PIN silences both | One speaker and one status card can show one alarm |
| D-30 | Remote Disarm order: fire → already disarmed → lockout → PIN; first three never test the PIN | A fire alarm can only be silenced on site; never test a PIN when the answer is known |
| D-31 | Remote Arm order: fire → already armed → OK; no PIN | The app view can be stale, so the Pi always answers; a PIN for arming was declined (nuisance, not a breach) |
| D-32 | Shared wrong-PIN counter, 5 → 5 min lockout + alert; nothing checked during the lockout | 10 000 combinations at 10 msg/s took ≈ 17 min; a shared counter stops alternating sources |
| D-33 | Low battery ≤ 30 % (≈ 6.7 V, 3 samples) → one alert per outage | Warning before the shutdown |
| D-34 | ≤ 6.4 V → SHUTDOWN alert (≤ 5 s PUBACK wait), retained offline with reason, DISCONNECT 0x00, sync SQLite, power off; next boot on mains → POWER_RESTORED | Protects the SD card and tells the owner why the device is offline |
| D-35 | One FAULT alert type with the component in `details`, sent once per fault | Error handling says “notify”, which needed a message |
| D-36 | Database failure at start-up: backup first; only if it has no PIN, default PIN from a root-only file + FAULT (DATABASE) + PIN change required | A default PIN is a security risk, so it is the last resort |
| D-37 | EVENT_LOG: CHECK list of 14 types, ISO 8601 UTC text, `id` = `event_id`; key digits and connectivity changes not logged | `id` orders events and drives the sync |
| D-38 | PENDING_NOTIFICATION with `UNIQUE(event_id)` | Enforces 1 : 0..1 and makes `markSent` fast |
| D-39 | PIN_CONFIG single row (`CHECK id = 1`), `pbkdf2_sha256$<iterations>$<salt>$<hash>`, never published | A plain hash of a short PIN is reversed instantly from the SD card |
| D-40 | Topic tree `vigil/home1/{status, connection, alert, event, response, cmd}`; status and connection retained | Retained state for new subscribers; one home per tree |
| D-41 | Fire silenced is an **alert** (owner notified); intrusion silenced is a log event | The owner must know a fire alarm was silenced |
| D-42 | Status fields `armed_state`, `fire_state`, `power_source`, `battery_pct` (10 % steps), `timestamp`; exit delay = DISARMED, entry delay = ARMED | Fewer messages; the FSM keeps both delay states internally |
| D-43 | `online` with timestamp; clean `offline` with reason + timestamp; Last Will without timestamp | The Will payload is fixed at CONNECT |
| D-44 | Reply sets per command (§10.4) | The app can be stale; the Pi always answers |
| D-45 | App status-card priority, offline behaviour, ALREADY_* as snackbars, 20-item list ordered by `event_id`, one reconnection summary notification | Follows the priority rule and avoids notification bursts |
| D-46 | IR reflective sensors (“PIR” on slides), FoneStar EP-36 speaker (“Buzzer Module”) with NPN driver, MQ-2 10 k/20 k divider, RGB common anode at 5 V, no RTC, INA219 for mains detection and battery estimate | See §4; hardware review of 31 Oct |
| D-47 | FSMs (state-dependent behaviour) and flowcharts (procedures) both kept; control-loop flowchart = `AlarmControllerThread::run()`, each FSM = a `switch` in `handleEvent()` | A flowchart of the FSM logic would need a “which state?” diamond for every event |
| D-48 | Buildroot 2026.08, `raspberrypi4_64` base, Bootlin stable toolchain, RPi kernel 6.12, libgpiod v1 | Stable toolchain; libgpiod v2 would need a different toolchain |
| D-49 | C++17 + CMake; Paho MQTT C++ (MQTT 5), nlohmann-json, SQLite, OpenSSL | Available in Buildroot 2026.08 and on the laptop |
| D-50 | Local Mosquitto only for development/tests, same users/ACL/port; broker address in a config file | A cloud broker cannot be switched off on demand; no lock-in |
| D-51 | Time from NTP (chrony) + saved date; the application starts without waiting for NTP | No RTC on the Pi 4; the local alarm must not depend on the network |

---

## 18. Open issues and default decisions

Use the **default** when implementing until the team decides otherwise, and say in your answer that a default was used.

### 18.1 The 28 issues from the diagram review (Work Log)

| # | Issue | Default for implementation |
|---|-------|----------------------------|
| 1 | GUI doc said fire silence returns to the previous armed state | **Decided:** always DISARMED (§6.4); update the GUI doc/prototype text |
| 2 | No smoke self-loop in FIRE_ALARM | Smoke ignored in FIRE_ALARM (F-3) |
| 3 | “Wi-Fi loss ≤ 2 s” needs a link monitor | Netlink `LinkMonitor` in NetworkMQTTThread (§7.9) |
| 4 | `EVT_RETRY_TIMER` is not a controller event | Internal to NetworkMQTTThread; never queued |
| 5 | RGB colour depends on two FSMs | One function of (power, network) — §4.4 / §6.10 |
| 6 | POWER_RESTORED after SHUTDOWN not on the boot branch | P-1: last power event SHUTDOWN or POWER_LOST + mains at boot → POWER_RESTORED |
| 7 | Halted Pi may not restart when mains returns | Hardware test (§4.7); document the result |
| 8 | No BATTERY_MODE → SHUTDOWN path | Implement P-7 (critical voltage from BATTERY_MODE) |
| 9 | Boot on battery sends no POWER_LOST | P-2: send it unless the last power event already is POWER_LOST |
| 10 | No status publish for 10 % battery steps | `EVT_BATTERY_LEVEL` from PowerMonitorThread → status |
| 11 | Default-PIN branch has no working database | Create a new empty database in that branch |
| 12 | No rule for writing `vigil.db.bak` | Backup after a good start-up check and after every PIN change (§9.5) |
| 13 | Start-up could block on NTP | Start without waiting (already in `S99vigil`); saved-date script |
| 14 | MQ-2 warm-up drawn as blocking | Runs in parallel in MQ2SensorThread; detection inactive meanwhile; 60 s default |
| 15 | No branch for PWM/GPIO failures at start | Degraded start + FAULT (§6.8 step 4) |
| 16 | Watchdog armed only when the Supervisor starts | Open `/dev/watchdog` early in `main` |
| 17 | Lockout counter resets on reboot | Accepted for the prototype (state in RAM); mention in the report |
| 18 | Correct local PIN while disarmed logs a false DISARMED | No event when nothing changes |
| 19 | Fire-silence box does not stop intrusion | One valid keypad PIN silences both and stops the delay timers |
| 20 | Beeps collide with alarm tones | Alarm keeps sounding; beeps dropped, or played over the warning (§6.10) |
| 21 | PBKDF2 can block the controller | 20 000 iterations as a start; measure; constant-time compare |
| 22 | Analysis slides lack lockout and ALREADY_DISARMED | Report note: the design extends the analysis |
| 23 | Control loop dispatches exclusively | Every FSM that handles the event, Fire FSM first |
| 24 | No branch for get_events, faults, key digits | get_events in Network; `EVT_FAULT`; `EVT_KEY_DIGIT` → beep |
| 25 | epoll without timeout → no heartbeat | 1 s timeout, heartbeat every pass |
| 26 | One eventfd read can stand for several events | Drain the queue after each read |
| 27 | Shutdown fire-and-forget | Wait for Network and Storage with timeouts (§6.9) |
| 28 | `change_pin` rules undefined | Lockout → LOCKED_OUT; wrong current PIN counts; RAM updated only after Storage confirms |

### 18.2 Gaps found while writing this guide

| ID | Gap | Default |
|----|-----|---------|
| G1 | `event_id` exists only after the insert | StorageThread hands each committed event to NetworkMQTTThread (`IEventSink`) |
| G2 | `[smoke]` guard needs the level, not an edge | `MQ2SensorThread::isSmokePresent()` (atomic, debounced) |
| G3 | Keypad conventions not specified | `#` submit, `*` clear, 10 s entry timeout, ≤ 8 digits kept |
| G4 | Some `details` values not fixed by the slides | INTRUSION_ALARM `IR1`/`IR2`; POWER_LOST battery %; POWER_RESTORED null or `BOOT` |
| G5 | Contents of response `detail` | LOCKED_OUT = remaining minutes; INVALID_NEW_PIN = `"4-6 digits"` |
| G6 | How the RGB LED “shows a fault” | Blink the current colour at 1 Hz |
| G7 | Malformed / retained / non-expiring commands | Ignore + syslog, no reply |
| G8 | No `FaultComponent` for the UPS/INA219 | Syslog only (a new `UPS` value would need a team decision) |
| G9 | Door already open when arming | Not detected until it closes and opens again (limitation) |
| G10 | Armed state is not persisted | After a reboot the system starts DISARMED (as the start-up flowchart says) |
| G11 | Tone plan mentions an exit-delay warning; the FSM has none | No exit-delay tone (follow the FSM); optional later |
| G12 | Arm button outside DISARMED | Ignored, with ERROR_BEEP when no alarm sounds |
| G13 | `change_pin` while a save is pending | `SAVE_FAILED` with detail `BUSY` |
| G14 | Events/commands needed but not in the class diagram | `EVT_BATTERY_LEVEL`, `EVT_FAULT`, `EVT_PIN_SAVE_RESULT`; `CommandQueue::popFor/close`; `Event::monoNs` |

### 18.3 Deck, report and document items (not code)

- Slides 35–36: rename `PIRSensorsThread` → `IRSensorsThread`; slide 36: add the policy and numbers (FIFO 80/70/50, OTHER).
- Driver slide (44): add “the only custom driver” and the interface (device node, file operations, command struct, error codes, overlay note) — §8.
- Add the Wi-Fi specification slide (802.11n/ac on CYW43455 + brcmfmac, WPA2-PSK via wpa_supplicant, boot order).
- Diagram fixes: Operational FSM guard on its own line (Remote Disarm only); PIN flowchart fire branch also silences the intrusion alarm; control loop: `change_pin` goes through the lockout and PIN check, remove the retry timer; class diagram RGB line; package diagram links (input threads → libgpiod, PowerMonitor → i2c-dev, Supervisor → threads); slide 29 footer: Change PIN also returns LOCKED_OUT; Power FSM font, BATTERY_MODE → SHUTDOWN arrow, boot action for POWER_RESTORED.
- GUI and broker documents: client ID, `request_id`, 20 events, `NetworkMQTTThread`, silence → DISARMED, “Offline” without time.
- Report: power-failure alerts are instant only when the router/ONT has its own UPS (recommended installation); otherwise they arrive when connectivity returns. Declare the use of AI tools (Figma Make prototype) as the course rules require.

---

## 19. Implementation roadmap

Phase 4 of the Gantt runs until ≈ 27 Dec 2026; verification in the first half of January. Milestones in dependency order (owners as agreed: David = cloud/MQTT/app, Ana = hardware/driver/hardware threads):

| # | Milestone | Owner | Done when |
|---|-----------|-------|-----------|
| M0 | Repository skeleton, Mosquitto test broker + `test-broker.sh`, Buildroot config + overlay, EMQX deployment started | — | **done** (3 Nov) |
| M1 | Finish EMQX (2 users, 13 rules incl. deny-all), `cloud/samples/*.json`, optional `docs/protocol.md` | David | `SUB_WAIT=3 cloud/test-broker.sh secrets/emqx.env` → 13 passed |
| M2 | `pi/` skeleton: CMake (host / sim / target), `core/` (Event, queues, `PiMutex`, Config, time, log), `WorkerThread`, test harness | both | host build + `ctest` green; cross-built `vigil --version` runs on the Pi |
| M3 | `AlarmLogic`, `PinHasher`, status and output mapping | both | every scenario of §16.2 passes |
| M4 | `StorageThread` + `Database` (schema, outbox, `get_events`, PIN, backup, degraded mode) | David | storage tests pass; PIN and pending rows survive a restart |
| M5 | `NetworkMQTTThread`, `MqttClient`, `Protocol`, `LinkMonitor`; simulation build end to end with Mosquitto | David | `online`/Last Will, commands, outbox order and pacing, `get_events`, reconnection order verified with `mosquitto_sub` |
| M6 | `vigil_pwm` driver + overlay + test tool; BR2_EXTERNAL packages; `config.txt` | Ana | slide-53 driver tests pass; the image boots with `/dev/vigil_pwm` |
| M7 | Hardware threads: LocalInput, IR, MQ2, PowerMonitor, Output | Ana | slides 49, 50, 53, 54 pass on the Pi |
| M8 | Supervisor + watchdog, RT settings, latency measurements | both | slide 58 passes |
| M9 | Flutter app milestones §13.8 (in parallel from M1) | David | app tests of slides 55–56 pass |
| M10 | Full integration on EMQX, 24 h run, autonomy, all deck tests + the ≈ 35 new rows, report material | both | everything green, results recorded |

---

## 20. Conventions and working rules

### 20.1 Code

- C++17, namespace `vigil`, one class per header/source pair named after the class; headers under `include/vigil/<area>/`.
- Keep the diagram names for classes, public methods, states and events (they are in the report); private members may use a trailing underscore.
- RAII for every file descriptor, line request, SQLite handle and mutex; no raw `new`/`delete`.
- Errors: return codes / `std::optional` in the threads; exceptions only during start-up and configuration; every `run()` catches everything, logs it and lets the Supervisor decide (no exception crosses a thread boundary).
- Logging with `syslog()` (ident `vigil`), one line per fact, levels used consistently. **Never** log a PIN, a password or a `cmd` payload.
- No string-built SQL; prepared statements only. Validate every external input (MQTT payloads, config files).
- Each logic change comes with unit tests; each component has the “done when” of §19.

### 20.2 Git

- Small commits with clear messages; one feature per branch when both work on the same area.
- Before every commit: `git status` must show no password, key, certificate, `broker.conf`, `broker.json` or `wpa_supplicant.conf`.

### 20.3 Working with Claude on this project

1. Start a task by naming the section of this guide it implements (e.g. “implement §7.4 MQ2SensorThread”).
2. Claude reads that section plus §5–§6, proposes the files to create/change, then implements with tests.
3. If the implementation needs a rule that is not here, Claude stops and asks, or uses the §18 default and says so.
4. If a decision changes, update this guide (and the Decisions Record in the Claude project) in the same change.

---

## Appendix A — Glossary

| Term | Meaning |
|------|---------|
| Alert | Event published on `alert`, stored in the outbox, shown as a phone notification |
| Log event | Event published once on `event` (history only), recovered with `get_events` |
| Outbox | `PENDING_NOTIFICATION`: alerts stored before publishing, marked SENT after PUBACK |
| `event_id` | `EVENT_LOG.id`; orders events, drives the app sync, lets the app drop duplicates |
| Clean Start / Session Expiry | MQTT 5 session controls; the Pi keeps no session (1 / 0), the app keeps one (0 / max) |
| Message Expiry | MQTT 5 property: the broker drops a `cmd` not delivered within 30 s |
| Last Will | Message the broker publishes for the Pi when it disappears without DISCONNECT |
| Retained | The broker keeps the last message of the topic and gives it to every new subscriber |
| PUBACK reason code | QoS 1 acknowledgement; `0x00` success, `0x10` success with no subscribers, `0x97` quota exceeded |
| eventfd / timerfd / epoll | Linux fds that let the controller wait for queued events and timers in one call |
| `SCHED_FIFO` | Real-time fixed-priority scheduling (1–99) |
| PI mutex | Priority-inheritance mutex: a low-priority holder temporarily inherits the waiter's priority |
| hrtimer | Kernel high-resolution timer used to step tone/blink patterns |
| Device-tree overlay | `.dtbo` applied by the firmware (`dtoverlay=`) that adds/changes hardware nodes |
| Platform driver | Kernel driver bound to a device-tree node by its `compatible` string |
| libgpiod | User-space library for the GPIO character device (`/dev/gpiochip*`) |
| INA219 | I²C current/voltage monitor on the UPS HAT (`0x42`) |
| BR2_EXTERNAL | Buildroot mechanism to keep project packages and board files outside the Buildroot tree |
| Degraded mode | Running with a failed part (sensor, driver, database) while reporting a FAULT |

