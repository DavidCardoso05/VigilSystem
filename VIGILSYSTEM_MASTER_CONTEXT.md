# VigilSystem — Master Context

| Field | Value |
|---|---|
| Document | `VIGILSYSTEM_MASTER_CONTEXT.md` — single source of truth (SSOT) for VigilSystem |
| Version | **1.4** — 2026-11-14. v1.0 approved by David and distributed to the repository and the Claude Project; v1.1 adds DEC-013 (Phase 4 presentation decisions); v1.2 adds DEC-014 (intrusion `ALARM_SILENCED` event removed); v1.3 adds DEC-015 (Pi and broker ready for several homes, app for one home); v1.4 adds DEC-016 (app MQTT session handling), approved by David |
| Last consolidation | 2026-11-09 |
| Maintained by | Claude, only with explicit approval from David Cardoso (see §O and `VIGILSYSTEM_GUIDELINES.md`) |
| Team | Group 8 — David Cardoso (PG63873), Ana Sofia Cunha (A107017) |
| Language | Technical English (conversations with David are in European Portuguese) |

> **Tags.** Every statement below is tagged when it is not an approved, verified fact:
> **[APPROVED]** decided by David · **[VERIFIED]** checked against a file or a test result ·
> **[UNVERIFIED]** stated in a document but not checked · **[ASSUMPTION]** reasonable belief, not confirmed ·
> **[OPEN]** waiting for a decision or a measurement · **[PROPOSAL]** suggested, not approved.
> Untagged statements in §C–§F and §I describe the current approved design.

---

## 0. How to use this document

### 0.1 Authority and precedence (DEC-011)

1. **This document** describes the current approved state and the full decision history.
2. **`VIGILSYSTEM_GUIDELINES.md`** says how to work on the project (rules, SOLID, diagrams, maintenance).
3. **Specialised documents in `docs/`** (Implementation Guide, hardware specification, design-deck change list) hold detail. They derive from this document; if they disagree with it, this document wins and the disagreement is recorded in §N.
4. **`docs/archive/`** holds superseded documents, kept only as history. Never implement from them.

Mandatory course requirements (§B.1) rank above every design decision. A conflict between them is reported to David, never resolved silently.

**Never** treat a document as current because of its date or its name. Several documents dated 2–5 Nov 2026 describe a superseded design (§N.3).

### 0.2 Which section answers which question

| Question | Section |
|---|---|
| What is VigilSystem and what must it do? | §A, §B |
| What is the approved design? | §C (overview), §D (hardware), §E (software), §I (communication) |
| Why is it like that? What else was considered? | §G (decision records), §H (simplification history) |
| How is SOLID applied? | §F |
| What exists, what works, what has been tested? | §J, §K |
| What must the presentation show? | §L |
| What is left to do? What is still undecided? | §M, §N |
| How must Claude work with the team? | §O and `VIGILSYSTEM_GUIDELINES.md` |

### 0.3 Rules for future conversations

- Before proposing any architectural change, read §C, §E, §G and §N. Settled decisions are reopened only for a **new** problem, and only with David's authorization.
- Decision identifiers: `DEC-001…DEC-012` (simplification review and consolidation, 8–9 Nov 2026); `D-01…D-51` (earlier design decisions, status in §G.3). DEC-013 (presentation, 9 Nov), DEC-014, DEC-015 and DEC-016 (14 Nov) followed. New decisions continue at `DEC-017`.
- Slide numbers refer to the **88-slide Design deck of 8 Nov 2026** unless stated otherwise (see §L.4: the deck stored in the Claude Project is an older 61-slide version).

### 0.4 Sources reviewed for this consolidation

| Source | Date | How it was used |
|---|---|---|
| This conversation (simplification review) | 8–9 Nov 2026 | Primary source for DEC-001…DEC-011. The part before the context summary is known only through that summary and the decision log written at the time |
| `claude/VigilSystem – Simplification Decision Log.md` (Portuguese) | 8–9 Nov | Full record of DEC-001…DEC-010 |
| `claude/VigilSystem – Presentation Changes (DEC-001 to DEC-006).md` (Portuguese; covers DEC-001…DEC-010) | 9 Nov | Slide-by-slide change list for the 88-slide deck |
| Implementation Guide (Project copy and repo copy `docs/IMPLEMENTATION_GUIDE.md`) | 4 Nov | Detailed rules still valid where not superseded (§G.3) |
| Design Decisions Record, Revision Plan (+ pointer), Class Diagram, Package Diagram, Class Diagrams for Slides, Diagrams Work Log, Data Storage/ERD/Cloud updates, MQTT Broker Selection, Mobile App GUI Design, Hardware Context | 27 Oct – 4 Nov | Historical design states; valid facts carried over with their status |
| Analysis deck `G8_VigilSystem_A_Presentation.pptx` | 25 Oct | Requirements and constraints (§B) |
| Design deck, 88-slide version (uploaded to the conversation) | 8 Nov | Current presentation baseline |
| Design deck, 61-slide version (in the Claude Project) and repo PDF (61 slides, 3 Nov) | 2–3 Nov | Older versions |
| Repository `~/VigilSystem` (read-only; `secrets/` not opened) | 3–5 Nov | Implementation status (§J) |

All Claude Project documents listed above were archived in `docs/archive/` on 9 Nov 2026 and then removed from the Project (confirmed by David); the index is `docs/archive/README.md`.

**Not available:** earlier conversations (2–5 Nov); the Claude Project memory; the full text of the contradiction list CON-01…CON-19 (only CON-01, 02, 03, 06, 08, 12 and 19 are recoverable); the original list of ten global working rules; `NEXT_3_HOURS.md`; "Threads Implementation Class Plan (3h)"; "Design Deck: Changes To Make (31 Oct)"; the Figma prototype; the official course syllabus.

---

## A. Project identity and overview

| Item | Value |
|---|---|
| Name | **VigilSystem** — smart home security and environmental hazard monitoring system |
| Group | Group 8: David Cardoso (PG63873) and Ana Sofia Cunha (A107017) |
| Programme | MEEIC (Master in Industrial Electronics and Computers Engineering), Universidade do Minho, ESRG — Embedded Systems Research Group |
| Courses | SEA (Sistemas Embebidos Avançados) and STR (Sistemas de Tempo Real) |
| Ownership | David: cloud, MQTT contract, communication thread and classes, Flutter app. Ana: hardware, the `vigil_pwm` driver, the hardware-facing threads |
| Phase (9 Nov 2026) | Design phase closing (simplification review done; presentation not yet updated). Implementation started only as a laptop prototype of the superseded architecture (§J) |
| Deliverable language | English (slides and report) |

### A.1 Problem

Commercial home alarms are expensive, rely on proprietary hardware and monthly subscriptions, and become vulnerable during power or internet outages.

### A.2 Purpose and operating concept

A Raspberry Pi 4 in a small enclosure near the entrance:

1. **Detects intrusion** while armed: two IR reflective sensors on door/window frames detect the movement (opening) of the monitored door or window (DEC-010).
2. **Detects smoke/gas 24/7**, armed or disarmed, with an MQ-2 module.
3. **Sounds distinct local alarms** on a speaker (fire, intrusion, entry warning, key and error beeps) and shows state on a red LED (armed/alarm) and an RGB LED (power source + broker connection).
4. **Is controlled locally** (arm button; 3×4 keypad for the PIN) and **remotely** (Android app over MQTT 5/TLS through the EMQX Serverless broker).
5. **Keeps running on battery** (UPS HAT, 2×18650) when mains fails, and enters a safe state when the battery is nearly empty (DEC-005b).
6. **Tolerates network loss**: every event is stored in a local SQLite outbox and delivered in order when the broker is reachable again (DEC-006).

The **local alarm never depends on the network**. The Pi is the only place where the PIN is validated and where state and history live.

### A.3 Scope boundaries

- One device per home. The Pi and the broker are ready for several homes (DEC-015): each home has its own `home_id`, its own MQTT users and its own ACL rules in one shared EMQX deployment. The implementation and the demonstration use one home (`home1`) and one Android app configured for it (several phones may share it); the app never handles several homes.
- Cloud = broker only: no backend, REST API, cloud database or push service (D-10).
- The MQ-2 is an educational sensor, not a certified smoke detector.
- Postponed and removed features are listed in §B.7 and §H; they are **not** part of the current system and are not shown in the Design presentation (DEC-001, option M1).

---

## B. Requirements and constraints

### B.1 Mandatory course constraints

| ID | Constraint | Source | Status in the design |
|---|---|---|---|
| C-1 | Raspberry Pi board (Raspberry Pi 4 Model B) | Analysis deck slide 8; Project instructions | Met |
| C-2 | Object-oriented C++ | Analysis slide 8 | Met (C++17) |
| C-3 | **SOLID principles** | Professor, confirmed by David (8 Nov) | Met, with evidence map (§F, DEC-009) |
| C-4 | Custom Linux image built with **Buildroot** | Analysis slide 8 | Met (Buildroot 2026.08) |
| C-5 | At least one **Linux device driver** of the group's own (character device, `.ko`) | Analysis slide 8 | Met: `vigil_pwm` (DEC-002). Not yet written |
| C-6 | **PThreads** for multithreading | Analysis slide 8 | Met (5 threads + `main`) |
| C-7 | **UML-based** tools for analysis and design | Analysis slide 8 | Met. Diagram tool rule: draw.io (see Guidelines) |

The official course syllabus was not available; C-1…C-7 come from the Analysis deck and from David. **[UNVERIFIED against the syllabus]**

Non-technical constraints (Analysis slide 9): groups of two, limited budget, deadline at the end of the semester. Professor facts reported by David: SOLID is required; no particular kind of driver is valued more than another; a different approach from previous years is expected.

### B.2 Functional requirements (Analysis deck, slides 5–6)

All requirements of the Analysis remain in force (confirmed by David, 8 Nov).

| ID | Requirement (Analysis wording, shortened) | How the design meets it | Status |
|---|---|---|---|
| FR-01 | **Dual Detection:** monitor movement and smoke/gas hazards | Movement = movement of a monitored door/window, 2 IR sensors (DEC-010); MQ-2 24/7 | Designed |
| FR-02 | **Distinct Alarms:** specific tones for intrusion and fire | Tone sequences in user space played by `vigil_pwm` (DEC-002) | Designed |
| FR-03 | **Flexible Control:** arm/disarm locally (keypad/button) and remotely (app) | Button + keypad; app commands `arm`, `disarm` | Designed |
| FR-04 | **Entry/Exit Delay**, configurable | 60 s exit, 30 s entry (config file); remote arm has no exit delay | Designed |
| FR-05 | **Fire Alarm Silence Period**, configurable | 10 min; smoke ignored during the period; reactivated if smoke persists | Designed |
| FR-06 | **Instant Alerts** for intrusion, hazard, grid failure and restoration | Alerts on topic `alert`, phone notification through the app's foreground service | Designed |
| FR-07 | **Power Resilience:** automatic switch to UPS battery | UPS HAT; INA219 monitoring; LOW_BATTERY and safe state (DEC-005a/b) | Designed |
| FR-08 | **Visual Status:** red LED armed/disarmed (and alarm); RGB LED power source + network connection | Red LED via PWM1; RGB via 3 NPN switches (DEC-007) | Designed |
| FR-09 | **Connectivity Monitoring** of the broker, not only Wi-Fi, shown on the RGB LED | MQTT keep-alive, loss detected in ≤ 60 s (DEC-001 removed the ≤ 2 s Wi-Fi monitor) | Designed |
| FR-10 | **PIN Management:** change the PIN only from the app | `change_pin` command, current PIN required | Designed |
| FR-11 | **Pending Notification Handling:** store undelivered notifications, deliver in chronological order | Outbox = `EVENT_LOG.status/sent_at`; resent in `id` order (DEC-006) | Designed |
| FR-12 | **Event Logging** with timestamp in persistent storage | `EVENT_LOG` in SQLite on the microSD | Designed (limitation: runtime database failure → syslog only, DEC-004) |

### B.3 Non-functional requirements (Analysis deck, slide 7)

| ID | Requirement | How the design meets it |
|---|---|---|
| NFR-01 | **Real-time responsiveness:** sensor events and local commands handled deterministically with minimal delay | SCHED_FIFO threads, PI mutexes, bounded waits, targets in §B.4 |
| NFR-02 | **Availability:** grid ↔ battery switch causes no reset and no interruption | UPS HAT; degraded start; watchdog with state restore (DEC-004) |
| NFR-03 | **Security:** authenticated and encrypted remote communication; PIN validated only on the device and never stored in plain text | MQTT 5 over TLS, per-role ACL, PBKDF2-SHA256 salted PIN hash, shared lockout |

### B.4 Timing and performance targets

| Target | Value | Source |
|---|---|---|
| Sensor event → local alarm output | ≤ 1 s, also under CPU load | Deck slide 85; DEC-008 (AT-22) |
| Key press → beep | ≤ 100 ms | Deck slide 85; DEC-008 (AT-23) |
| Alarm event → phone notification | ≤ 5 s (when connected) | Deck slide 85; DEC-008 (AT-03) |
| Periodic loops | 15 ms (keypad), 100 ms (safety sensors), 1 s (power) without missed periods | DEC-003a; DEC-008 (AT-23) |
| Broker loss detected by the Pi | ≤ 60 s (2 × keep-alive) | D-18 as modified by DEC-001 |
| Battery autonomy (typical load) | ≥ 3 h (≈ 4 h theoretical) | Deck slide 81; DEC-008 (AT-13) |

### B.5 Hardware constraints

- GPIO logic is 3.3 V and **not 5 V tolerant**; 8 mA default drive per pin. No 5 V node may reach a GPIO (basis of DEC-007).
- Two hardware PWM channels: PWM0 on GPIO18 (speaker), PWM1 on GPIO13 (red LED).
- No RTC on the Pi 4: time from NTP plus a saved-date script (D-51).
- UPS HAT: 5 V up to 2.5 A continuous; INA219 at I²C `0x42` gives voltage and current only (no "mains lost" signal).

### B.6 Safety and reliability constraints

- **Fail-loud fire alarm:** once triggered it stays on until a valid keypad PIN silences it; remote disarm is rejected during a fire alarm (D-26, D-30).
- **The local alarm never depends on the network** (D-10, Guide rule 5).
- **Never refuse to start because one part failed:** degraded start with one FAULT alert per component (DEC-004).
- **Speaker protection:** never leave GPIO18 at a DC level; speaker duty ≤ 50 % (enforced by the driver, DEC-002).
- **Unexpected software errors** end in a watchdog reset with the last armed/disarmed state restored (DEC-004).

### B.7 Explicitly excluded from the current scope

| Item | Type | Decision |
|---|---|---|
| Battery percentage (10 % steps) | Postponed, not presented | DEC-001 (M1) |
| Power alerts at boot (POWER_RESTORED "BOOT", POWER_LOST at boot on battery) | Postponed, not presented | DEC-001 |
| Database backup and restore | Postponed, not presented | DEC-001 |
| `get_events` history sync | Postponed, not presented | DEC-001 |
| Reconnection summary notification in the app ("N alerts received") | Postponed, not presented | DEC-001 |
| Wi-Fi loss detection in ≤ 2 s (netlink `LinkMonitor`) | Removed | DEC-001 |
| Thread restart by a supervisor thread | Removed | DEC-001, DEC-003a |
| RGB "fault blink" | Removed | DEC-001 |
| `poweroff` on critical battery | Replaced by the safe state | DEC-005b |
| App: iOS, login, history screen, several homes or choosing a home | Out of scope | DEC-006 (6.4), D-23, DEC-015 (15.5) |
| Cloud backend, REST API, cloud database, Firebase push | Declined | D-10 |

### B.8 Approved divergences from the Analysis

Not shown in the presentation (DEC-013, P4-6); the reasons are kept here and in §G for oral questions.

| Analysis | Design | Decision |
|---|---|---|
| "Starts disarmed" (system-events table, deck A slide 15) | Restores the last ARMED/DISARMED state at boot | DEC-004 (4.1 b) |
| ERD with 3 tables (`PENDING_NOTIFICATION` included) | 2 tables; outbox is a pair of columns in `EVENT_LOG` | DEC-006 (6.1 b') |
| Motion detection by a PIR sensor (system-events table) | Movement of the monitored door/window, detected by IR reflective sensors | DEC-010 (A') |

---

## C. Approved system architecture (overview)

### C.1 Context

```
 Door/window IR ×2 ─┐                         ┌─ Speaker (PWM0, GPIO18) ─┐
 MQ-2 smoke/gas ────┤                         ├─ Red LED (PWM1, GPIO13) ─┤  via /dev/vigil_pwm
 3×4 keypad ────────┤   Raspberry Pi 4B       └─ RGB LED (3 NPN, GPIO)    (custom driver)
 Arm button ────────┤   Buildroot Linux
 UPS HAT INA219 ────┘   one process "vigil"   ── SQLite (microSD): EVENT_LOG + PIN_CONFIG
   (I²C 0x42)            5 pthreads + main     ── /dev/watchdog
                               │
                        Wi-Fi, MQTT 5 over TLS (port 8883)
                               │
                     EMQX Serverless broker (cloud, broker only)
                               │
                  Android app (Flutter, foreground service)
```

Development and tests use a local Mosquitto broker with the same users, ACL and port (D-50).

### C.2 Main components and responsibilities

| Component | Responsibility |
|---|---|
| Sensors and inputs (IR, MQ-2, keypad, button) | Produce digital levels sampled by the Pi |
| UPS HAT + INA219 | Keep the Pi powered; report battery voltage and current |
| `vigil` process | All logic: sampling, decisions, outputs, persistence, MQTT |
| `vigil_pwm.ko` (own driver) | Plays step sequences on the speaker and red-LED PWM channels with kernel timing |
| SQLite database | Event log with outbox state; PIN hash |
| EMQX Serverless | Routes messages between the Pi and the phones; holds retained state and the Last Will |
| Android app | Thin remote control: shows state and events, sends commands, never validates anything |

### C.3 Main data flows

1. **Local alarm path** (no network involved): `SafetySensorsThread` samples IR/MQ-2 every 100 ms → confirmed transition → `Event` on the EventQueue → `AlarmControllerThread` runs `AlarmLogic` → `AlarmOutputs` writes a tone sequence to `/dev/vigil_pwm` and sets the RGB lines → a `CommCommand` is queued for logging/publishing.
2. **Event and alert path:** `CommThread` inserts the event into `EVENT_LOG` as `PENDING` → `OutboxPublisher` publishes pending events in `id` order (≤ 5/s) → marks them `SENT` on PUBACK. Offline, events stay `PENDING` and leave in order after reconnection.
3. **Remote command path:** app publishes on `vigil/<home_id>/cmd` (QoS 1, 30 s expiry) → Paho callback → `CommThread` → `Protocol` parses → `Event` on the EventQueue → `AlarmLogic` decides → response, log event and status go back through the `CommQueue`.

### C.4 Concurrency model (summary; detail in §E.1)

Five threads plus `main`: `LocalInputThread` (15 ms), `SafetySensorsThread` (100 ms), `PowerMonitorThread` (1 s), `AlarmControllerThread` (waits on the EventQueue until the next deadline, at most 1 s), `CommThread` (CommQueue). `main` supervises heartbeats and feeds the watchdog. Two queues only: **EventQueue** (all → controller) and **CommQueue** (controller and Paho callbacks → Comm). Rule: **merge threads, never responsibilities** (DEC-003a/b).

### C.5 State management

- All system state (intrusion, fire, power, network, lockout, deadlines) lives in **`AlarmLogic`**, owned by `AlarmControllerThread`. It is pure: no threads, no I/O, time injected.
- Every domain object belongs to exactly one thread; only the two queues are shared.
- Persistent state: `EVENT_LOG` (events + outbox state) and `PIN_CONFIG`. At boot, `main` reads the last `ARMED`/`DISARMED` event (by `id`) and injects the initial intrusion state (DEC-004, 4.1 b).

### C.6 Error handling and recovery (DEC-004)

| Failure | Response | What keeps working |
|---|---|---|
| IR1 / IR2 | FAULT(IR1/IR2) once; sensor ignored until reboot | Everything else |
| MQ-2 | FAULT(MQ2) | Everything except fire detection |
| Keypad / button | FAULT(KEYPAD) | Remote disarm. **Limitation:** a fire alarm cannot be silenced until reboot |
| PWM driver | FAULT(SPEAKER) (covers the former PWM_DRIVER) | RGB LED and alerts |
| INA219 | FAULT(POWER_MONITOR) | Everything except mains-failure detection, low battery and safe state |
| Database at boot (corrupt or unreadable) | New database + default PIN hash + FAULT(DATABASE) + DISARMED | Everything |
| Database while running | syslog only; events of that period are lost even if connected (declared double-fault limitation) | Local alarm and the PIN in RAM |
| Broker / Wi-Fi | Not a fault: reconnect forever (backoff 1 → 60 s), outbox keeps events, RGB shows the loss | All local functions |
| Unexpected error / hung thread | Error logged; thread stops sending heartbeats; watchdog resets the Pi (15 s); last ARMED/DISARMED state restored | — |

Each FAULT is sent once per component per boot, with no retries. GPIO lines are requested one by one so one failure does not take others down.

### C.7 Why the architecture has this shape

- **One decision maker.** All rules live in one pure class (`AlarmLogic`), so behaviour is deterministic and every rule is unit-testable on a laptop (D-01, D-47, DEC-003b).
- **Threads exist only for timing reasons.** Separate threads are kept where the activation pattern differs (15 ms keypad, 100 ms safety sensors, 1 s power, event-driven controller, blocking I/O in Comm). Responsibilities that used to justify extra threads (outputs, storage, network, supervision) became classes (DEC-003a/b).
- **The local alarm never waits for I/O.** Disk and network work happens only in `CommThread` (SCHED_OTHER); the controller only computes, writes non-blocking driver commands and enqueues.
- **The kernel does only what needs kernel timing.** The driver plays sequences; tone design and output policy stay in user space (DEC-002).
- **Fail-loud, degrade, then reset.** Expected component failures degrade with one FAULT alert; unexpected errors reset through the watchdog and restore the armed state (DEC-004).

---

## D. Hardware architecture

### D.1 Components and availability (9 Nov 2026)

| Component | Reference | Role | Availability |
|---|---|---|---|
| Raspberry Pi 4 Model B | — | CPU (BCM2711, 4× Cortex-A72) | **Owned** |
| microSD 16/32 GB, Class 10 / A1 | — | Boot (FAT32) + rootfs (ext4) + SQLite | **Owned** |
| Breadboard | — | Bring-up | **Owned** |
| Red LED 5 mm diffused | Botnroll 2629 | Armed / alarm indicator | **Owned** |
| RGB LED common anode | CEL03119 (piranha) per Hardware Context **[UNVERIFIED part number]** | Power source + broker status | **Owned** |
| Speaker FoneStar EP-36, 8 Ω, 0.3 W | — | Tones ("Buzzer Module" in old slides) | **Owned** |
| IR reflective proximity sensor ×2 | Model to confirm (SEN10019 drawn on slide 12) | Door/window movement | **Owned** |
| UPS HAT (with 8.4 V charger) | Botnroll 4106 | Backup power, INA219 at `0x42` | To order (week of 9 Nov) |
| 2× Li-ion 18650, 2600 mAh (2S) | Botnroll BAT03005 | Battery | To order |
| Keypad 3×4 | Botnroll 434 | PIN entry | To order |
| Push-button (breadboard + panel) | Botnroll 716 + 1188 | Local arming | To order |
| MQ-2 gas sensor brick | Botnroll 939 | Smoke/gas | To order |
| Passives and board | See D.6 | Interface circuits | To order |
| Commercial plastic enclosure | Bought in a local shop | Housing | To buy (check dimensions) |

### D.2 GPIO allocation

| BCM | Header pin | Function | Configuration | Active level |
|---|---|---|---|---|
| 17, 27, 22, 23 | 11, 13, 15, 16 | Keypad rows 1–4 | Input, internal pull-up | LOW = key in the scanned column |
| 24, 25, 5 | 18, 22, 29 | Keypad columns 1–3 | Output, open-drain, scanned | Driven LOW when scanned |
| 6 | 31 | Arm button | Input, internal pull-up | LOW = pressed |
| 16 | 36 | IR1 (door/window) | Input, bias disabled; external 10 kΩ pull-up + Schottky (DEC-007) | LOW = closed, HIGH = open |
| 20 | 38 | IR2 (door/window) | Same as IR1 | Same |
| 21 | 40 | MQ-2 DO via 10 kΩ / 20 kΩ divider | Input, bias disabled | LOW = smoke/gas |
| 13 | 33 | Red LED, PWM1 (ALT0) | Owned by `vigil_pwm` | HIGH = on |
| 18 | 12 | Speaker, PWM0 (ALT5) | Owned by `vigil_pwm` | — |
| 4, 26, 19 | 7, 37, 35 | RGB red, green, blue → NPN base | Output | **HIGH = on** (DEC-007) |
| 2, 3 | 3, 5 | I²C-1 SDA/SCL → INA219 `0x42` | I²C | — |

Reserved: GPIO0/1 (HAT EEPROM), GPIO14/15 (serial console). Free: GPIO7–12.
**Obsolete pin maps** (never use): red LED on GPIO26; RGB on GPIO4/13/19; "PIR" on GPIO16/20 (Project instructions before this consolidation).

### D.3 Electrical interfaces

| Circuit | Approved design | Decision |
|---|---|---|
| Speaker driver | GPIO18 → 470 Ω → NPN (BC337 or 2N2222A) base, 10 kΩ base pull-down; +5 V → speaker → 33 Ω / 0.5 W → collector; 1N4148 flyback across the speaker; peak ≈ 120 mA; duty ≤ 50 %, pin LOW when silent | D-46 |
| RGB LED | Common anode at +5 V; each cathode → channel resistor (390 Ω R, 220 Ω G, 220 Ω B) → NPN collector, emitter to GND; GPIO → 1 kΩ → base, 10 kΩ base–GND; ≈ 7–8 mA per channel; ≈ 2.6 mA base current per GPIO | DEC-007 (7.1 b) |
| Red LED | GPIO13 → 220 Ω → LED → GND, ≈ 6 mA | D-46 |
| IR sensors | Sensor OUT → Schottky cathode (BAT85 preferred for low leakage; 1N5817 acceptable); anode → GPIO; 10 kΩ GPIO → 3.3 V. Works for open-collector and 5 V push-pull outputs; LOW at the GPIO ≈ 0.3–0.5 V (below V_IL max 0.8 V); 5 V never reaches the GPIO. Sensor supply 5 V | DEC-007 (7.2 b) |
| MQ-2 | 5 V supply (heater ≈ 120–150 mA); DO (LM393, active-low) → 10 kΩ → GPIO21; 20 kΩ GPIO21 → GND; GPIO bias disabled (the internal pull-down would lower the HIGH level) | D-46 |
| Keypad | Rows with pull-ups; columns open-drain (released = high-Z) so two keys cannot short a HIGH column to a LOW one; verify the connector pin order with a continuity tester | Guide §4.3 |
| Arm button | SPST-NO to GND, internal pull-up, debounce 2 reads at 15 ms | Guide §7.2 |
| Boot-safe levels | `config.txt` `gpio=` lines: rows and button input pull-up; IR and MQ-2 input no pull; RGB GPIO4/19/26 output LOW (transistors off); GPIO13/18 output LOW until the driver takes them. **The IR line must change from `pu` to `pn` in the Guide (consequence of DEC-007)** | Guide §12.2 as modified by DEC-007 |

### D.4 Power

- UPS HAT: 5 V regulated to the Pi through the header, up to 2.5 A continuous; 2S Li-ion (6.0 V empty, 7.2–7.4 V nominal, 8.4 V full); charger 8.4 V DC; protections against over-charge, over-discharge, over-current and short circuit.
- INA219 (`/dev/i2c-1`, `0x42`, big-endian registers): calibration `0x05 = 4096`, configuration `0x00 = 0x3EEF`; bus voltage = `(reg 0x02 >> 3) × 4 mV`; current = signed `reg 0x04 × 0.1 mA`.
- **Mains detection** (no hardware signal): current < −100 mA for 3 consecutive 1 s samples → mains lost; > +50 mA for 3 samples → mains back; near 0 mA keeps the previous state. **[UNVERIFIED: sign of the current and thresholds must be checked on the board]**
- **Thresholds (DEC-005a/b):** LOW_BATTERY at ≤ 6.7 V for 3 samples, only on battery, once per outage; **critical ≤ 6.4 V for 3 samples → safe state** (no power-off).
- Initial power state: the first 3 samples decide silently; mains is assumed until then (DEC-005b, 5b.2).
- Power budget (5 V rail): typical ≈ 0.77 A, maximum ≈ 1.64 A (speaker sounding) against 2.5 A. Theoretical autonomy ≈ 4 h typical, ≈ 2 h at maximum load (≈ 19.2 Wh, ≈ 16 Wh usable). Acceptance target ≥ 3 h.

### D.5 Physical assembly (DEC-007, 7.3 b)

1. **Bring-up** on the breadboard, one circuit at a time, to test each peripheral and the driver.
2. **Demonstration build:** an interface board on protoboard (speaker driver, 3 RGB NPNs, MQ-2 divider, IR diodes and pull-ups, screw terminals for the sensor cables) inside a **commercial plastic box bought in a local shop** (decided 9 Nov), with cut-outs for the keypad, panel button, LEDs and speaker, and ventilation slots for the Pi.
3. **Installation:** IR sensors on the door/window frames; MQ-2 high on a wall or the ceiling; cables to the box.

### D.6 Parts still to buy (from DEC-007)

NPN BC337/2N2222A ×4 (+2 spare); 1N4148 ×1 (+1); Schottky BAT85 ×2 (+1); resistors 220 Ω ×3, 390 Ω ×1, 470 Ω ×1, 1 kΩ ×3, 10 kΩ ×7, 20 kΩ ×1, 33 Ω 0.5 W ×1; protoboard; pin headers, screw terminals, Dupont jumpers, multi-core cable; Pi heatsinks; enclosure; plus the main parts marked "To order" in D.1. The purchases are on the critical path of every hardware test.

### D.7 Validation status

**Nothing has been assembled or physically validated yet.** Approved connections are design intent only.

| Check | Purpose | Status |
|---|---|---|
| `i2cdetect -y 1` shows `0x42`; sign of the INA219 current with the charger unplugged | Mains detection | Not done (HAT not bought) |
| HAT cuts its output at full discharge and restores it when the charger returns | **Condition of the safe-state decision (5b.3)**; if it fails, 5b.3 is reopened | Not done |
| IR output type and real LOW level at the GPIO; detection distance to the leaf | DEC-007 (7.2) margin | Possible now (IR owned) |
| GPIO21 HIGH level with the divider (≥ ≈ 2.5 V); MQ-2 threshold with a controlled smoke source | MQ-2 interface | Not done |
| Transistor pinout of the parts bought; speaker volume with 33 Ω; GPIO18 LOW when silent | Speaker driver | Not done |
| RGB fully off with GPIO LOW and during boot | DEC-007 (7.1) | Not done |
| PWM on GPIO18 and GPIO13 with the custom overlay, GPIO19 untouched | Driver | Not done |
| Measured autonomy; time from 6.7 V to 6.4 V | DEC-005a risk (non-linear Li-ion discharge) | Not done |
| Enclosure fits the Pi with the HAT and the interface board | 7.3 | Not done |

### D.8 Hardware limitations

- Only the two instrumented doors/windows are protected; a door already open when arming is detected only after it closes and opens again.
- The MQ-2 also reacts to alcohol vapours and needs a warm-up (60 s default, to be measured); it is not a certified detector.
- Low tones are weak on a 36 mm speaker (tones to be tuned by ear).
- No RTC: timestamps are approximate until NTP syncs after a boot without internet; event order never depends on the clock (it uses `EVENT_LOG.id`).

---

## E. Software architecture

### E.1 Process and threads (DEC-003a, DEC-004 4.5)

One C++17 process, `/usr/bin/vigil`, started by `/etc/init.d/S99vigil` as root (needed for SCHED_FIFO, `/dev/watchdog`, GPIO, I²C and `/dev/vigil_pwm`). Paho also runs its own internal threads.

| Thread | Activation | Priority | Role | Heartbeat timeout |
|---|---|---|---|---|
| `LocalInputThread` | Periodic, 15 ms (absolute-time sleep) | SCHED_FIFO 50 | Keypad scan + arm button | 1 s |
| `SafetySensorsThread` | Periodic, 100 ms | SCHED_FIFO 80 | IR1, IR2 (2-read confirmation, one event per opening) and MQ-2 (5 reads; emits "detected" and "cleared") | 1 s |
| `PowerMonitorThread` | Periodic, 1 s | SCHED_FIFO 50 | INA219 | 3 s |
| `AlarmControllerThread` | Waits on the EventQueue until the next deadline, at most 1 s | SCHED_FIFO 80 | Runs `AlarmLogic`; applies outputs directly (driver + RGB) | 3 s |
| `CommThread` | CommQueue (with timeout) | SCHED_OTHER | SQLite + MQTT (outbox) | 10 s |
| `main` | 1 s loop after start-up | SCHED_OTHER | Heartbeats → watchdog; SIGTERM | covered by the watchdog (15 s) |

**Why these priorities.** The fire and intrusion path (SafetySensors → Controller → outputs) shares the top band, because an alarm is only as fast as its slowest stage. Keypad and power are human-speed or slow. Disk and network work is best-effort in SCHED_OTHER, so it can never delay the alarm.

**Accepted trade-offs (DEC-003a).** No thread is activated directly by a hardware edge, and the former FIFO 70 band is gone. IR detection can take up to ≈ 200 ms (two 100 ms samples), irrelevant next to the 30 s entry delay. A failure of one sensor must not stop the sampling of the others.

### E.2 Queues and synchronisation

- **EventQueue** (all threads → `AlarmControllerThread`) and **CommQueue** (controller and Paho callbacks → `CommThread`) are instances of `BlockingQueue<T>`: fixed capacity, `pthread_mutex` with `PTHREAD_PRIO_INHERIT`, `pthread_cond` on `CLOCK_MONOTONIC` (`pthread_condattr_setclock`), all inside the queue class.
- **Deadline-based waiting (approved mechanism).** The exit delay, entry delay, silence period, lockout and "restore the warning after a beep" are deadlines held as state in `AlarmLogic`, with time injected. The controller waits on the EventQueue until the nearest deadline, at most 1 s (so it can send its heartbeat). This replaces epoll + eventfd + 4 timerfds and keeps immunity to NTP clock steps.
- Each domain object belongs to one thread; only the two queues are shared. No dynamic allocation in the real-time loops.
- `Event` and `CommCommand` are small value types; `CommCommand` is a `std::variant` of small structs (replaces the former union-like `OutputCmd`/`StorageCmd`/`NetCmd`).

### E.3 Packages and classes (DEC-003b)

| Package | Classes | Responsibility |
|---|---|---|
| `threads` | `WorkerThread` (abstract); `LocalInputThread`, `SafetySensorsThread`, `PowerMonitorThread`, `AlarmControllerThread`, `CommThread` | Scheduling only: wait, call components, heartbeat; one error policy (in `WorkerThread`) |
| `logic` (no I/O, time injected) | `AlarmLogic` (coordinates, dispatches Fire first, holds deadlines and the network state field); `IntrusionFsm`; `FireFsm`; `PowerFsm`; `PinGuard` (lockout); `PinHasher`; `OutputPolicy` (state → desired outputs) | Rules |
| `devices` | `DebouncedInput` (4 instances: IR1, IR2, MQ-2, button; parameters: confirmation samples, active level, warm-up); `KeypadScanner`; `PinEntry` (`#`, `*`, timeout, at most 8 digits); `PowerSupplyMonitor` (3-sample hysteresis, 6.7 V / 6.4 V) | Turn signals into facts |
| `hw` | `IGpioInput`, `IGpioOutput`, `IPwm`, `IPowerSensor`; `GpioLines` (libgpiod v1), `PwmDevice` (`/dev/vigil_pwm`), `Ina219` (i2c-dev); simulated implementations | Hardware access |
| `outputs` | `AlarmOutputs` | Convert desired outputs into driver sequences and RGB line levels |
| `storage` | `Database` (RAII connection); `EventStore` (events + outbox); `PinStore` | SQLite |
| `net` | `IMessageTransport`; `MqttLink` (Paho, TLS, reconnection); `Protocol` (JSON, topic choice; topics built from `home_id`, DEC-015); `OutboxPublisher` (order, ≤ 5/s, mark SENT) | MQTT |
| `core` | `BlockingQueue<T>`; `Event`; `CommCommand`; `HealthMonitor`; `Watchdog` | Infrastructure |

About 35 types in total. Fallback **B−** (`AlarmLogic` as one class, ≈ 30 types) is declared but needs a new decision to be used.

**Dependency direction.** `logic` depends on nothing. `devices` and `outputs` depend on `hw` interfaces. `OutboxPublisher` depends on `IMessageTransport`. `main` is the composition root: it builds the real or the simulated implementations (`VIGIL_SIM`) and injects them through constructors.

**Refinements chosen in DEC-003b.** One `DebouncedInput` class replaces separate door, smoke and button classes. The network state is a field of `AlarmLogic` (no rules of its own after DEC-001). Power keeps its own class (`PowerFsm`) because it has rules.

**Concrete on purpose (documented trade-offs; not shown in the presentation, DEC-009 9.3).** `EventStore` and `PinStore` (in-memory SQLite is the test double); `AlarmOutputs` (its dependencies are already abstract); `PinHasher` (a pure computation, tested with the real implementation); `Watchdog` (only `main` uses it; an empty version is selected at compile time for simulation; `HealthMonitor` is pure and testable). The FSMs are `switch` statements over enums, with no State pattern.

### E.4 Behaviour (rules carried over from Guide §6, updated by the decisions)

**Intrusion FSM** — states `DISARMED`, `EXIT_DELAY`, `ARMED`, `ENTRY_DELAY`, `INTRUSION_ALARM`:

| From | Event [guard] | Actions | To |
|---|---|---|---|
| start | boot [last state ARMED] / [otherwise] | no event logged (DEC-004) | ARMED / DISARMED |
| DISARMED | button [no fire alarm] | key beep; start exit deadline (60 s); red LED on; status stays DISARMED | EXIT_DELAY |
| DISARMED, EXIT_DELAY | remote arm [no fire alarm] | cancel exit deadline; reply OK; `ARMED` (REMOTE); status | ARMED |
| EXIT_DELAY | exit deadline [no fire alarm] | `ARMED` (LOCAL); status | ARMED |
| EXIT_DELAY | exit deadline [fire alarm] | stay (the keypad PIN that silences the fire disarms) | — |
| ARMED | `EVT_MOTION_DETECTED` (IR1/IR2) | start entry deadline (30 s); warning tone; remember the sensor; no alert, no log | ENTRY_DELAY |
| ENTRY_DELAY | entry deadline | intrusion tone; red LED flashing; `INTRUSION_ALARM` alert (IR1/IR2); status | INTRUSION_ALARM |
| EXIT_DELAY, ARMED, ENTRY_DELAY, INTRUSION_ALARM | valid keypad PIN (no fire guard) | stop deadlines and tone; LED off; `DISARMED` (LOCAL); status. No separate event for stopping an intrusion alarm (DEC-014) | DISARMED |
| same four | remote disarm [no fire alarm ∧ not locked out ∧ PIN valid] | stop deadlines and tone; LED off; reply OK; `DISARMED` (REMOTE); status | DISARMED |
| DISARMED, EXIT_DELAY, ENTRY_DELAY, INTRUSION_ALARM | motion | ignored | — |
| any state other than DISARMED, or during a fire alarm | button | ignored, error beep (dropped while an alarm sounds) — default G12 | — |

**Fire FSM** — `NORMAL`, `FIRE_ALARM`, `SILENCED`: smoke detected in NORMAL → FIRE_ALARM (fire tone has priority, LED flashing, `FIRE_ALARM` alert `DETECTED`); further smoke ignored; remote disarm → `REJECTED_FIRE_ACTIVE` without testing the PIN; valid **keypad** PIN → SILENCED (tone off, 10 min silence deadline, `ALARM_SILENCED` (FIRE) alert, intrusion FSM → DISARMED); smoke ignored while SILENCED; silence deadline with smoke present → FIRE_ALARM (`REACTIVATED`), without smoke → NORMAL + `HAZARD_CLEARED` alert. There is no FIRE_ALARM → NORMAL transition (fail-loud). The "smoke present" guard uses the state kept from the `SMOKE_DETECTED`/`SMOKE_CLEARED` events (no read from another thread, DEC-003a).

**Power FSM** — `AC_POWERED`, `BATTERY_MODE`, `LOW_BATTERY`, `SAFE_STATE` (DEC-005a/b):

| From | Event | Actions | To |
|---|---|---|---|
| start | first 3 samples | decided silently, no alert; mains assumed until then | AC_POWERED or BATTERY_MODE |
| AC_POWERED | mains lost (3 samples) | `POWER_LOST` alert (details null); RGB battery colour; status BATTERY | BATTERY_MODE |
| BATTERY_MODE, LOW_BATTERY | mains back (3 samples) | `POWER_RESTORED` alert (details null); RGB mains colour; status GRID | AC_POWERED |
| BATTERY_MODE | ≤ 6.7 V, 3 samples | `LOW_BATTERY` alert (details = voltage, e.g. `"6.68 V"`), once per outage | LOW_BATTERY |
| BATTERY_MODE, LOW_BATTERY | ≤ 6.4 V, 3 samples | `SHUTDOWN` alert (details null; PENDING; published if connected); database closed + `sync`; system keeps running; later events go to syslog only | SAFE_STATE |
| SAFE_STATE | mains back (3 samples) | controlled reboot (`sync` + reboot); recovery through the normal boot path (DEC-012, 12.2) | — (reboot) |
| AC_POWERED | voltage events | ignored | — |

**Network state** (field of `AlarmLogic`): starts `NET_LOST`; `NET_CONNECTED` after CONNACK; it only changes the RGB colour. The local alarm never depends on it.

**PIN rules** (D-30…D-32, Guide BR-PIN-1…9): one wrong-PIN counter shared by keypad and app; the 5th consecutive wrong PIN starts a 5 min lockout and a `PIN_LOCKOUT` alert (LOCAL/REMOTE); during a lockout no PIN is hashed or compared, not even during a fire alarm; the counter resets after a valid PIN and at the end of the lockout (also at reboot — accepted limitation); PIN = 4–6 digits; stored only as `pbkdf2_sha256$<iterations>$<salt>$<hash>` (20 000 iterations to start, measured so one check takes ≈ 20–50 ms on the Pi), compared in constant time; never logged or published. Keypad entry (default G3): digits buffered (at most 8), `#` submits, `*` clears, 10 s timeout. A valid keypad PIN when nothing is active logs nothing (key beep only).

**Remote command checks (in order):**

| Command | Order of checks |
|---|---|
| `arm` | fire alarm → `REJECTED_FIRE_ACTIVE`; already ARMED/ENTRY_DELAY/INTRUSION_ALARM → `ALREADY_ARMED`; otherwise `OK` (also from EXIT_DELAY and during SILENCED) |
| `disarm` | fire alarm → `REJECTED_FIRE_ACTIVE`; already disarmed → `ALREADY_DISARMED`; lockout → `LOCKED_OUT` (detail = minutes left); PIN → `OK` / `INVALID_PIN` (counts) |
| `change_pin` | lockout → `LOCKED_OUT`; wrong current PIN → `INVALID_PIN` (counts); new PIN not 4–6 digits → `INVALID_NEW_PIN`; save already pending → `SAVE_FAILED` (`BUSY`); otherwise the new hash is saved by `CommThread` (`PinStore`) and the result comes back as an event; the hash in RAM changes only after a successful save → `OK`, else `SAVE_FAILED` |

**Output arbitration** (`OutputPolicy`, recomputed after every event): speaker base = fire > intrusion > entry warning > off; key and error beeps are dropped while a fire or intrusion tone sounds and are played over the warning, after which the warning is restored through a deadline (DEC-002). Red LED = flashing during any alarm, on in EXIT_DELAY/ARMED/ENTRY_DELAY, off otherwise; the wrong-PIN blink is dropped while flashing. RGB = green (mains, connected), yellow (mains, lost), blue (battery, connected), magenta (battery, lost); pure red is reserved for the red LED; no fault blink (DEC-001).

**Status snapshot** (published retained on every change, never periodically): `armed_state` = DISARMED (DISARMED, EXIT_DELAY) / ARMED (ARMED, ENTRY_DELAY) / INTRUSION_ALARM; `fire_state` = NORMAL / FIRE_ALARM / SILENCED; `power_source` = GRID / BATTERY; `timestamp`. No `battery_pct` (DEC-001).

### E.5 Data persistence (DEC-006)

SQLite library linked into the process (not a server), database `/var/lib/vigil/vigil.db` on the persistent ext4 partition; `PRAGMA journal_mode=WAL; synchronous=FULL; busy_timeout=2000`; times stored as ISO 8601 UTC text. Two tables:

```sql
CREATE TABLE IF NOT EXISTS EVENT_LOG (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,   -- event_id in every MQTT message
    event_type  TEXT NOT NULL CHECK (event_type IN (
                    'ARMED','DISARMED','WRONG_PIN','PIN_CHANGED','PIN_LOCKOUT','ALARM_SILENCED',
                    'INTRUSION_ALARM','FIRE_ALARM','HAZARD_CLEARED','POWER_LOST','POWER_RESTORED',
                    'LOW_BATTERY','SHUTDOWN','FAULT')),
    timestamp   TEXT NOT NULL,
    details     TEXT NULL,
    status      TEXT NOT NULL CHECK (status IN ('PENDING','SENT')),   -- outbox state (DEC-006)
    sent_at     TEXT NULL
);
CREATE INDEX IF NOT EXISTS idx_event_status ON EVENT_LOG(status, id);

CREATE TABLE IF NOT EXISTS PIN_CONFIG (
    id          INTEGER PRIMARY KEY CHECK (id = 1),  -- single row, never published
    pin_hash    TEXT NOT NULL,
    updated_at  TEXT NOT NULL
);
```

The column set is approved (DEC-006); the exact DDL is the expected form and will be fixed in the Implementation Guide v2. Every event (alert or log event) is inserted as `PENDING` and marked `SENT` on PUBACK. `PENDING_NOTIFICATION` no longer exists. When the database is recreated, `id` starts again at 1; the app therefore drops exact duplicates and orders its list by arrival (DEC-006, 6.2 a). No backup file (DEC-001). The default PIN hash for a new database is read from `/etc/vigil/secrets/default_pin_hash` (root-only).

### E.6 Linux device driver `vigil_pwm` (DEC-002)

- **What stays (D-06, D-07):** platform driver, consumer of `pwm-bcm2835`; own device-tree overlay (GPIO18 = PWM0 ALT5 = speaker; GPIO13 = PWM1 ALT0 = red LED; `compatible = "esrg,vigil-pwm"`); character device `/dev/vigil_pwm`; per-channel hrtimers; mutex for writers; hardware protection (speaker duty ≤ 50 %, output LOW when stopped); `release()` is fail-loud (outputs keep their last state if the application dies; the watchdog then resets the Pi). Never load `pwm-2chan` with it; never enable `dtparam=audio=on`.
- **What changed:** `write()` takes a **sequence of up to 8 steps** `{frequency_hz, duty, duration_ms}` (frequency 0 = silent step) and a **repeat** flag for one channel. The kernel has no built-in patterns and no base/one-shot rule. Tones and the output policy live only in user space (`OutputPolicy`, `AlarmOutputs`). The user space composes "beep over warning": it sends the beep, then re-sends the warning when a deadline expires.
- **Errors:** `EINVAL` (bad size, channel, step count, duty or frequency out of range), `EFAULT`, `EIO` (PWM apply failed), `EINTR`.
- **Kernel facts (RPi kernel 6.12.61, from Guide §8.1):** `pwm-bcm2835` is atomic, so `pwm_apply_atomic()` may be called from the hrtimer callback; `pwm_apply_might_sleep()` in process context; `class_create()` takes one argument; platform `.remove` returns `void`; `MODULE_LICENSE("GPL")` is required.
- **UAPI (DEC-012, 12.4):** at most 8 steps per sequence; a write with 0 steps stops the channel (output LOW); one repeat flag. The exact struct layout is written in the Implementation Guide v2.
- **First technical step:** overlay + `probe()` + one constant tone working on the Pi, as early as possible.

### E.7 Application lifecycle and supervision

**Start-up (`main`, DEC-001/003a/004/005b):**

1. Read `/etc/vigil/broker.conf` (`-c`) and `/etc/vigil/vigil.conf` (`-C`, defaults if missing); `openlog`; `mlockall`; block signals. Validate `home_id` (DEC-015, 15.3): if it is invalid, log it to syslog and do not start the MQTT connection; everything local starts normally, `CommThread` keeps storing events as `PENDING`, and the RGB LED shows the broker as lost. After the configuration is fixed and the service restarted, the pending events are sent in order.
2. Create the EventQueue and the CommQueue.
3. Open `/dev/watchdog` early (15 s timeout).
4. Open `/dev/vigil_pwm` (failure → FAULT(SPEAKER)); request each GPIO line separately (failure → FAULT for that component); open the INA219 (failure → FAULT(POWER_MONITOR)). Never refuse to start.
5. Database: open + `PRAGMA quick_check` → OK: load the PIN hash. Failure: rename the file to `vigil.db.corrupt-<time>`, create a new database, store the default PIN hash, FAULT(DATABASE), syslog "PIN change required". First boot (no file): create it with the default hash, no FAULT.
6. Read the last `ARMED`/`DISARMED` event (by `id`) → initial intrusion state (DISARMED if the database was recreated). The restore logs no event. The first reading of every `DebouncedInput` is its baseline, so a door already open at boot does not trigger.
7. Start the five threads, consumers first: `CommThread`, `AlarmControllerThread`, then `PowerMonitorThread`, `SafetySensorsThread`, `LocalInputThread` (DEC-012, 12.5).
8. Power state decided silently by the first 3 samples; MQ-2 warm-up (60 s default) runs in parallel, without smoke events.
9. `main` enters its 1 s supervision loop.

**Supervision.** `HealthMonitor` compares every thread's last heartbeat with its timeout (1 / 1 / 3 / 3 / 10 s). All fresh → feed `/dev/watchdog`; otherwise stop feeding and the Pi resets after 15 s. An unexpected error (exception or impossible state) in a thread is logged by `WorkerThread`, the thread stops sending heartbeats, and the watchdog resets the Pi, which comes back in the restored state. There are no thread restarts.

**Clean stop (SIGTERM from `S99vigil`).** Stop the threads, turn outputs off, close the database, close the watchdog with the magic character (disables it), and close the broker connection **without sending DISCONNECT**, so the broker publishes the Last Will at once (DEC-012, 12.1).

**Safe state** (≤ 6.4 V): see the Power FSM above; if mains returns before the HAT cuts power, `main` reboots the Pi (DEC-012, 12.2). Reason: with `poweroff`, if mains returned before the battery was empty, the halted Pi could stay off indefinitely (Work Log issue 7). **Condition:** the HAT must cut its output at full discharge and restore it when the charger returns; if the test fails, the decision returns to `poweroff` through a new decision.

### E.8 Configuration parameters

| Parameter | Value | Status |
|---|---|---|
| Home identifier `home_id` | `vigil.conf`, default `home1`; 1–32 characters `[a-z0-9-]` (no `/`, `+` or `#`); used in the topics `vigil/<home_id>/…`; the Pi `client_id` and MQTT username `vigil-device-<home_id>` are derived from it (no longer written in `broker.conf`) | Approved (DEC-015) |
| Exit / entry delay / silence period | 60 s / 30 s / 10 min (`vigil.conf`) | Approved |
| Wrong-PIN limit / lockout | 5 / 5 min | Approved |
| PIN length / entry timeout | 4–6 digits / 10 s | Approved / default |
| Sampling periods | 15 ms keypad + button, 100 ms IR + MQ-2, 1 s INA219 | Approved (DEC-003a) |
| Confirmation | button 2 reads; IR 2 reads; MQ-2 5 reads; INA219 3 samples | Approved |
| MQ-2 warm-up | 60 s | Default — to be measured |
| Low battery / critical | ≤ 6.7 V / ≤ 6.4 V, 3 samples, on battery | Approved (DEC-005a/b) |
| Mains lost / back | current < −100 mA / > +50 mA | Proposal — validate on the board |
| Heartbeat timeouts | 1 / 1 / 3 / 3 / 10 s (LocalInput, SafetySensors, PowerMonitor, Controller, Comm) | Approved (DEC-004) |
| Watchdog | 15 s (bcm2835 maximum) | Approved |
| MQTT keep-alive Pi / app | 30 s / 60 s | Approved |
| Reconnect backoff | 1 s → 60 s for every failure, logged once per episode | Approved (DEC-006, 6.3 b) |
| `cmd` Message Expiry / app timeout | 30 s / 32 s | Approved / default |
| Outbox pacing | ≤ 5 publishes/s | Approved |
| App event list | last 20 events, arrival order | Approved (DEC-006) |
| PBKDF2 iterations | 20 000 (target 20–50 ms per check) | Default — to be measured |

---

## F. SOLID design principles

### F.1 How SOLID is applied in this project

- SOLID is a course requirement (C-3) and is applied **pragmatically**: every abstraction must have a concrete benefit (testability on the laptop, real/simulated substitution, isolation of a C API). No abstraction exists only to demonstrate a principle.
- Threads and queues are judged on timing; classes are judged on responsibilities. Rule: **merge threads, never responsibilities** (DEC-003a/b). The old rule "one thread = one class = one responsibility" (D-03, old slide 70) is superseded.
- Every future architectural proposal must include a **SOLID Impact Analysis**: principles involved; whether each is improved, preserved or violated; a better alternative if one exists; trade-offs; defensibility in front of the professor.

### F.2 Evidence map (DEC-009; English text for slide 70)

| Principle | Where | Evidence |
|---|---|---|
| S — Single responsibility | Threads only schedule; `AlarmLogic` decides; `OutputPolicy` owns the output rules; `EventStore` / `OutboxPublisher` / `MqttLink` / `Protocol` | A new tone changes only `OutputPolicy`; a new message format changes only `Protocol` |
| O — Open/closed | `IGpioInput`, `IPwm`, `IPowerSensor`, `IMessageTransport`; `DebouncedInput` configured by parameters; tones as step sequences | New sensor = new `DebouncedInput` instance (plus one FAULT enum value); new tone = new sequence, driver unchanged; simulated hardware = new class, callers unchanged; RGB active level changed by configuration only (DEC-007) |
| L — Liskov substitution | `WorkerThread` subclasses; real and simulated hardware classes | Same contract tests pass on simulated (L1/L2) and real (L3) hardware; one heartbeat and error policy for all threads (no thread restarts) |
| I — Interface segregation | `IGpioInput` / `IGpioOutput` / `IPwm` / `IPowerSensor`; `EventStore` vs `PinStore`; `CommCommand` as small structs | No client depends on methods it does not use; the union-like command structs are gone |
| D — Dependency inversion | `logic` depends on nothing; devices and outputs depend on hardware interfaces; `OutboxPublisher` on `IMessageTransport`; `main` is the composition root | `AlarmLogic` is tested on the laptop with no hardware and injected time |

In the presentation, one short SOLID note appears on the slides where a principle is visible (package/class diagram, IR interface, threads, storage/Comm, driver, test strategy), and slide 70 summarises them (DEC-009, 9.1).

### F.3 Deliberate trade-offs (documented here; not presented, DEC-009 9.3)

| Where SOLID is not applied | Why |
|---|---|
| Alarm rules (FSMs) are `switch` statements over enums; adding an event type edits `AlarmLogic` | The rules change rarely; a State pattern would roughly triple the classes. **Never claim "new event = new EventType value" as OCP evidence** — it is false |
| `EventStore`, `PinStore` without interfaces | In-memory SQLite is a faithful test double |
| `AlarmOutputs` without an interface | Its dependencies (`IPwm`, `IGpioOutput`) are already abstract |
| `PinHasher` without an interface | Pure computation; tested with the real implementation |
| `Watchdog` without an interface | Only `main` uses it; an empty implementation is chosen at compile time for simulation; the decision logic (`HealthMonitor`) is pure |

If the professor asks why one of these classes has no interface, answer with this table.

### F.4 Design patterns actually used (DEC-009, 9.2; English text for slide 68)

| Pattern | Where | Problem it solves |
|---|---|---|
| Producer–Consumer | Input/sensor/power threads → EventQueue → controller; controller + Paho callbacks → CommQueue → `CommThread` | Many asynchronous sources, one deterministic decision maker; the controller never blocks on disk or network |
| Monitor Object | `BlockingQueue<T>` (PI mutex + monotonic condition variable) | Thread-safe hand-over without priority inversion; wait until the next deadline |
| Template Method | `WorkerThread`: wait → step() → heartbeat; one error policy | Same life cycle, heartbeat and error handling for the 5 threads |
| Pure FSM logic | `AlarmLogic` + `IntrusionFsm`, `FireFsm`, `PowerFsm`, `PinGuard` | Behaviour unit-tested on a laptop |
| Adapter | `GpioLines`, `Ina219`, `PwmDevice`, `MqttLink`, `Database` | Hide C APIs; a libgpiod v1 → v2 change touches one class |
| Composition Root | `main()` builds and injects real or simulated objects | Same code on the Pi and on the laptop |
| Transactional Outbox | `EVENT_LOG.status/sent_at` + `OutboxPublisher` | No event lost between the database and MQTT delivery |
| Watchdog | `main` feeds `/dev/watchdog` only while every heartbeat is fresh | A hung thread or unexpected error resets the Pi; last state restored |

Removed from the old slide 68: Command (union structs), Active Object, Observer (`IEventSink`), Supervisor restarting threads.

---

## G. Decision history

### G.1 Index of the simplification review (8–9 Nov 2026)

All decisions below were approved by David Cardoso in the conversation of 8–9 Nov 2026 (DEC-014 to DEC-016: 14 Nov 2026), except DEC-012, which David delegated to Claude. **[OPEN]** Ana Sofia Cunha's review of them is not recorded.

| ID | Topic | Choice | Status |
|---|---|---|---|
| DEC-001 | Scope of the deliverable | B + M1 (postponed items not presented) | Approved; shutdown item amended by DEC-005b |
| DEC-002 | Device driver strategy | B — generic sequence player | Approved |
| DEC-003a | Concurrency model | B' — 5 threads + main, deadline-based waiting | Approved |
| DEC-003b | Class responsibilities and dependencies | B — merge threads, never responsibilities | Approved |
| DEC-004 | Supervision and error handling | 4.1 b, 4.2 b, 4.3 (FAULT per hardware component + DATABASE at boot), 4.4 a, 4.5 defaults | Approved |
| DEC-005a | Low-battery threshold | 6.7 V | Approved |
| DEC-005b | Remaining power management | 5b.1 c, 5b.2 confirmed, 5b.3 b (safe state), 5b.4 a (DISCONNECT 0x04) | Approved; 5b.3 conditional on a HAT test; **5b.4 replaced by DEC-012 (12.1)** |
| DEC-006 | Remote protocol and app scope | 6.1 b', 6.2 a, 6.3 b, 6.4 a | Approved |
| DEC-007 | Open hardware | 7.1 b, 7.2 b, 7.3 b (+ commercial box) | Approved |
| DEC-008 | Test strategy and acceptance set | 8.1 b, 8.2 B, 8.3 b (method as a note only) | Approved |
| DEC-009 | SOLID evidence map | 9.1 B + notes along the deck, 9.2 b, 9.3 not presented | Approved |
| DEC-010 | Intrusion sensor vs the Analysis "movement" requirement | A' — refine the requirement, keep the 2 IR sensors | Approved |
| DEC-011 | Source of truth and documentation consolidation | Master Context as SSOT; plan F1–F8 | Approved (9 Nov) |
| DEC-012 | Open technical points (delegated to Claude, criterion: simplification) | Planned stop without DISCONNECT; reboot if mains returns in safe state; SHUTDOWN details null; driver UAPI N = 8, empty sequence = stop; consumers start first | Decided 9 Nov under David's delegation |
| DEC-013 | Phase 4 presentation decisions | P4-1…P4-11 (71 slides, no annex, no divergence notes, edits only in authorized batches); Gantt not updated | Approved (9 Nov) |
| DEC-014 | Intrusion `ALARM_SILENCED` log event | B — removed; `ALARM_SILENCED` exists only with details `FIRE` (alert) | Approved (14 Nov) |
| DEC-015 | Readiness for several homes | B — Pi and broker ready for several homes (`home_id`, per-home users and ACL); app for one home; 15.1–15.7 | Approved (14 Nov) |
| DEC-016 | App MQTT session handling | 16.1 A — the app subscribes after every CONNACK; 16.2 A — session expiry 7 days requested, the granted value is used | Approved (14 Nov) |

### G.2 Decision records

#### DEC-001 — Scope of the deliverable

- **Problem.** About 12 features had been added during Design beyond the Analysis requirements, all at the same priority. Without a closed scope the specification kept growing (deck from 61 to 88 slides between 2 and 8 Nov).
- **Original design.** Scope = everything in the Implementation Guide.
- **Alternatives.** A — minimum (Analysis requirements only); B — balanced; C — keep everything.
- **Choice: B + M1.** Postponed features are not shown in the Design presentation.

| Category | Items |
|---|---|
| Keep | All Analysis requirements; PIN lockout; second IR sensor |
| Simplify | Watchdog fed only with fresh heartbeats, no thread restarts; LOW_BATTERY and, below the critical threshold, safe state (originally log + sync + poweroff, amended by DEC-005b); degraded start + one FAULT, no RGB blink, no retries; database failure → new database with the default PIN + FAULT |
| Postpone (not presented) | Battery % in 10 % steps; power alerts at boot; database backup/restore; `get_events` history; reconnection summary in the app |
| Remove | Wi-Fi loss detection in ≤ 2 s by netlink (`LinkMonitor`) |

- **Reasons.** Keeps every Analysis commitment plus the cheap, defensible additions; cuts work and presentation volume.
- **SOLID.** L improves (no thread restarts, every subclass honours the `WorkerThread` contract); D improves (no `get_events` → no Network→Storage concrete dependency); S preserved.
- **Consequences.** Wi-Fi loss now detected by the broker keep-alive (≤ 60 s); no remote history; no database backup. The loss of log events generated offline was later solved by DEC-006 (6.1 b').
- **Affected.** Guide §5.4, §6.5, §6.9, §7.1, §7.9, §9.5, §10.4, §10.8, §14, §16, §17; slides listed in the deck change list; GUI document; class diagram.

#### DEC-002 — Device driver strategy

- **Problem.** Output policy lived in three places (kernel, `AlarmLogic`, OutputThread); patterns were fixed in the kernel (changing a tone meant rebuilding the module); the driver is on the critical path of all sound and red-LED output and had not been started.
- **Original design.** `vigil_pwm.ko` with 8 fixed patterns, base + one-shot rule, hrtimer stepping.
- **Alternatives.** A — keep; B — generic sequence player; C — minimal driver (frequency, duty, enable); D — input driver with interrupts, sound and LED through sysfs.
- **Choice: B.** The professor values no particular kind of driver, so the simplest design that keeps kernel timing was chosen.
- **What changes.** `write()` takes up to N steps {frequency, duty, duration} + repeat; no patterns or base/one-shot rule in the kernel; tones and output policy only in user space; user space restores the warning after a beep with a deadline.
- **SOLID.** S improves (`OutputPolicy` is the only owner of output policy); O improves (a new tone is data; driver and UAPI unchanged); L improves (the simulated `IPwm` only "plays a sequence"); I and D preserved.
- **Consequences.** User space must restore the base output after a beep; the controller no longer needs an OutputThread because `write()` returns at once. First technical step: overlay + `probe()` + a constant tone on the Pi.
- **Affected.** Deck slides 65–67, 80; Guide §8.3–8.4.

#### DEC-003a — Concurrency model

- **Problem.** 9 threads, 4 queues, epoll + eventfd + 4 timerfds, `IEventSink`, cross-thread database reads. After DEC-001 and DEC-002 several threads no longer had a reason to exist.
- **Alternatives.** A — 3 threads; B — 6 threads; B' — 5 threads; C — keep 9.
- **Choice: B' with deadline-based waiting** (table in §E.1).
- **Reasons for B' over B.** In B the first IR glitch triggers at once, while B' filters it with 2-read confirmation; B uses the libgpiod event API, where v1 (Pi) and v2 (laptop) differ most; in B' the simulation only returns levels, so it is more faithful.
- **Removed.** OutputThread; Storage/Network split; `IEventSink`; SupervisorThread (role moved to `main`); separate IR and MQ-2 threads; the cross-thread `isSmokePresent()` read; epoll, eventfd and timerfds.
- **SOLID.** Depends on DEC-003b; threads become thin loops; deadlines in logic with injected time remove the dependency on OS timers (D improves). `IGpioInput` loses `waitEdge`.
- **Trade-offs accepted.** No hardware-edge-activated task; no FIFO 70 band; IR detection up to ≈ 200 ms; a failing sensor must not stop the others.
- **Consequences.** Contradiction CON-02 ("any edge" vs "rising edge") disappears.
- **Affected.** Slides 16, 43–48, 54, 56–59, 62–64, 68–69, 77, 80, 85; Guide §5, §6.2, §7.

#### DEC-003b — Class responsibilities and dependencies

- **Problem.** The class model followed "one thread = one class = one responsibility" (D-03, old slide 70). After DEC-003a threads no longer matched responsibilities. Concrete issues: `AlarmControllerThread` held all state and 6 handlers while `AlarmLogic` only had `handle()`; `NetworkMQTTThread` had about six responsibilities; `StorageThread` built alert JSON; concrete dependencies between threads; union-like command structs.
- **Alternatives.** A — classes follow threads (~20 types); B — merge threads, never responsibilities (~35 types); B− — B with `AlarmLogic` as one class (~30); C — maximal SOLID: interface per component, State pattern, handler registry (~50).
- **Choice: B** (packages in §E.3). B− is a declared fallback that needs a new decision.
- **SOLID.** S: threads only schedule; no god classes; output arbitration in one place. O: sensors, tones, real vs simulated hardware, broker; deliberately not the alarm rules. L: `WorkerThread` contract and real/simulated substitutability, checked by the same tests. I: capability interfaces; storage split by client. D: `logic` independent; declared trade-offs (§F.3).
- **Consequences.** Class count stays near 35; what shrinks is threads, queues, inter-thread dependencies and multi-reason classes. The presentation gets three views (concurrency, simplified class diagram by package, full diagram in the annex [superseded by DEC-013, P4-2: no annex; the full diagram is shown on 2 detail slides]). Names change, which conflicted with the Guide rule "keep the names" (CON-08, resolved by DEC-011).
- **Affected.** Slides 16–23 (responsible thread), 43–46, 68, 70, flowcharts; Guide §3.2, §5.4–5.5, §5.7, §7; "Class Diagrams for Slides (PlantUML)" superseded.

#### DEC-004 — Supervision and error handling

- **Problem.** State after a reboot, handling of unexpected errors and degraded behaviour per component were undefined; the error-handling slide had inconsistencies (CON-06: "database rebuilt", "IR or MQ-2 stuck" without any stuck-sensor detection, supervisor restarts).
- **Original design.** Always start DISARMED (Analysis slide 15, Guide G10); supervisor restarting threads; FAULT for 6 components with retries; database backup, restore and RAM outbox.
- **Choices.**
  - **4.1 b — restore the last ARMED/DISARMED state** at boot (ARMED, ENTRY_DELAY and INTRUSION_ALARM return as ARMED; others as DISARMED; DISARMED if the database was recreated; no event logged; first sensor reading is the baseline). ≈ 30–40 lines, 3–4 tests. **Diverges from the Analysis** (reason prepared for oral questions, not shown; DEC-013, P4-6).
  - **4.2 b — unexpected errors:** log, stop heartbeats, watchdog reset; policy only in `WorkerThread`. Expected device errors follow 4.3.
  - **4.3 — FAULT only for hardware components (IR1, IR2, MQ2, KEYPAD, SPEAKER, POWER_MONITOR) + DATABASE at boot; runtime database failure → syslog only; broker loss is not a fault.** Once per component per boot, no retries; each GPIO line requested separately. ≈ 95–110 lines, 8–10 tests. (David first asked about removing FAULT completely, then proposed FAULT only for components; this was adopted.)
  - **4.4 a — MQ-2 failure is signalled only by the FAULT alert** (no local chirp).
  - **4.5 — heartbeat timeouts** 1 / 1 / 3 / 3 / 10 s; watchdog 15 s, disabled on clean stop (magic close).
- **SOLID.** D preserved (initial state injected by `main`); L reinforced (one error policy in `WorkerThread`); devices only report faults as events; no `IErrorHandler`.
- **Accepted risks.** ≈ 1–2 min without full protection during a reboot (watchdog + boot + MQ-2 warm-up), to be measured; alerts lost during a runtime database failure; with a broken keypad a fire alarm cannot be silenced until reboot; a broken MQ-2 with no network is not visible locally; lockout counter resets on reboot; `event_id` restarts with a new database (handled by DEC-006, 6.2 a).
- **Affected.** Slides 33, 49, 53, 64, 86; Guide §6.8, §7.1, §14; GUI document (FAULT texts).

#### DEC-005a — Low-battery threshold

- **Problem.** With M1 the battery percentage is no longer presented, so "≤ 30 %" made no sense.
- **Choice.** LOW_BATTERY at **≤ 6.7 V for 3 consecutive samples, only on battery**; critical stays at 6.4 V.
- **Risk.** "30 % ≈ 6.7 V" came from a linear mapping; Li-ion discharge is not linear. Measure the time between 6.7 V and 6.4 V in the autonomy test.

#### DEC-005b — Remaining power management

- **Choices.**
  - **5b.1 c** — only LOW_BATTERY carries a value (the voltage, e.g. `"6.68 V"`); POWER_LOST and POWER_RESTORED have `details` null.
  - **5b.2 confirmed** — the first 3 samples silently decide mains or battery at boot; mains assumed until then.
  - **5b.3 b — safe state, no power-off** (replaces the DEC-001 power-off item, reopened with David's authorization because of Work Log issue 7: a halted Pi may not restart when mains returns). ≈ 15–20 lines. **Condition:** verify that the HAT cuts and restores its output; if not, return to power-off through a new decision.
  - **5b.4 a — planned stop sends DISCONNECT with reason 0x04** ("Disconnect with Will Message") so the broker publishes the Last Will at once; on battery exhaustion the Last Will arrives by keep-alive (≈ 45 s). ≈ 2–5 lines, 1 test with Mosquitto.
- **Original design.** Battery % in alerts; power alerts at boot; coordinated power-off with "offline" + reason and DISCONNECT 0x00.
- **SOLID.** Neutral: `PowerFsm` produces "enter safe state"; `CommThread` closes `EventStore`; DISCONNECT 0x04 lives inside `MqttLink`.
- **Consequences.** In safe state the batteries discharge until the HAT protection cuts; the app can no longer show the reason for "offline" (LOW_BATTERY and SHUTDOWN give the context).
- **Superseded in part.** The 5 Nov laptop prototype reported that Paho C++ did not pass the 0x04 reason code to the broker. 5b.4 was therefore replaced by DEC-012 (12.1): the planned stop closes the connection without any DISCONNECT, which makes the broker publish the Last Will at once.

#### DEC-006 — Remote protocol and app scope

- **Problem.** Log events generated offline were lost (no `get_events`); `event_id` restarts with a recreated database; app scope and reconnection rule to confirm.
- **Original design.** Only alerts in the outbox (`PENDING_NOTIFICATION`); app ordered and de-duplicated by `event_id`; two reconnection rules (backoff, and every 5 min for authentication errors).
- **Choices.**
  - **6.1 b' — every event goes through the outbox, which becomes two columns of `EVENT_LOG`** (`status`, `sent_at`); `PENDING_NOTIFICATION` removed (3 → 2 tables, **divergence from the Analysis ERD**; reason prepared for oral questions, not shown — DEC-013, P4-6); `Protocol` picks the topic (`alert` or `event`) from the event type; ≈ 20–30 fewer lines.
  - **6.2 a — the app drops exact duplicates (full message) and orders its list by arrival**; protocol unchanged; ≈ 10–15 Dart lines.
  - **6.3 b — one reconnection rule:** backoff 1 → 60 s for every failure, logged once per episode.
  - **6.4 a — reduced app scope** (§I.7).
- **SOLID.** S improves (`OutboxPublisher` has one sending policy, `Protocol` chooses the topic, `EventStore` has fewer methods); no new abstractions.
- **Risks.** ERD differs from the Analysis; the foreground service with MQTT in a separate isolate remains the riskiest part of the app (prototype early).

#### DEC-007 — Open hardware

- **Choices.** **7.1 b** — one low-side NPN per RGB cathode, anode at 5 V, GPIO HIGH = on (resolves CON-03; the deck already had this circuit, the Guide and Hardware Context did not). **7.2 b** — universal IR interface: Schottky in series + external 10 kΩ pull-up to 3.3 V (works for open-collector and 5 V outputs; measurement still needed). **7.3 b** — breadboard bring-up, then a protoboard interface board in an enclosure; IR on frames, MQ-2 high; **enclosure = commercial plastic box from a local shop** (David, 9 Nov).
- **Main alternatives rejected.** Direct GPIO sink for the RGB (5 V node at a GPIO, red channel not fully off, LEDs lit while GPIOs float at boot); deciding the IR circuit only after measuring (blocks the schematic); breadboard-only demonstration (fragile) or a custom PCB (time).
- **SOLID.** Neutral; active levels are parameters (O example).
- **Consequences.** Purchase list in §D.6; purchases on the critical path; IR LOW margin to measure.

#### DEC-008 — Test strategy and acceptance set

- **Problem.** ≈ 75 tests in 10 slides with no ID, objective, level or link to requirements; ≈ 12 invalidated by decisions; missing tests for new behaviour; real-time targets without a method; the Revision Plan proposed ≈ 110 tests.
- **Choices.** **8.1 b** — 3 levels (L1 laptop unit, L2 laptop integration, L3 Raspberry Pi). **8.2 B** — 1 strategy slide + 24 acceptance tests (ID · Objective · Stimulus · Expected result · Level · Req.); full catalogue in the report (DEC-013, P4-2). **8.3 b** — software timestamps + external measurement, **kept as a note only**: the presentation shows targets and load conditions, without instruments or laboratory references.
- **SOLID.** Tests are the evidence for D (logic without hardware) and L (same contract tests on simulated and real implementations).
- **Consequences.** Test slides 10 → 5; Guide §16 and §19 to update; the Revision Plan's extra rows and "Method" column are superseded. Detail in §K.

#### DEC-009 — SOLID evidence map

- **Problem.** Old slide 70 had claims made false by the decisions, and one claim that was already false ("new event = EventType value"); slide 68 listed four patterns that no longer exist; nothing said how to verify the claims.
- **Choices.** **9.1 B** — evidence map (Principle · Where · Evidence) **plus short SOLID notes along the presentation**; **9.2 b** — patterns slide with only the 8 patterns that exist; **9.3** — where SOLID is not applied is **not presented** (reasons kept in DEC-003b and §F.3 for oral answers).
- **Consequences.** Slides 68 and 70 rewritten; about 6 notes; the simplified class diagram must show every name the map uses; the map doubles as a code-review checklist (Guidelines).

#### DEC-010 — Intrusion sensor and the Analysis "movement" requirement

- **Problem.** The Analysis requires "monitor movement" and its event table names a PIR sensor; the design uses 2 IR reflective sensors on doors/windows, with no justification in the deck and `EVT_MOTION_DETECTED` still named after motion.
- **Alternatives.** A — rewrite the requirement and rename events; A' — refine the requirement without changing it; B — add one PIR to the 2 IR; C — replace the IR with a PIR.
- **Choice: A'** (after David clarified that the goal is to detect the movement of the door/window and that only the 2 IR are kept). The requirement wording stays; in the design "movement" = movement of a monitored door/window leaf (closed = LOW, open = HIGH, closed → open transition confirmed in 2 reads); the event keeps the name `EVT_MOTION_DETECTED` (source IR1/IR2) for traceability; app text "Intrusion – door/window moved (IR1)"; one justification line in the deck [superseded by DEC-013, P4-6: not shown; the reason is prepared for oral questions].
- **Defence.** Detection at the entry point before the intruder is inside (consistent with the entry delay); no false alarms from pets, heat or sunlight; < 2 ms response without a PIR blocking time or warm-up. Likely question "why not a reed switch?": available hardware, contactless; a reed switch would plug into the same `DebouncedInput` with no code change.
- **Declared limitations.** Only the two instrumented doors/windows; a door open at arming is detected only after it closes and reopens.
- **Consequences.** No hardware change; CON-19 ("object in front of IR") resolved.

#### DEC-011 — Source of truth and documentation consolidation

- **Problem.** CON-08: the Implementation Guide (§0.1, §2.2) and the repo `CLAUDE.md` declared the Guide authoritative and forbade redesign, while ten approved decisions changed the design. CON-01: the Project instructions described an even older design. 13 overlapping design documents, two diverging copies of the Guide, a 61-slide deck in the Project while the current deck has 88 slides.
- **Choice (approved 9 Nov, recommendations F1–F8).**
  - This document is the SSOT; precedence in §0.1.
  - The repository `~/VigilSystem` is the canonical copy; the Claude Project holds a mirror with the same paths (`VigilSystem/…`).
  - Superseded documents are archived in the repo (`docs/archive/`, with a banner) and removed from the Project, each removal confirmed by David.
  - The Implementation Guide gets a banner and a section-validity map now; a v2 rewrite follows in approved batches.
  - The Portuguese decision log is archived once its content is here.
  - The earlier standing authorization to update the decision log automatically is **revoked**: Claude proposes documentation changes as soon as a decision is complete and applies them only with approval.
  - DEC-005b 5b.4 recorded as open; later decided by DEC-012 (12.1).
  - The global working rules are re-drafted for approval (the original list was lost).
  - A hardware specification v2 is kept as its own document, summarised in §D.

#### DEC-012 — Open technical points decided under delegation

- **Context.** On 9 Nov David approved the Master Context draft and delegated to Claude the open technical points, with simplification as the criterion ("aprovo que decidas tu tendo em conta a simplificação do projeto"). David may revert any of them.
- **12.1 Planned stop (replaces DEC-005b 5b.4 a).** On SIGTERM the application does **not** send an MQTT DISCONNECT: it closes the network connection (or simply exits). MQTT 5 requires the broker to publish the Will Message when the connection closes without a DISCONNECT packet, so the app shows "offline" at once. The Will Delay Interval stays 0.
  - Why: no code, no dependence on Paho passing reason 0x04 (prototype evidence), same visible result.
  - Rejected: keep 0x04 (feasibility doubtful); retained "offline" + DISCONNECT 0x00 (brings back a message and code removed by DEC-005b).
  - Verification: AT-15 with Mosquitto and with EMQX.
- **12.2 Mains returns while in SAFE_STATE.** `main` performs a controlled reboot (`sync()` + `reboot(RB_AUTOBOOT)`); the system recovers through the normal boot path (state restore, DEC-004; silent initial power state, DEC-005b).
  - Why: one recovery path (the same as after a watchdog reset); no "reopen the database" path.
  - Trade-off: ≈ 1–2 min without full protection (boot + MQ-2 warm-up) in a rare situation; no POWER_RESTORED alert after that reboot (the app sees SHUTDOWN, then "online" again).
- **12.3 `SHUTDOWN` alert `details` = null.** It has a single cause; consistent with 5b.1 c (only LOW_BATTERY carries a value).
- **12.4 Driver UAPI.** At most **8 steps** per sequence (the longest planned sequence, the wrong-PIN blink, has 6); a write with **0 steps stops the channel** and leaves the output LOW; one repeat flag per sequence. The exact struct layout goes into the Implementation Guide v2.
- **12.5 Thread start order.** Consumers first (`CommThread`, `AlarmControllerThread`), then producers (`PowerMonitorThread`, `SafetySensorsThread`, `LocalInputThread`). The queues already exist before any thread starts; this order only avoids events waiting without a consumer.
- **SOLID.** Neutral; 12.1 removes a dependency of `MqttLink` on a library feature.
- **Affected.** §E.4, §E.6, §E.7, §I.4, §I.5, §N.1; deck slides 30, 32, 40, 51, 64, 65; AT-12 (optional extra case) and AT-15.

#### DEC-013 — Phase 4 presentation decisions

- **Problem.** After DEC-001…DEC-012 the 88-slide deck had to be restructured: order, annex, slide count, how divergences from the Analysis are shown, and who edits slides and diagrams.
- **Decisions (David, 9 Nov 2026; full table in `docs/presentation/DESIGN_DECK_CHANGES.md` §2).**
  - **P4-1 A** — keep the current order; apply only the decided changes.
  - **P4-2 A** — no annex; nothing is moved out of the deck. Consequences: the full class diagram stays in the deck (simplified overview + 2 detail slides by package); the full test catalogue goes to the report (allowed by DEC-008, 8.2).
  - **P4-3** — hardware specification (old 6–11) and interface tables (old 16–23) condensed from 14 to **8 slides**: 3 specification slides + 5 interface slides, two related interfaces per slide when they share a thread or a driver. This was Claude's interpretation of "4 seems too few"; David confirmed it.
  - **P4-4** — keep all 17 data and communication slides (old 26–42), updated.
  - **P4-5 A** — one flowchart per slide.
  - **P4-6** — divergences from the Analysis are **not shown**: no summary slide and no justification lines. The reasons stay in §B.8 and §G for oral questions.
  - **P4-7 B** — one end-to-end walkthrough slide.
  - **P4-8 B** — Claude drafts the `.drawio` diagrams (with SVG and PNG exports); the team reviews and adjusts them.
  - **P4-9 C** — Claude edits text and tables, the team does layout and images; Claude acts **only when David authorizes a batch, and only on the slides named in it**.
  - **P4-10 B** — the connectivity LED FSM slide is removed; the RGB colour table and the network state go to the RGB interface slide.
  - **P4-11 B** — GUI: states table + one mock-up slide.
  - **Gantt** — not updated; the slide is kept as it is.
- **Result.** 71 slides, no annex (88 − 3 − 3 − 1 − 4 − 2 − 5 + 1 = 71).
- **SOLID.** Neutral (presentation only). The six SOLID notes of DEC-009 go on the slides listed in the change plan §3.3.
- **Affected.** §B.8, §J, §K.2, §L.4, §L.5, §M; Guidelines §7; `DESIGN_DECK_CHANGES.md` (P4-3 marked confirmed, §9 marked applied).

#### DEC-014 — Intrusion `ALARM_SILENCED` log event

- **Problem.** A valid keypad PIN during an intrusion alarm logged `DISARMED (LOCAL)` and then `ALARM_SILENCED (INTRUSION)` (D-41). (1) The second event repeats the first: an intrusion alarm has no silence period, so `INTRUSION_ALARM` followed by `DISARMED` already says that the alarm was stopped. (2) The name suggests a silence period like the fire one, which caused confusion during the review of the log-event slides (14 Nov). (3) It was ambiguous whether a remote disarm also logged it: the remote-disarm row of §E.4 said "as above", while Implementation Guide §6.6 said keypad only.
- **Current solution (before this decision).** §E.4: "from INTRUSION_ALARM also `ALARM_SILENCED` (INTRUSION)"; §I.4: `ALARM_SILENCED` on topic `event` with details `INTRUSION`; order when one keypad PIN stops both alarms: `DISARMED` → `ALARM_SILENCED (INTRUSION)` → `ALARM_SILENCED (FIRE)`. It was a Design addition: the Analysis (deck A, slide 15, PIN Submission) only says that a correct PIN "silences the active alarm… and notifies the app".
- **Alternatives.** A — keep it and clarify the remote case in §E.4; B — remove it, keep `ALARM_SILENCED` only for `FIRE`.
- **Decision: B** (David, 14 Nov 2026) — Status: approved.
- **What changes.** Stopping an intrusion alarm (keypad or app) logs only `DISARMED` (LOCAL or REMOTE). `ALARM_SILENCED` exists only with details `FIRE`, on topic `alert`. When one keypad PIN stops both alarms, the order is `DISARMED` (`event`) → `ALARM_SILENCED (FIRE)` (`alert`). The `EVENT_LOG` CHECK list does not change (the fire version still uses `ALARM_SILENCED`).
- **SOLID.** Neutral: one action less in `IntrusionFsm` and one value less for `Protocol`; no abstraction changes.
- **Consequences.** One event type/detail combination less to implement and test; the remote-disarm ambiguity disappears; the app list shows "Intrusion alarm" then "Disarmed" instead of an extra "Intrusion alarm silenced" line. No Analysis requirement is affected (FR-12 Event Logging is still met by `DISARMED`).
- **Affected.** §E.4 (Intrusion FSM rows for keypad PIN and remote disarm), §I.4 (event table), §G.3 (D-41), §H (row 25), §J.1 (DR-8). Deck: Log Events (2/2) (new 29), Real-Time Alerts (1/3) note of Fire Alarm Silenced (new 25), and any slide that shows the event (Intrusion FSM 43, PIN Validation flowchart 48, GUI 62–63 — to be checked). Implementation Guide §6, §7 event table, §10.4/§10.7, app texts table, §16 and §17 (marked as removed, 14 Nov); `DESIGN_DECK_CHANGES.md` v1.2 (§3.1, §5, §8).
- **Sources.** Conversation of 14 Nov 2026 (review of the log-event message specification).

#### DEC-015 — Readiness for several homes

- **Problem.** David wants the system ready to serve more than one home, although only one home is implemented. The topic tree already has a home level (`vigil/home1/…`, D-40) and the Pi `client_id` already contains the home, but `home1` was fixed in the ACL and the documents, and one `vigil-device` user and one `vigil-app` user were shared by everyone: with two homes, any app could command any home and any Pi could publish as another home.
- **Current solution (before this decision).** One home, one device (§A.3); users `vigil-device` / `vigil-app` with 12 ACL rules for `home1` + deny-all (D-20); several homes out of scope for the app (DEC-006 6.4, D-23).
- **Alternatives.** A — keep one home and only mention the home level of the topics; B — Pi and broker ready for several homes, app for one home; C — full multi-home (the app chooses among homes, provisioning, a new subscription scheme because of the limit of 10 subscriptions per client).
- **Decision: B** (David, 14 Nov 2026) — Status: approved. Sub-decisions:
  - **15.1 A** — a design decision of the team (David), not a course requirement; §B.1 does not change.
  - **15.2** — key `home_id` in `vigil.conf`, default `home1`, 1–32 characters `[a-z0-9-]` (never `/`, `+` or `#`, which would break topics and ACL matching). `Protocol` builds the topics `vigil/<home_id>/…`; the Pi `client_id` and MQTT username `vigil-device-<home_id>` are derived from it.
  - **15.3 C** — invalid `home_id` at start-up: the local system starts normally; the MQTT connection is not started; the error goes to syslog; events stay `PENDING` (consistent with DEC-004: never refuse to start; the local alarm never depends on the network).
  - **15.4 A** — two MQTT users per home, `vigil-device-<home_id>` and `vigil-app-<home_id>`, each with explicit username rules (12 rules per home), deny `#` for everyone as the last rule. EMQX placeholders (`${username}`) were rejected: a placeholder must replace a whole topic segment, so it can serve only one of the two roles, and Serverless supports no external authorization.
  - **15.5** — the app is configured for one home: `home_id`, username and password in its `broker.json` (git-ignored). No home selection and no screen for it; several homes stay out of scope for the app.
  - **15.6** — the slides keep `home1` in topics and examples (the change is not visible at slide size and changes nothing in the flows); every new document, diagram and code uses `<home_id>`.
  - **15.7** — AT-16 gains one case: credentials of home A can neither publish nor subscribe on home B's topics (L2: Mosquitto with `home1` and `home2`; L3: EMQX). No new acceptance test; the deck row of AT-16 stays as it is (15.6); the case is in the full catalogue of the report.
- **SOLID.** O improves: a new home is configuration (one `home_id`, two users, 12 rules), with no code change. S preserved: only `Protocol` knows the topic layout; `MqttLink` only uses the derived `client_id` and username. No new abstraction.
- **Consequences.** Homes are isolated by credentials and ACL in one shared EMQX deployment (the deployment keeps the name `vigil-home1`). Each extra home costs broker quota; the free-tier share per home (≈ 13 % for one Pi and two phones) is an estimate, so no number of supported homes is claimed. The EMQX Serverless limits on users and ACL rules are not verified (N-13). The test broker, the EMQX rules and the configuration examples must be updated in an implementation batch (DR-9).
- **Affected.** §A.3, §B.7, §C.3, §E.3, §E.7, §E.8, §G.3 (D-20, D-40), §I.1–§I.3, §I.7, §I.8, §J.1 (DR-9, DR-10), §K.2 (AT-16), §L.5, §M.1, §N.1 (N-13), Appendix B. Implementation Guide validity box (§10, §11, §13). `DESIGN_DECK_CHANGES.md` v1.3 (§3.6: slides unchanged). Repository configuration: `cloud/emqx/acl-rules.md`, `cloud/mosquitto/acl` and `passwd`, `cloud/README.md`, `cloud/emqx/emqx.env.example`, `cloud/test-broker.sh`, `buildroot/overlay/etc/vigil/broker.conf.example`, and the files in `secrets/` (DR-9).
- **Sources.** Conversation of 14 Nov 2026 (EMQX subscribe model; readiness for several homes). EMQX Cloud documentation on authorization placeholders and Serverless limitations (consulted 14 Nov 2026).

#### DEC-016 — App MQTT session handling

- **Problem.** While the sequence diagrams were updated (14 Nov), two points of the app connection were not fixed by the documents: (1) whether the app subscribes on every connection or only when the broker has no session for it; (2) what happens with the session expiry the app requests. The analysis also confirmed how the broker works: it pushes every message to all sessions whose subscription matches (no client polls); it stores a client's subscriptions and up to 1000 queued QoS 1 messages in its session until the session expires; retained messages are stored per topic. The Pi keeps no session (Clean Start = 1, Session Expiry = 0): it subscribes again after every CONNACK, and a command sent while it is offline is discarded (the app times out).
- **Current solution (before this decision).** §I.2: app Clean Start 0, session expiry "maximum accepted (7 days requested)"; subscriptions listed, with no rule for reconnection. Implementation Guide §10.2 already said to read the granted value back from CONNACK.
- **16.1 Subscription on reconnection.** A — subscribe after every CONNACK; B — subscribe only when CONNACK has Session Present = 0; C — leave it to the implementation.
  - **Decision: A** (David, 14 Nov 2026) — Status: approved. David first chose B and then reverted to A for simplicity, after Claude reported a problem of B: if Android stops the app process while the broker keeps the session, the app reconnects with Session Present = 1, does not subscribe, receives no retained state and has no status to show until a change arrives. B also depended on `mqtt5_client` exposing Session Present (not verified) and added a second path to test.
  - With A the app always receives the retained `status` and `connection` after connecting; subscribing again over a kept session is harmless; the cost is one SUBSCRIBE per connection.
- **16.2 Session expiry.** A — request 7 days and use the value granted in CONNACK; B — request the maximum (never expire); C — request less (e.g. 2 hours).
  - **Decision: A** (David, 14 Nov 2026) — Status: approved. B was rejected because sessions of phones that no longer use the app would stay on the broker and queue alerts (disconnected sessions count as sessions in EMQX; quota effect not verified); C because a phone offline longer than the expiry would lose the queued alerts.
  - The maximum value EMQX Serverless grants to MQTT 5 clients is not verified (N-14). When a session expires, the alerts queued in it are lost; the app then starts a new session.
- **SOLID.** Neutral (protocol behaviour of the app; no class changes).
- **Consequences.** Diagrams D6–D9 (slides 31–34) show this behaviour. No change to the Pi.
- **Affected.** §G.1, §I.2, §I.5, §N.1 (N-14).
- **Sources.** Conversation of 14 Nov 2026 (sequence diagram review; EMQX Cloud documentation on persistent sessions, offline message queues and session expiry, consulted 14 Nov 2026).

### G.3 Earlier decisions D-01…D-51 (Implementation Guide §17, 4 Nov 2026)

| ID | Topic | Status now |
|---|---|---|
| D-01 | Event-queue architecture, single decision maker | **Valid in principle**; structure replaced by DEC-003a/b |
| D-02 | One EventQueue + one CommandQueue per consumer; timers in the controller (epoll) | **Superseded** by DEC-003a |
| D-03 | Nine threads | **Superseded** by DEC-003a |
| D-04 | Abstract `WorkerThread`; SupervisorThread owns the watchdog | **Modified**: `WorkerThread` kept; supervision moved to `main` (DEC-003a, DEC-004) |
| D-05 | Priorities FIFO 80/70/50, OTHER | **Superseded** by DEC-003a |
| D-06 | Exactly one custom driver: PWM platform driver | **Valid**, modified by DEC-002 (sequence player) |
| D-07 | Own overlay; never `pwm-2chan` | Valid |
| D-08 | Driver failure → RGB + FAULT | **Modified**: FAULT(SPEAKER); RGB keeps showing power/network, no blink (DEC-001/004) |
| D-09 | libgpiod for GPIO, i2c-dev for INA219 | Valid |
| D-10 | Broker only, no backend | Valid |
| D-11 | EMQX Serverless chosen over HiveMQ Cloud, AWS IoT Core, self-hosted Mosquitto | Valid |
| D-12 | Outbox row for every alert before publishing | **Modified** by DEC-006 (every event; columns in `EVENT_LOG`) |
| D-13 | Log events not buffered; recovered with `get_events` | **Superseded** by DEC-006 / DEC-001 |
| D-14 | `get_events` returns the 20 newest | **Postponed** (DEC-001) |
| D-15 | Reconnection order: online → status → pending | Valid (pending events) |
| D-16 | App `client_id` and `request_id` format; foreign replies ignored | Valid (event-batch merge obsolete) |
| D-17 | Never stop reconnecting; auth errors every 5 min | **Modified** by DEC-006 (one backoff 1 → 60 s) |
| D-18 | Detection bounds: broker ≤ 60 s, Wi-Fi ≤ 2 s, Last Will ≈ 45 s | **Modified**: Wi-Fi ≤ 2 s removed (DEC-001) |
| D-19 | TLS per hop; broker sees `cmd` payloads (accepted) | Valid |
| D-20 | Users `vigil-device`/`vigil-app`, least-privilege ACL + deny-all, no login screen | **Modified** by DEC-015: per-home users `vigil-device-<home_id>` / `vigil-app-<home_id>`; least privilege, deny-all and no login screen kept |
| D-21 | `cmd` Message Expiry 30 s; Pi clean session | Valid |
| D-22 | Android foreground service holds the MQTT connection | Valid (DEC-006 6.4) |
| D-23 | Flutter, Android, one screen, app never validates | Valid |
| D-24 | Button → 60 s exit delay; remote arm immediate; both rejected during a fire alarm | Valid |
| D-25 | IR ignored when disarmed and in exit delay; 30 s entry delay | Valid (wording per DEC-010) |
| D-26 | Fire monitored 24/7, fail-loud | Valid |
| D-27 | Keypad PIN during fire → silence 10 min, DISARMED | Valid |
| D-28 | Silence timeout rules | Valid |
| D-29 | Priority fire > intrusion > fire silenced; one PIN silences both | Valid |
| D-30 | Remote disarm check order | Valid |
| D-31 | Remote arm check order, no PIN | Valid |
| D-32 | Shared wrong-PIN counter, 5 → 5 min lockout | Valid |
| D-33 | Low battery ≤ 30 % | **Superseded** by DEC-005a (≤ 6.7 V) |
| D-34 | ≤ 6.4 V → shutdown with power-off | **Superseded** by DEC-005b (safe state) |
| D-35 | One FAULT alert type, once per fault | Valid; component list per DEC-004 |
| D-36 | Database failure at boot: backup first, default PIN last | **Superseded** by DEC-001/004 (no backup) |
| D-37 | EVENT_LOG CHECK list, ISO 8601 text, `id` = `event_id` | Valid |
| D-38 | `PENDING_NOTIFICATION` with UNIQUE(event_id) | **Superseded** by DEC-006 (table removed) |
| D-39 | PIN_CONFIG single row, PBKDF2 string | Valid |
| D-40 | Topic tree `vigil/home1/…` | **Modified** by DEC-015: `vigil/<home_id>/…` (`home1` in the implementation and the slides) |
| D-41 | Fire silenced = alert; intrusion silenced = log event | **Modified** by DEC-014: only the fire version exists (alert); stopping an intrusion alarm logs only `DISARMED` |
| D-42 | Status fields incl. `battery_pct` | **Modified**: no `battery_pct` (DEC-001) |
| D-43 | `online` with timestamp; clean `offline` with reason; Last Will without timestamp | **Modified**: clean `offline` with reason removed (DEC-005b) |
| D-44 | Reply sets per command | **Modified**: `get_events` removed |
| D-45 | App card priority, 20-item list ordered by `event_id`, reconnection summary | **Modified**: arrival order, no summary, no battery % (DEC-006) |
| D-46 | Hardware review of 31 Oct ("PIR"/"Buzzer Module" labels, RGB at 5 V, divider, no RTC, INA219) | **Modified** by DEC-007 and DEC-010 |
| D-47 | FSMs and flowcharts both kept; FSM = `switch` | Valid (control loop per DEC-003a) |
| D-48 | Buildroot 2026.08, Bootlin stable toolchain, RPi kernel 6.12, libgpiod v1 | Valid |
| D-49 | C++17, CMake, Paho MQTT C++, nlohmann-json, SQLite, OpenSSL | Valid |
| D-50 | Local Mosquitto for development and tests | Valid |
| D-51 | NTP + saved date; start without waiting for NTP | Valid |

**Defaults from Guide §18.2 still in use:** G3 (keypad conventions), G5 (response `detail`), G7 (malformed/retained/non-expiring commands ignored without reply), G9 (door open at arming = limitation), G11 (no exit-delay tone), G12 (button outside DISARMED ignored + error beep), G13 (`SAVE_FAILED` `BUSY`). **Superseded:** G1, G2, G6, G8, G10, G14; G4 modified by DEC-005b.

**Revision Plan proposals (4 Nov) not adopted:** extra ≈ 35 test rows and a "Method" column (DEC-008); RGB fault blink latched (D11; removed by DEC-001); FaultComponent `UPS` (D13; became POWER_MONITOR in DEC-004); MQ-2 re-request every 10 s (D12; no retries, DEC-004); 9th keypad digit with error beep (D8; not decided, G3 default applies).

---

## H. Simplification history (8–9 Nov 2026)

Starting point: an 88-slide design with 9 threads, 4 queues, epoll + eventfd + 4 timerfds, a driver with 8 built-in patterns, a 3-table database, ≈ 12 features beyond the Analysis and ≈ 75 planned tests (≈ 110 proposed). Goal: a feasible, SOLID-compliant and defensible system, without losing any Analysis requirement.

| # | Original | Why it was excessive | Change | Type | Requirement still met | Decision |
|---|---|---|---|---|---|---|
| 1 | 9 threads | Threads created per responsibility, not per timing need | 5 threads + `main` | Merged | NFR-01 | DEC-003a |
| 2 | 4 queues (EventQueue + 3 CommandQueues) | One queue per consumer thread that no longer exists | EventQueue + CommQueue | Merged | — | DEC-003a |
| 3 | epoll + eventfd + 4 timerfds | Four timer objects for deadlines a pure class can hold | One condition-variable wait until the nearest deadline | Simplified | FR-04, FR-05 | DEC-003a |
| 4 | OutputThread | Driver `write()` returns at once; a thread added only latency | Outputs applied by the controller (`AlarmOutputs`) | Merged | FR-02, FR-08 | DEC-003a |
| 5 | StorageThread + NetworkMQTTThread | Cross-thread database reads and `IEventSink` callbacks | `CommThread` with `EventStore`, `OutboxPublisher`, `MqttLink`, `Protocol` | Merged (responsibilities kept as classes) | FR-11, FR-12 | DEC-003a/b |
| 6 | SupervisorThread restarting threads | Restart logic hard to test; LSP issues | `main` feeds the watchdog from heartbeats; reset + state restore | Simplified | NFR-02 | DEC-001, DEC-004 |
| 7 | IR edge-wait thread with 200 ms retrigger guard | Separate thread; libgpiod v1/v2 differences | Periodic sampling, 2-read confirmation | Merged / simplified | FR-01 | DEC-003a |
| 8 | Cross-thread `isSmokePresent()` | Shared state between threads | `SMOKE_DETECTED`/`SMOKE_CLEARED` events | Simplified | FR-05 | DEC-003a |
| 9 | Driver with 8 fixed patterns and base/one-shot rule | Policy in the kernel; rebuild to change a tone | Generic sequence player; policy in user space | Simplified | C-5, FR-02 | DEC-002 |
| 10 | `PENDING_NOTIFICATION` table for alerts only | Log events lost offline; extra table | `EVENT_LOG.status/sent_at` for every event | Merged and extended | FR-11, FR-12 | DEC-006 |
| 11 | `get_events` history sync | Extra command, extra Network→Storage dependency | — | Postponed, not presented | FR-11 met by the outbox | DEC-001 |
| 12 | Battery % in 10 % steps | Extra events and payload field | Voltage only in LOW_BATTERY | Postponed, not presented | FR-07 | DEC-001, DEC-005a/b |
| 13 | Power alerts at boot | Boot-time special cases | Initial power state decided silently | Postponed, not presented | FR-06 (runtime alerts kept) | DEC-001, DEC-005b |
| 14 | Database backup and restore | Extra recovery paths | New database + default PIN + FAULT | Postponed, not presented | NFR-02 | DEC-001, DEC-004 |
| 15 | App reconnection summary notification | Extra app logic | — | Postponed, not presented | FR-06 | DEC-001 |
| 16 | Wi-Fi loss ≤ 2 s by netlink (`LinkMonitor`) | Extra component; broker loss is what matters | Keep-alive detection ≤ 60 s | Removed | FR-09 | DEC-001 |
| 17 | RGB fault blink; FAULT retries every 10 s; RAM outbox | Extra states and timers | One FAULT per component per boot | Removed | NFR-02 | DEC-001, DEC-004 |
| 18 | Two reconnection rules (backoff + 5 min on auth errors) | Two code paths | One backoff 1 → 60 s | Simplified | FR-09 | DEC-006 |
| 19 | Power-off at ≤ 6.4 V with "offline + reason" and DISCONNECT 0x00 | Pi may stay halted when mains returns | Safe state; planned stop closes the connection without DISCONNECT (DEC-012) | Replaced | FR-07, NFR-02 | DEC-005b |
| 20 | Low battery at ≤ 30 % | Percentage no longer presented | ≤ 6.7 V | Represented differently | FR-07 | DEC-005a |
| 21 | ≈ 75 tests (≈ 110 proposed) in 3-column tables | Volume without method or traceability | Strategy + 24 acceptance tests; catalogue in the report (DEC-013) | Represented differently | All | DEC-008 |
| 22 | SOLID slide with claims per thread | False claims after the decisions | Evidence map + notes | Represented differently | C-3 | DEC-009 |
| 23 | 3-slide class diagram | Unreadable at slide size | Simplified diagram by package + full diagram on 2 detail slides (DEC-013) | Represented differently | C-7 | DEC-003b |
| 24 | App history sync, battery %, reconnection summary | Scope | One screen, last 20 events in arrival order | Simplified | FR-03, FR-06, FR-10 | DEC-006 |
| 25 | `ALARM_SILENCED (INTRUSION)` log event (review of 14 Nov) | Repeated `DISARMED`; misleading name; remote case ambiguous | Stopping an intrusion alarm logs only `DISARMED` | Removed | FR-12 | DEC-014 |

Effort and risk reduced: fewer threads and synchronisation points to verify, no kernel pattern logic, one database table less, no backup/restore paths, no netlink code, smaller app, and a smaller, traceable test set.

---

## I. Communication and data management

### I.1 Broker, accounts and ACL (D-10, D-11, D-20)

- **Production:** EMQX Serverless deployment `vigil-home1` (name kept; one deployment serves every home, DEC-015), MQTT 5 over TLS on port 8883, spend limit 0. **Tests:** local Mosquitto (`cloud/mosquitto/`) with the same users, ACL and port, using a test CA. Only the broker configuration file changes.
- **Users, per home (DEC-015, 15.4):** `vigil-device-<home_id>` (the Pi; password in `/etc/vigil/secrets/mqtt_pass`, mode 600) and `vigil-app-<home_id>` (all phones of that home). Implemented home: `vigil-device-home1`, `vigil-app-home1`.
- **ACL (12 rules per home + 1 deny-all; 13 rules for one home; `cloud/emqx/acl-rules.md`):** the device user of a home publishes `status`, `alert`, `event`, `response`, `connection` and subscribes `cmd`, only under `vigil/<home_id>/`; the app user of the same home publishes `cmd` and subscribes to the other five; **deny `#` for everyone as the last rule** (EMQX Serverless allows anything not matched). Credentials of one home give no access to another home's topics.
- **EMQX limits shaping the design:** 10 publishes/s per client (outbox paced at ≤ 5/s); 1000 queued messages per offline client; 10 subscriptions per client; free quota ≈ 13 % used.

### I.2 Connection parameters

| | Raspberry Pi | App (each phone) |
|---|---|---|
| `client_id` | `vigil-device-<home_id>` (derived from `vigil.conf`) | `vigil-app-XXXXXXXX` (8 random hex, generated once) |
| Username | `vigil-device-<home_id>` | `vigil-app-<home_id>` (from `broker.json`) |
| Clean Start / Session Expiry | 1 / 0 (no session kept: commands sent while the Pi is offline are discarded) | 0 / 7 days requested; the app uses the value granted in CONNACK (DEC-016) |
| Keep-alive | 30 s | 60 s |
| Last Will | `connection`: `{"state":"offline"}`, QoS 1, retained | none |
| Subscriptions (QoS 1) | `cmd`, after every CONNACK | `status`, `connection`, `alert`, `event`, `response`, after every CONNACK (DEC-016) |
| TLS | CA, certificate and host name verified; never falls back to plain MQTT | same |
| Reconnection | Never stops: backoff 1 → 60 s for every failure, logged once per episode (DEC-006, 6.3 b) | app policy |

After every CONNACK the Pi: subscribes `cmd` → publishes `{"state":"online","timestamp":…}` (retained) → full status (retained) → pending events in `id` order (D-15).

### I.3 Topics

| Topic | Direction | QoS | Retained | Outbox | Content |
|---|---|---|---|---|---|
| `vigil/<home_id>/status` | Pi → app | 1 | yes | no (republished after each reconnection) | Current state |
| `vigil/<home_id>/connection` | Pi → app | 1 | yes | no | `online` or the Last Will |
| `vigil/<home_id>/alert` | Pi → app | 1 | no | yes | Alerts → phone notification |
| `vigil/<home_id>/event` | Pi → app | 1 | no | **yes** (DEC-006) | Log events (list only, no notification) |
| `vigil/<home_id>/response` | Pi → app | 1 | no | no | Command replies |
| `vigil/<home_id>/cmd` | app → Pi | 1 | never | — | Commands, Message Expiry 30 s |

`<home_id>` is the home identifier from `vigil.conf` (DEC-015); in the implementation and in the slides it is `home1` (15.6).

### I.4 Payloads (compact UTF-8 JSON; times ISO 8601 UTC)

```json
{"armed_state":"ARMED","fire_state":"NORMAL","power_source":"GRID","timestamp":"2026-11-03T15:00:00Z"}
{"state":"online","timestamp":"2026-11-03T15:00:05Z"}
{"state":"offline"}
{"event_id":57,"event_type":"FIRE_ALARM","timestamp":"2026-11-03T15:01:00Z","details":"DETECTED"}
{"request_id":"vigil-app-1a2b3c4d-18","type":"disarm","pin":"1234"}
{"request_id":"vigil-app-1a2b3c4d-18","result":"LOCKED_OUT","detail":"4"}
```

| Event type | Topic | `details` |
|---|---|---|
| `INTRUSION_ALARM` | alert | `IR1` / `IR2` |
| `FIRE_ALARM` | alert | `DETECTED` / `REACTIVATED` |
| `ALARM_SILENCED` | alert | `FIRE` |
| `HAZARD_CLEARED` | alert | null |
| `POWER_LOST` / `POWER_RESTORED` | alert | null (DEC-005b) |
| `LOW_BATTERY` | alert | voltage, e.g. `"6.68 V"` (DEC-005b) |
| `SHUTDOWN` | alert | null (DEC-012, 12.3) |
| `FAULT` | alert | `IR1` `IR2` `MQ2` `KEYPAD` `SPEAKER` `POWER_MONITOR` `DATABASE` (DEC-004) |
| `PIN_LOCKOUT` | alert | `LOCAL` / `REMOTE` |
| `ARMED`, `DISARMED`, `WRONG_PIN` | event | `LOCAL` / `REMOTE` |
| `PIN_CHANGED` | event | null |

Commands: `arm`, `disarm` (`pin`), `change_pin` (`pin`, `new_pin`); `get_events` no longer exists (DEC-001). Replies and their order: §E.4. `request_id` = `<client_id>-<counter>`. Not logged: key digits, button presses, the start of an entry delay, connectivity changes (syslog only). Never logged or published: any PIN, password or `cmd` payload.

### I.5 Delivery, ordering and offline operation

- **Outbox (DEC-006, 6.1 b'):** every event is inserted as `PENDING` before publishing; `OutboxPublisher` publishes pending events in `id` order at ≤ 5/s; PUBACK `0x00` or `0x10` → `SENT`; `0x97` (quota) → back off and retry; link lost before PUBACK → stays `PENDING`. Live and resent events always leave in `id` order.
- **Remote command order:** response → log event → status. **New alert order:** stored → published → status.
- **Offline Pi:** local alarm, keypad and outputs unaffected; RGB shows the loss; events accumulate as `PENDING`. **Offline phone:** the broker keeps up to 1000 QoS 1 messages in the phone's session until the granted session expiry; after it, the queued messages are lost and the app starts a new session (DEC-016). Delivery is push: the broker sends each message to every session whose subscription matches, so every phone of the home receives it.
- **Duplicates:** QoS 1 can deliver twice; the app drops exact duplicates and lists events in arrival order (6.2 a), which is safe also after the database is recreated and `id` restarts.
- **Detection bounds:** broker loss on the Pi ≤ 60 s; the broker publishes the Last Will ≈ 45 s after the Pi goes silent; a planned stop publishes it at once because the Pi closes the connection without DISCONNECT (DEC-012, 12.1).

### I.6 Incoming command validation

Ignore retained `cmd` messages and messages without a Message Expiry property; require a JSON object with a string `request_id` (1–64 characters) and a known `type`; `disarm` needs `pin`, `change_pin` needs `pin` and `new_pin`; anything else → syslog, no reply (default G7).

### I.7 Android app (DEC-006, 6.4 a)

- Flutter, Android, one screen, light theme, no login; Material 3 tokens from the GUI document (green `#2E7D32`, grey `#5F6368`, amber `#B26A00`, red `#C62828`, primary `#1A4D8F`).
- **Status card** priority: fire > intrusion > fire silenced > armed/disarmed; power row "Mains"/"Battery" **without percentage**; online/offline from `connection` (Last Will; "Offline" without time).
- **Buttons** Arm, Disarm, Change PIN with dialogs, every Pi reply handled, 32 s timeout ("No response from the device. The command was not executed.").
- **List** of the last 20 events (alerts and log events), arrival order, exact duplicates dropped. Intrusion notification text: "Intrusion – door/window moved (IR1)" (DEC-010).
- **Notifications** only for the `alert` topic, through an Android foreground service (`specialUse` type) that keeps the MQTT connection open; permanent low-priority "VigilSystem – monitoring" notification.
- **One home (DEC-015, 15.5):** the app is configured for one home through `broker.json` (git-ignored: host, port, `home_id`, username `vigil-app-<home_id>`, password, optional CA); there is no home selection and no screen for it.
- **Out of scope:** history sync, reconnection summary, iOS, several homes or choosing a home, configuring delays from the app.
- **Risk:** the MQTT connection inside the foreground-service isolate is the hardest part; prototype it early.

### I.8 Security notes (D-19, D-20)

TLS protects each hop separately; the broker can read `cmd` payloads, including the PIN (accepted; `cmd` is never retained or stored). A leaked app credential can only send commands to its own home (disarm still needs the PIN, protected by the lockout); it cannot fake alerts, status or `online`, and it gives no access to another home (per-home users and ACL, DEC-015). A leaked device credential requires physical access to the SD card. The PIN hash uses PBKDF2-SHA256 with a random salt; the default PIN hash file is root-only. Change the Buildroot root password before the demonstration.

---

## J. Implementation status (evidence as of 9 Nov 2026)

| Subsystem | Status | Evidence |
|---|---|---|
| Repository skeleton | **Implemented** | `~/VigilSystem`, 2 commits on 4 Nov ("Add implementation guide and CLAUDE.md"; "Add project skeleton…"). **No git remote configured** |
| Local Mosquitto test broker + `cloud/test-broker.sh` (13 checks) | **Implemented, tested on the laptop** | Scripts and configuration in `cloud/`; used successfully by the 5 Nov prototype demo **[VERIFIED: files; demo results recorded in `staging/threads-class/README.md`]** |
| EMQX Serverless deployment and ACL | **In progress / unknown** | Guide §3.1 (4 Nov): deployment started, steps A3–A5 probably not done. Not checked now (`secrets/` not opened) |
| Buildroot configuration and overlay | **Implemented** | `buildroot/configs/`, `buildroot/overlay/` (S40wifi, S99vigil, chrony). A build output folder exists in `~/buildroot/buildroot-2026.08/output/` **[UNVERIFIED: image content and boot on the Pi]**. Pending: BR2_EXTERNAL packages, `config.txt`, `rt-tests`/`stress-ng`, choose Dropbear or OpenSSH |
| Pi application (`pi/`) | **Not started in the repository** (only `.gitkeep`) | — |
| Laptop prototype `staging/threads-class/` (5 Nov) | **Prototype of the superseded architecture; not committed; not integrated** | 69 unit tests passed and 8 demo checks on the laptop with simulated hardware and Mosquitto (5 Nov). Contains core queues, `WorkerThread`, `NetworkMQTTThread` + `MqttClient` (TLS, Last Will, backoff), `Protocol`, stub controller, `LocalInputThread`, `MQ2SensorThread`, `IRSensorsThread`, `PowerMonitorThread` with a fake INA219, simulated board. `GpioLines` (libgpiod v1) and `Ina219` written but **never compiled** |
| `AlarmLogic`, storage + outbox, `OutboxPublisher`, `AlarmOutputs`, `HealthMonitor`/watchdog | **Not started** | — |
| `vigil_pwm` driver + overlay | **Not started** (`driver/` empty) | — |
| Android app | **Not started** (`app/` empty) | A Figma Make prototype exists according to the GUI document **[UNVERIFIED: not in the repository]** |
| Hardware | **Not assembled, not validated** | Owned: Pi, red LED, RGB LED, speaker, 2 IR, breadboard, microSD. Rest ordered the week of 9 Nov |
| Design presentation | **88-slide deck (8 Nov) not yet updated** | Phase 4 decided (DEC-013); change plan v1.1 ready (`docs/presentation/DESIGN_DECK_CHANGES.md`, 71 slides); no slide edited yet |
| Documentation | **Consolidated** (9 Nov; updated 14 Nov) | Master Context v1.3, Guidelines, `CLAUDE.md`, `README.md`, hardware specification v2, Guide validity box and `docs/archive/` in the repository; mirror under `VigilSystem/` in the Claude Project; old Project documents removed. The Project instructions and the global working rules (`PASTE_IN_TEXTS.md`) were pasted by David (confirmed 14 Nov) |

### J.1 Discrepancies between code/documents and the approved design

| ID | Discrepancy | Action needed (future work, not done now) |
|---|---|---|
| DR-1 | The prototype follows the superseded architecture (separate IR/MQ-2/Network threads, epoll queues, 3 command queues, `get_events`, 5 min auth retry, `isSmokePresent()`, planned `LinkMonitor` and OutputThread) | Refactor into the DEC-003a/b structure; reuse `WorkerThread`, `PiMutex`, `MqttClient` (→ `MqttLink`), `Protocol`, backoff, keypad and debounce logic, INA219 and GPIO wrappers, simulated board |
| DR-2 | Prototype: keypad/button request failure → syslog only | DEC-004 requires FAULT(KEYPAD) |
| DR-3 | Prototype planned stop: retained `offline` + DISCONNECT 0x00, because Paho did not pass reason 0x04 | Replace by "close without DISCONNECT" (DEC-012, 12.1) |
| DR-4 | Implementation Guide (two diverging copies) describes the superseded design | Validity box added on 9 Nov; Project copy archived; v2 in approved batches (DEC-011) |
| DR-5 | Project instructions, repo `CLAUDE.md` and `README.md` describe older designs | **Resolved:** `CLAUDE.md` and `README.md` replaced on 9 Nov; Project instructions pasted by David (confirmed 14 Nov) (DEC-011) |
| DR-6 | The Claude Project holds the 61-slide deck; the repo holds a 61-slide PDF (3 Nov); the current deck has 88 slides | David uploads the current deck to the Project and exports a new PDF to `docs/design/` |
| DR-7 | `staging/threads-class/README.md` says `BR2_PACKAGE_NLOHMANN_JSON` is missing, but `vigil_fragment.config` already has `BR2_PACKAGE_JSON_FOR_MODERN_CPP=y` (the Buildroot package name) | Probably no action; confirm at the first cross-build |
| DR-8 | Implementation Guide (§6.6, §10.4/§10.7 ordering, app texts table) still has `ALARM_SILENCED (INTRUSION)` | **Resolved 2026-11-14:** the references are marked as removed in the Guide (validity box note added); `DESIGN_DECK_CHANGES.md` v1.2 lists the slide actions |
| DR-9 | Repository configuration still uses the shared users `vigil-device` / `vigil-app` and only `home1` (DEC-015): `cloud/emqx/acl-rules.md`, `cloud/mosquitto/acl` and `passwd`, `cloud/README.md`, `cloud/emqx/emqx.env.example`, `cloud/test-broker.sh` (no cross-home check), `buildroot/overlay/etc/vigil/broker.conf.example` (`client_id` and `username` written in it), and the files in `secrets/` | Separate implementation batch: rename the users to `vigil-device-home1` / `vigil-app-home1`, add `home2` test users for the AT-16 isolation case, regenerate the Mosquitto `passwd` (needs the passwords in `secrets/`, done by David), update the EMQX console. Left unchanged on 14 Nov so the working test broker is not broken |
| DR-10 | Implementation Guide §10.1–§10.3, §11 and §13 describe one home with shared users | Validity box note added 2026-11-14 (DEC-015) |

---

## K. Testing and validation (DEC-008)

### K.1 Levels

| Level | Where | What |
|---|---|---|
| L1 — unit | Laptop, GoogleTest + ctest | `AlarmLogic` and FSMs with injected time; `DebouncedInput`, `KeypadScanner`, `PinEntry`, `PowerSupplyMonitor` on simulated hardware; `EventStore`/`PinStore` on in-memory SQLite; `Protocol`; `BlockingQueue` |
| L2 — integration | Laptop | `VIGIL_SIM` build + Mosquitto + `mosquitto_sub` + app |
| L3 — target | Raspberry Pi | Real hardware, `vigil_pwm`, power, real time under load, EMQX + app on a phone |

The same contract tests run on simulated (L1/L2) and real (L3) implementations of `IGpioInput`, `IPwm` and `IPowerSensor` (evidence for LSP). L1 and L2 can start now; L3 waits for the hardware.

### K.2 Acceptance tests (shown in the presentation; full English table in the deck change list)

| ID | Test | Level | Analysis requirements |
|---|---|---|---|
| AT-01 | Local arming with exit delay | L3 | Flexible Control; Entry/Exit Delay; Visual Status |
| AT-02 | Disarm during the entry delay | L3 | Entry/Exit Delay; Flexible Control |
| AT-03 | Intrusion alarm (phone alert ≤ 5 s) | L3 | Dual Detection; Distinct Alarms; Instant Alerts |
| AT-04 | Wrong PIN → lockout | L1 + L3 | Security; Flexible Control |
| AT-05 | Fire alarm in any state, distinct tone | L3 | Dual Detection; Distinct Alarms |
| AT-06 | Fire silence period and reactivation | L1 + L3 | Fire Alarm Silence Period |
| AT-07 | Remote arm/disarm; disarm rejected during fire | L2 + L3 | Flexible Control |
| AT-08 | PIN change from the app | L2 | PIN Management; Security |
| AT-09 | IR glitch rejected | L1 | Dual Detection |
| AT-10 | MQ-2 warm-up without false alarm | L3 | Dual Detection |
| AT-11 | Mains failure and return | L3 | Power Resilience; Instant Alerts; Visual Status; Availability |
| AT-12 | Low battery, safe state and reboot | L3 | Power Resilience |
| AT-13 | Autonomy ≥ 3 h | L3 | Power Resilience |
| AT-14 | Offline events delivered in order | L2 + L3 | Pending Notification Handling; Connectivity Monitoring; Event Logging |
| AT-15 | Offline detection (Last Will and planned stop) | L2 + L3 | Instant Alerts |
| AT-16 | Secure channel (TLS, ACL, expired command; credentials of home A cannot publish or subscribe on home B's topics — DEC-015, case kept in the report catalogue, deck row unchanged) | L2 + L3 | Security |
| AT-17 | State restored after reboot | L2 + L3 | Availability |
| AT-18 | Watchdog recovery | L3 | Availability |
| AT-19 | Component fault | L2 | Availability |
| AT-20 | Corrupt database at boot | L2 + L3 | Event Logging; Availability |
| AT-21 | Custom PWM driver | L3 | Distinct Alarms; Visual Status |
| AT-22 | Alarm latency under load ≤ 1 s | L3 | Real-Time Responsiveness |
| AT-23 | Key → beep ≤ 100 ms; periodic loops without missed periods | L3 | Real-Time Responsiveness |
| AT-24 | 24 h stability | L3 | Availability |

Every Analysis requirement is covered by at least one acceptance test. The full catalogue (unit and integration tests) goes to the report (DEC-013, P4-2) and to the Implementation Guide v2.

### K.3 Real-time measurement method (note only; not in the presentation, DEC-008 8.3)

- Software: `CLOCK_MONOTONIC` timestamps at the reading that creates the event and at the driver `write()`; `cyclictest` for system latency; `stress-ng` for load.
- External: oscilloscope or logic analyser with one channel on the input pin (IR or MQ-2) and one on GPIO18; at least 50 samples; minimum, mean and maximum.
- **[OPEN]** Laboratory equipment availability is not confirmed; without it, software measurement only, with the limitation stated in the report.

### K.4 Results so far and limitations

- **No acceptance test has been run.** The only results are the 69 laptop unit tests and 8 demo checks of the superseded-architecture prototype (5 Nov); they are not acceptance evidence.
- AT-19 is feasible only at L2 (a libgpiod read failure cannot easily be caused on the Pi).
- AT-18 needs a way to block one thread in a test build (for example a signal enabled only in that build).
- AT-12 doubles as the verification of the safe-state condition (HAT cuts and restores power).

---

## L. Presentation context

### L.1 Role

The Design presentation is the deliverable of the Design phase for SEA and STR, in English, assessed by the professor. It follows the Analysis presentation (deck A, 33 slides). It must reflect the approved architecture of this document, not the superseded one.

### L.2 What the professor expects

Explicitly attributed to the professor (G9 review, as recorded in the Revision Plan of 4 Nov):

| # | Expectation | How VigilSystem answers it |
|---|---|---|
| P1 | Physical deployment: breadboard, protoboard or PCB, and where each sensor is mounted | DEC-007 (7.3 b), slide 13 caption |
| P2 | Design patterns + SOLID map (pattern → class → problem) | DEC-009: slides 68 and 70 + notes |
| P3 | Class diagram following UML rules and a readable structure | DEC-003b: simplified diagram by package; full diagram on 2 detail slides by package (DEC-013) |
| P4 | Task slide with the role of each task | Thread table (DEC-003a) |
| P5 | One flowchart per task | Flowcharts for the 5 threads and `main` (deck change list) |
| P6 | Is SQLite a library or a server? | Library linked into the process (§E.5) |

Reported by David: SOLID is mandatory; no particular kind of driver is valued more than another; a different approach from previous years is expected.

### L.3 Lessons from other groups (context, not requirements)

- **G9 TrolTrack** (this year, well received): test tables with IDs (`TC-…`), an "Objective" column and an explicit test strategy line (host testing → Raspberry Pi integration → results). Adopted in DEC-008 in VigilSystem's own form.
- **G4 AllWaysSafe** (best last year): test cases and error handling organised per subsystem.
- The detailed slide-by-slide comparison made early in the 8 Nov conversation is only partly recoverable (§N.4).
- **Originality:** other groups' solutions are not requirements. The text of current slide 6 was reused from G4 and must be rewritten.

### L.4 Deck versions

| Version | Where | Use |
|---|---|---|
| 88 slides, 8 Nov 2026 | Team's working copy; uploaded to the conversation of 8 Nov | **Current baseline** for every slide number in this document |
| 61 slides, 2 Nov | Claude Project (`G8_VigilSystem_D_Presentation.pptx`) | Outdated — to be replaced by David |
| 61 slides, 3 Nov | Repo `docs/design/G8_VigilSystem_D_Presentation.pdf` | Outdated |
| 71 slides (target) | `docs/presentation/DESIGN_DECK_CHANGES.md` v1.3 | Result of DEC-013; not yet produced |

### L.5 What goes where (DEC-013)

- **Main presentation:** decisions and behaviour; concurrency view (5 threads + 2 queues); simplified class diagram by package; one flowchart per thread; FSMs; driver architecture and flowcharts; test strategy + 24 acceptance tests; SOLID notes, SOLID map and patterns; one end-to-end walkthrough slide; the full class diagram (overview + 2 detail slides by package). 71 slides, no annex.
- **Report, not the deck:** the full test catalogue (DEC-013, P4-2). The interface and message tables stay in the deck (condensed per P4-3; all 17 data and communication slides kept, P4-4).
- **Never presented:** battery %, `battery_pct`; power alerts at boot; database backup/restore; `get_events`/history sync; reconnection summary ("N alerts received"); Wi-Fi loss ≤ 2 s; thread restarts by a supervisor; RGB fault blink; power-off on critical battery; measurement equipment or "tests in the laboratory" (DEC-008, 8.3); where SOLID is not applied and the claim "new event = EventType value" (DEC-009); summaries or justification lines about differences from the Analysis (DEC-013, P4-6).
- **Readiness for several homes (DEC-015, 15.6):** not changed in the slides — topics and examples keep `home1`, and the AT-16 row keeps its current text. New documents, diagrams and code use `<home_id>`.
- **Change list:** `docs/presentation/DESIGN_DECK_CHANGES.md` v1.3 (English): new order (71 slides), the action for each of the 88 old slides, ready-to-paste text for new and merged slides, diagram list D1–D30 and the final checklist. It replaces the earlier Portuguese change list (archived).
- **How the deck is edited (DEC-013, P4-8/P4-9):** Claude edits text and tables only in batches David authorizes, and only on the slides named in each batch; Claude drafts the diagrams in draw.io and the team reviews them; the team does layout and images. The Gantt slide is not updated.

---

## M. Pending work

### M.1 Approved work

| Area | Task | Depends on |
|---|---|---|
| Documentation (DEC-011) | Consolidation done on 9 Nov. Project instructions and global working rules (`PASTE_IN_TEXTS.md`) pasted — **done** (confirmed by David, 14 Nov) | David |
| Documentation | Implementation Guide v2, in approved batches | This document |
| Presentation | Execute `DESIGN_DECK_CHANGES.md` (71 slides) in batches; Claude drafts diagrams D1–D30 in draw.io | David's authorization of each batch (DEC-013, P4-9) |
| Presentation | Upload the 88-slide deck to the Project; export a new PDF to `docs/design/` | David |
| Hardware | Order the parts (week of 9 Nov); measure the IR output now; on arrival test the HAT first (`0x42`, current sign, cut/restore at discharge); breadboard bring-up; protoboard; enclosure | Purchases |
| Software | Refactor the prototype into the approved structure (DR-1, DR-2); `AlarmLogic` with L1 tests; `EventStore`/`PinStore`/outbox; `MqttLink`/`OutboxPublisher`/`Protocol`; `OutputPolicy`/`AlarmOutputs`; `HealthMonitor`/`Watchdog`/`main` | Guide v2 sections |
| Driver | Overlay + `probe()` + constant tone first; then the sequence player and its tests | Pi + speaker (owned) |
| Platform | BR2_EXTERNAL packages, `config.txt`, `rt-tests`/`stress-ng`, one SSH server, root password | — |
| Cloud | Update the repository configuration for DEC-015 (DR-9); finish EMQX (users `vigil-device-home1` / `vigil-app-home1`, 12 rules + deny-all; check the user and rule limits, N-13) and run `test-broker.sh` against it, including the cross-home case of AT-16 | David (passwords in `secrets/`) |
| App | Foreground-service + MQTT prototype early; then the one-screen app | Protocol in Guide v2 |
| Tests | L1/L2 now; L3 after hardware; 24 acceptance tests | — |

### M.2 Decisions and approvals still needed

1. Authorization of each deck-editing batch and each diagram draft (DEC-013, P4-8/P4-9).
2. Implementation roadmap update (Guide §19 is obsolete). The Gantt slide stays as it is (DEC-013).

### M.3 Suggestions (not approved)

- Add a private git remote (backup, and access for Ana).
- Ask Ana to review DEC-001…DEC-013 and this document.
- Re-plan the implementation milestones against the Gantt (implementation until ≈ 27 Dec 2026, verification ≈ 1–15 Jan 2027).

---

## N. Unresolved questions and uncertainty

### N.1 Open technical questions

| # | Question | Evidence | Next step |
|---|---|---|---|
| N-1 | ~~DISCONNECT 0x04 through Paho~~ | **Decided by DEC-012 (12.1):** no DISCONNECT on a planned stop | Verify in AT-15 that both brokers publish the Will at once |
| N-2 | ~~Leave SAFE_STATE when mains returns?~~ | **Decided by DEC-012 (12.2):** controlled reboot | — |
| N-3 | Does the HAT cut its output at full discharge and restore it on charge? | Condition of 5b.3 | First HAT test |
| N-4 | Driver UAPI struct layout | N = 8 and the stop rule decided by DEC-012 (12.4) | Write the struct in Guide v2 §8 |
| N-7 | INA219 current sign and mains thresholds | Proposal | Measure |
| N-8 | MQ-2 warm-up; PBKDF2 iterations | Defaults | Measure on the Pi |
| N-9 | IR model, output type, LOW level | 7.2 works for both types | Measure (possible now) |
| N-10 | Laboratory scope / logic analyser | Unknown | Ask the lab |
| N-11 | Enclosure dimensions | — | Check before buying |
| N-12 | EMQX deployment state; does the Buildroot image boot? | Not checked | Check |
| N-13 | EMQX Serverless limits on the number of users and authorization rules (how many homes one deployment holds) | Not verified (DEC-015); free-tier share per home ≈ 13 % is an estimate | Check in the EMQX console when finishing the deployment |
| N-14 | Maximum session expiry EMQX Serverless grants to MQTT 5 clients; does `mqtt5_client` expose the granted value from CONNACK? | Not verified (DEC-016) | Check in the first app connection test against EMQX |

### N.2 Unverified assumptions

- C-1…C-7 match the official syllabus (not available).
- RGB LED part number CEL03119 (Hardware Context) vs Botnroll 417 (old Project instructions).
- The 88-slide deck is edited in Google Slides (its export contains "Google Shape" identifiers).
- A Figma Make prototype of the app exists (GUI document).
- Ana agrees with DEC-001…DEC-011.

### N.3 Obsolete or contradictory sources (do not implement from them)

| Source | What is wrong now |
|---|---|
| Project instructions (before consolidation) | PIR, four custom drivers, red LED on GPIO26, RGB on 4/13/19, 3-table database, `VigilSystem`/`DelayTimer`/`AlarmActuator` classes, "transition to Design" |
| Repo `CLAUDE.md` and `README.md` (4 Nov) | The Guide as source of truth, "do not redesign", old thread names, link to a missing `NEXT_3_HOURS.md` |
| Implementation Guide (both copies, 4 Nov) | 9 threads, epoll/timerfd, command queues, Supervisor, `PENDING_NOTIFICATION`, `get_events`, netlink, battery %, power-off, start DISARMED, 5 min auth retry, 8 driver patterns, RGB active-low. The two copies also differ from each other on the RGB circuit |
| Design Decisions Record (2 Nov) | Superseded items in §G.3 |
| Revision Plan (4 Nov) | Numbering of a 71-slide deck; proposals partly rejected |
| Class / Package diagrams, Class Diagrams for Slides | Superseded by DEC-003b |
| Data Storage / ERD / Cloud updates (2 Nov) | 3-table ERD; asks for an "ONT" box and "recommended UPS" on the cloud slide, while Guide §2.4 lists them as declined. **[OPEN]** Which version is in the current deck was not checked |
| MQTT Broker Selection (27 Oct), GUI Design (31 Oct) | Outbox for alerts only, `get_events`, 5 min retry, battery %, "last seen", reconnection summary |
| Hardware Context (31 Oct) | RGB sunk directly by GPIOs, IR interface, "PIR" label, list of owned parts |
| Work Log (3 Nov) | History; its 28 issues are partly resolved by the decisions |

All Claude Project documents in this table are archived in `docs/archive/` (index: `docs/archive/README.md`) and were removed from the Claude Project on 9 Nov 2026.

### N.4 Information that could not be recovered

The full list CON-01…CON-19; the original ten global working rules; the detailed Phase 2 comparison with G4 and G9; earlier conversations (2–5 Nov); `NEXT_3_HOURS.md`; "Threads Implementation Class Plan (3h)"; "Design Deck: Changes To Make (31 Oct)".

### N.5 Main risks

| Risk | Mitigation |
|---|---|
| Schedule: the implementation phase ends ≈ 27 Dec and only a prototype of the old design exists | L1/L2 work now; reuse the prototype; driver early |
| Purchases on the critical path | Order this week; test the HAT first |
| HAT does not restore power after cut-off | First HAT test; fallback decision ready |
| Foreground service with MQTT in an isolate | Prototype early |
| Only one copy of the repository, no remote | Add a remote (suggestion M.3) |
| Decisions approved by one of two team members | Ana's review |
| Documentation drift between the repo and the Project | Maintenance procedure (§O, Guidelines) |

---

## O. Collaboration rules (summary)

Full rules in `VIGILSYSTEM_GUIDELINES.md` (project-specific) and in David's personal preferences in Claude (global rules).

1. **David decides.** Claude is not authorized to modify, create, delete, replace or restructure any project file without David's explicit approval: code, architecture, hardware, requirements, presentations, documents, diagrams, configuration, tests, commits and installations.
2. **Before any change:** state the problem, why it matters, the alternatives (including keeping the current solution), trade-offs, consequences and a recommendation; then ask for authorization. Approval of one change is not approval of others. **Approving a decision is not authorization to execute it.**
3. **Architectural proposals** include a SOLID Impact Analysis (§F.1) and use the decision format (problem, current solution, alternatives A/B/C, recommendation, risks, decision, status). Features are classified Keep / Simplify / Postpone / Remove.
4. **Honesty:** distinguish facts, assumptions and proposals; never invent explanations or decisions; challenge decisions with arguments; report technical problems instead of silently changing the design; never conceal uncertainty.
5. **Settled decisions** (§G) are reopened only for a new problem and with authorization.
6. **Language:** conversation in European Portuguese with English technical terms; project documentation and slides in technical English.
7. **Diagrams:** draw.io (editable `.drawio` plus SVG/PNG exports); never modify existing diagrams without authorization.
8. **SOLID** is mandatory and applied pragmatically.
9. **Documentation maintenance:** when a decision is complete, Claude lists the documentation changes it implies and asks for approval; nothing is edited autonomously (the earlier standing authorization was revoked by DEC-011). Every update bumps the version and date of this document and is mirrored between the repository and the Claude Project.
10. **Never commit secrets**; no git commit unless David asks.

---

## Appendix A — Documentation map (target structure after consolidation)

```
VigilSystem/                                  (repository = canonical copy; Claude Project mirrors it)
├── CLAUDE.md                                 entry point and reading order
├── VIGILSYSTEM_MASTER_CONTEXT.md             this document (SSOT)
├── VIGILSYSTEM_GUIDELINES.md                 working rules, SOLID, draw.io, maintenance procedure
└── docs/
    ├── IMPLEMENTATION_GUIDE.md               detailed specification (banner now; v2 later)
    ├── hardware/HARDWARE_CONTEXT.md          hardware specification v2
    ├── presentation/DESIGN_DECK_CHANGES.md   Phase 4 deck change plan (English)
    ├── design/                               deck PDF, broker-selection .docx
    └── archive/                              superseded documents with a banner
```

## Appendix B — Glossary

| Term | Meaning |
|---|---|
| Alert | Event published on `alert`; produces a phone notification |
| Log event | Event published on `event`; list only |
| Outbox | `EVENT_LOG.status` = `PENDING` until PUBACK, then `SENT` |
| `event_id` | `EVENT_LOG.id` |
| `home_id` | Identifier of one home in `vigil.conf` (default `home1`); used in the topics `vigil/<home_id>/…` and the MQTT usernames (DEC-015) |
| Safe state | Below 6.4 V: SHUTDOWN alert, database closed, system keeps running until the HAT cuts power |
| Deadline | A point in time held by `AlarmLogic` (exit, entry, silence, lockout, warning restore) |
| Composition root | `main()`, where real or simulated objects are created and injected |
| Degraded mode | Running with a failed component after one FAULT alert |
| Last Will | Message the broker publishes when the Pi disappears without a normal disconnect |
| L1 / L2 / L3 | Test levels: laptop unit, laptop integration, Raspberry Pi |

## Appendix C — Change log

| Version | Date | Change | Approved by |
|---|---|---|---|
| 0.9 (draft) | 2026-11-09 | First consolidation (DEC-001…DEC-011, audit of documents and repository) | — |
| 1.0 | 2026-11-09 | Draft approved; DEC-012 added (delegated decisions); Phase 4 change plan referenced | David (DEC-012 delegated to Claude) |
| 1.1 | 2026-11-09 | Distributed to the repository and the Project; old Project documents archived and removed; DEC-013 (Phase 4 presentation decisions); §0.4, §B.8, §G.1, DEC-003b/004/006/008/010 notes, §H, §J, §J.1 (DR-4, DR-5), §K.2, §L.2, §L.4, §L.5, §M, §N.3 updated | David |
| 1.2 | 2026-11-14 | DEC-014 (intrusion `ALARM_SILENCED` log event removed): header, §0.3, §E.4, §G.1, §G.2, §G.3 (D-41), §H (row 25), §I.4, §J.1 (DR-8, resolved the same day: Implementation Guide and `DESIGN_DECK_CHANGES.md` updated for DEC-014) | David |
| 1.3 | 2026-11-14 | DEC-015 (Pi and broker ready for several homes, app for one home): header, §0.3, §A.3, §B.7, §C.3, §E.3, §E.7, §E.8, §G.1, §G.2, §G.3 (D-20, D-40), §I.1–§I.3, §I.7, §I.8, §J.1 (DR-9, DR-10), §K.2 (AT-16), §L.5, §M.1, §N.1 (N-13), Appendix B. Implementation Guide validity box and `DESIGN_DECK_CHANGES.md` v1.3 updated in the same batch. Stale version references corrected the same day in §J (Documentation row: v1.1 → v1.3) and §L.4 (`DESIGN_DECK_CHANGES.md` v1.1 → v1.3); pasting of the Project instructions and global working rules marked done in §J, §J.1 (DR-5) and §M.1 (confirmed by David) | David |
| 1.4 | 2026-11-14 | DEC-016 (app MQTT session handling: subscribe after every CONNACK; 7 days requested, granted value used): header, §0.3, §G.1, §G.2, §I.2, §I.5, §N.1 (N-14) | David |
