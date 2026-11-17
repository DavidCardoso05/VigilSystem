# VigilSystem — Design Deck Change Plan (Phase 4)

| Field | Value |
|---|---|
| Document | `docs/presentation/DESIGN_DECK_CHANGES.md` |
| Version | **1.3** — 2026-11-14: §3.6 added (DEC-015: readiness for several homes, slides unchanged). v1.2 — 2026-11-14: DEC-014 items added (intrusion `ALARM_SILENCED` removed). v1.1: Phase 4 decisions taken by David on 2026-11-09 (DEC-013); P4-3 confirmed |
| Baseline | Design deck, **88 slides, 8 Nov 2026** ("old" numbers below refer to it) |
| Result | **71 slides**, no annex |
| Sources | `VIGILSYSTEM_MASTER_CONTEXT.md` v1.3 (DEC-001…DEC-015); `VIGILSYSTEM_GUIDELINES.md`; Phase 4 decisions (§2) |
| Replaces | Draft 0.9 of this plan and the Portuguese change list `claude/VigilSystem – Presentation Changes (DEC-001 to DEC-006).md` |

**Nothing in the deck has been changed yet.** Claude edits slides only when David authorizes it, and only the slides he authorizes (§3.4).

---

## 1. How to use this document

1. §4 gives the new order (71 slides) and where each old slide goes.
2. §5 lists, for each of the 88 old slides, exactly what changes.
3. §6 has the ready-to-paste English text of every new, merged or rewritten slide.
4. §7 lists the diagrams to draw in draw.io.
5. §3 applies to the whole deck; §8 is the final check before presenting.

P4-3 was an interpretation of David's answer and **David confirmed it**: the hardware specification and interface slides go **from 14 to 8** (3 specification + 5 interface slides), because 4 was too few (§4, P4-3).

---

## 2. Phase 4 decisions

| ID | Decision | Choice |
|---|---|---|
| P4-1 | Overall structure | **A** — keep the current order; apply only the decided changes |
| P4-2 | Annex | **A** — no annex; nothing is moved out of the deck |
| P4-3 | Hardware specification (old 6–11) and interface tables (old 16–23) | **Condense, but not to 4 slides** → 3 specification slides + 5 interface slides (two related interfaces per slide where they share a thread or a driver). *Interpretation confirmed by David* |
| P4-4 | Data and communication (old 26–42) | **Keep all 17 slides**, updated |
| P4-5 | Flowcharts | **A** — one flowchart per slide |
| P4-6 | Divergences from the Analysis | **Not shown** — no summary slide and no justification lines. The reasons stay in the Master Context (§B.8, §G) for oral questions |
| P4-7 | End-to-end walkthrough | **B** — one slide (§6.5 explains it) |
| P4-8 | Who draws the diagrams | **B** — Claude drafts the `.drawio` files; the team reviews and adjusts |
| P4-9 | How slides are edited | **C** — Claude edits text and tables, the team does layout and images; Claude acts **only when authorized and only on the authorized slides** |
| P4-10 | Connectivity LED FSM (old 52) | **B** — removed; the RGB colour table and the network state go to the RGB interface slide |
| P4-11 | GUI (old 72–75) | **B** — states table + one mock-up slide |
| — | Gantt (old 87) | **Not updated** — slide kept as it is |

Consequences of P4-2 (no annex) on earlier decisions:

- The full class diagram stays in the deck (3 slides: simplified overview + 2 detail slides by package).
- The full test catalogue goes to the report, not the deck (allowed by DEC-008, 8.2).

---

## 3. Rules for the whole deck

### 3.1 Find and replace

| Find | Replace with | Decision |
|---|---|---|
| `IRSensorsThread`, `MQ2SensorThread` | `SafetySensorsThread` | DEC-003a |
| `OutputThread` | `AlarmControllerThread` (class `AlarmOutputs`) | DEC-003a/b |
| `StorageThread`, `NetworkMQTTThread` | `CommThread` (classes `EventStore`, `MqttLink`, `OutboxPublisher`, `Protocol`) | DEC-003a/b |
| `SupervisorThread` | `main` (classes `HealthMonitor` + `Watchdog`) | DEC-003a |
| `OutputCmd`, `StorageCmd`, `NetCmd`, three `CommandQueue<…>` | `CommCommand` on one `CommQueue`; outputs applied directly by the controller | DEC-003a/b |
| `IEventSink`, "committed event → Network" | remove | DEC-003a |
| `LinkMonitor`, netlink, "Wi-Fi loss ≤ 2 s", "wpa_supplicant event" (as detection) | remove; "broker loss detected by keep-alive (≤ 60 s)" | DEC-001 |
| epoll, eventfd, timerfd, "4 timerfds" | "waits on the event queue until the next deadline (pthread_cond, CLOCK_MONOTONIC)" | DEC-003a |
| `isSmokePresent()` | events `SMOKE_DETECTED` / `SMOKE_CLEARED` | DEC-003a |
| `EVT_BATTERY_LEVEL`, battery %, `battery_pct`, "10 % steps" | remove | DEC-001 |
| "≤ 30 %" (low battery) | "≤ 6.7 V" | DEC-005a |
| "shutdown", "power off" (critical battery) | "safe state" | DEC-005b |
| `PENDING_NOTIFICATION`, "outbox row" | `EVENT_LOG.status` / `sent_at`, "event stays PENDING" | DEC-006 |
| `get_events`, "App sync", "recovered by get_events" | remove | DEC-001 |
| "auth errors every 5 min", "Retry every 5 min" | "backoff 1 s → 60 s for every failure; logged once per episode" | DEC-006 |
| `PWM_DRIVER` (FaultComponent) | `SPEAKER` | DEC-004 |
| "Retry GPIO request every 10 s", "re-request the line every 10 s" | "FAULT once; component ignored until reboot" | DEC-004 |
| RGB "fault blink", "blinking at 1 Hz" | remove | DEC-001 |
| "Supervisor restarts it", "hung thread is restarted" | "watchdog resets the Pi; last ARMED/DISARMED state restored at boot" | DEC-001, DEC-004 |
| "Device Offline (clean)" with reason, "DISCONNECT 0x00", "DISCONNECT 0x04" | "planned stop: the Pi closes the connection without DISCONNECT, so the broker publishes the Last Will at once" | DEC-005b, DEC-012 |
| "starts DISARMED" | "restores the last ARMED/DISARMED state" | DEC-004 |
| `pattern_id`, 8 patterns in the driver, "base / one-shot" | "step sequences (≤ 8 steps) supplied by user space" | DEC-002, DEC-012 |
| "duplicates dropped by event_id" | "exact duplicates dropped; list in arrival order" | DEC-006 |
| IR "NPN open-collector (to be confirmed by measurement) → 3.3 V pull-up on GPIO" | "Schottky diode in series + external 10 kΩ pull-up to 3.3 V (safe for open-collector or 5 V output)" | DEC-007 |
| RGB "GPIO LOW = on", "active-low (sink)" (if present) | "NPN low-side per cathode; GPIO HIGH = on" | DEC-007 |
| "object in front of IR", "object in its detecting scope" | "the door/window monitored by IR1 moves (closed → open)" | DEC-010 |
| "PIR", "HC-SR501" (as the intrusion sensor) | "IR reflective sensor on the door/window frame" | DEC-010 |
| "Buzzer Module", "buzzer" (for the speaker) | "Speaker (FoneStar EP-36, 8 Ω, with NPN driver)" | D-46 (label correction) |
| `ALARM_SILENCED (INTRUSION)`, "Intrusion alarm silenced", "The intrusion version is a log event" | remove; stopping an intrusion alarm logs only `DISARMED` | DEC-014 |

### 3.2 Never presented

Battery percentage or `battery_pct`; power alerts at boot; database backup or restore; `get_events` or history sync; "N alerts received"; Wi-Fi loss ≤ 2 s; thread restarts by a supervisor; RGB fault blink; power-off on critical battery; measurement equipment or "tests in the laboratory"; where SOLID is not applied, and the claim "new event = EventType value"; summaries or justification lines about differences from the Analysis (P4-6).

### 3.3 SOLID notes (DEC-009)

One small footer line, only on these slides (new numbers):

| New | Slide | Note |
|---|---|---|
| 38 | Class Diagram — Overview | "SOLID – D: the alarm logic depends on nothing; main() injects real or simulated hardware." |
| 13 | IR and MQ-2 Sensor Interfaces | "SOLID – O: IR1, IR2, MQ-2 and the arm button are instances of one DebouncedInput class, configured by parameters." |
| 41 | Task Overview | "SOLID – S: threads only schedule; each component they call has one responsibility." |
| 52 | Flowchart — CommThread | "SOLID – S/I: EventStore stores, OutboxPublisher sends, Protocol formats; PinStore is a separate interface for PIN data." |
| 54 | Linux Device Driver | "SOLID – O: a new tone is data (a step sequence); the driver and its UAPI do not change." |
| 64 | Test Strategy | "SOLID – L: the same contract tests run on simulated and real hardware." |

### 3.4 How the deck is edited (P4-8, P4-9)

- **Text and tables:** Claude, only after David authorizes a batch, and only on the slides named in that authorization. If the deck is in Google Slides, through the Google Slides connector; if it is a PowerPoint file, by editing the `.pptx`. Each batch ends with a list of what changed.
- **Diagrams:** Claude drafts the `.drawio` source with SVG and PNG exports (§7); the team reviews them and places the PNGs.
- **Layout, images and visual polish:** the team.
- Never edit a slide or diagram outside the authorized batch.

### 3.5 Style

- Names exactly as in Master Context §E.3 (threads, classes, events, topics).
- Schematic legend colours by signal type (red 5 V, black GND, orange 3.3 V, blue I²C, purple PWM, green GPIO input, brown GPIO output).
- Every diagram readable at slide size (Guidelines §4); split rather than shrink.
- The Raspberry Pi text (old slide 6) is rewritten in the group's own words; it had been reused from G4.
- Title typos: old 59 "IRSenorsThread" disappears with the merge; old 71 "Parameters Times" → "Timing Parameters".

### 3.6 Readiness for several homes (DEC-015)

The Pi and the broker are ready for several homes (per-home `home_id`, users and ACL); the app serves one home. **The slides do not change for this** (DEC-015, 15.6): topics and examples keep `vigil/home1/…` (slides 22–34, diagrams D5–D9), user names on the slides stay as they are, and the AT-16 row keeps the text of §6.7. The cross-home isolation case of AT-16 goes only to the full test catalogue in the report. Do not replace `home1` by `<home_id>` on any slide.

---

## 4. New deck: 71 slides

| New | Title | From old | Action |
|---|---|---|---|
| 1 | Cover | 1 | Keep |
| 2 | Agenda | 2 | Update the list (§5) |
| 3 | Problem Statement | 3 | Keep |
| 4 | System Overview | 4 | Check labels |
| 5 | Hardware Architecture | 5 | Check labels |
| 6 | Hardware Specification — Processing and Power | 6, 7, 8 | **Merge** (P4-3) |
| 7 | Hardware Specification — Sensors and Inputs | 9, 10 (keypad), 11 (button) | **Merge** |
| 8 | Hardware Specification — Outputs | 10 (speaker), 11 (LEDs) | **Merge** |
| 9 | Hardware Connections | 12 | Edit diagram |
| 10 | Hardware Layout | 13 | Edit |
| 11 | GPIO Allocation | 14 | Edit |
| 12 | Power Budget | 15 | Label check |
| 13 | IR and MQ-2 Sensor Interfaces | 16, 17 | **Merge** (same thread) |
| 14 | Keypad and Push-Button Interfaces | 18, 19 | **Merge** (same thread) |
| 15 | RGB LED Interface | 20 (+ colour table from 52) | Edit |
| 16 | Red LED and Speaker Interfaces | 21, 22 | **Merge** (same driver) |
| 17 | UPS / Power Monitor Interface | 23 | Edit |
| 18 | Software Tools | 24 | Keep |
| 19 | Software COTS | 25 | Keep |
| 20 | Data Storage Specification | 26 | Redesign |
| 21 | Database ERD | 27 | Redraw |
| 22 | Cloud Architecture | 28 | Check |
| 23 | Message Specification — Remote Commands | 29 | Edit |
| 24 | Message Specification — System Status & Connections | 30 | Edit |
| 25 | Message Specification — Real-Time Alerts (1/3) | 31 | Edit |
| 26 | Message Specification — Real-Time Alerts (2/3) | 32 | Edit |
| 27 | Message Specification — Real-Time Alerts (3/3) | 33 | Edit |
| 28 | Message Specification — Log Events (1/2) | 34 | Edit |
| 29 | Message Specification — Log Events (2/2) | 35 | Edit |
| 30 | Message Specification — Broker Connection | 36 | Edit |
| 31 | MQTT — Connection and Alerts | 37 | Edit |
| 32 | MQTT — Remote Commands | 38 | Edit |
| 33 | MQTT — Connection Loss and Recovery | 39 | Edit |
| 34 | MQTT — Low Battery and Safe State | 40 | Redesign |
| 35 | Communication Stack | 41 | Edit |
| 36 | Wi-Fi Specification | 42 | Edit |
| 37 | Package Diagram | 43 | Redraw |
| 38 | Class Diagram — Overview (simplified by package) | 46 | Redraw |
| 39 | Class Diagram — Threads, Logic and Core | 44 | Redraw |
| 40 | Class Diagram — Devices, Hardware, Outputs, Storage and Network | 45 | Redraw |
| 41 | Task Overview | 47 | Redraw |
| 42 | Task Priority | 48 | Edit |
| 43 | Intrusion FSM | 49 | Edit |
| 44 | Fire FSM | 50 | Edit |
| 45 | Power Management FSM | 51 | Redraw |
| — | Connectivity LED FSM | 52 | **Removed** (P4-10) |
| 46 | Flowchart — Start-up | 53 | Redraw |
| 47 | Flowchart — AlarmControllerThread | 54, 57 | **Merge** |
| 48 | Flowchart — PIN Validation | 55 | Edit |
| 49 | Flowchart — SafetySensorsThread | 56, 59 | **Merge** |
| — | Flowchart — OutputThread | 58 | **Removed** (thread no longer exists) |
| 50 | Flowchart — LocalInputThread | 60 | Edit |
| 51 | Flowchart — PowerMonitorThread | 61 | Edit |
| 52 | Flowchart — CommThread | 62, 63 | **Merge** |
| 53 | Flowchart — main (Supervision) | 64 | **Replace** |
| 54 | Linux Device Driver | 65 | Edit |
| 55 | Linux Device Driver — Architecture | 66 | Redraw |
| 56 | Linux Device Driver — Flowcharts | 67 | Redraw |
| 57 | End-to-end Walkthrough — Intrusion and Fire | — | **New** (P4-7) |
| 58 | Design Patterns | 68 | Rewrite |
| 59 | Threads Responsibilities | 69 | Rewrite |
| 60 | SOLID Principles | 70 | Rewrite |
| 61 | Timing Parameters | 71 | Edit |
| 62 | GUI — States | 72 | Edit |
| 63 | GUI — Main Screen | 73, 74, 75 | **Merge** (P4-11) |
| 64 | Test Strategy | — | **New** (DEC-008) |
| 65–68 | Acceptance Tests (1/4 … 4/4) | 76–85 | **Replace** (DEC-008) |
| 69 | Error Handling — Critical Cases | 86 | Rewrite |
| 70 | Gantt Chart | 87 | Keep (not updated) |
| 71 | Thank you | 88 | Keep |

Count: 88 − 3 (hardware specification 6 → 3) − 3 (interfaces 8 → 5) − 1 (old 52) − 4 (flowcharts 12 → 8) − 2 (GUI 4 → 2) − 5 (tests 10 → 5) + 1 (walkthrough) = **71**.

---

## 5. Slide-by-slide actions (all 88 old slides)

| Old | Title | New | What changes |
|---|---|---|---|
| 1 | Cover | 1 | — |
| 2 | Agenda | 2 | "Test Cases" → "Test Strategy and Acceptance Tests"; add "End-to-end Walkthrough" after "Linux Device Drivers"; keep the other items |
| 3 | Problem Statement | 3 | — |
| 4 | System overview | 4 | No "PIR" or "buzzer" label (§3.1) |
| 5 | Hardware Architecture | 5 | No "PIR" or "buzzer" label (§3.1) |
| 6–8 | Hardware Specification (Pi, UPS HAT, batteries + adapter) | 6 | Merge; text in §6.1; **Pi text rewritten** |
| 9 | Hardware Specification (MQ-2, IR) | 7 | Merge with keypad (10) and button (11); text in §6.1. IR: Schottky + external pull-up; "door/window frame" wording (DEC-007, DEC-010) |
| 10 | Hardware Specification (speaker, keypad) | 7 (keypad), 8 (speaker) | "Buzzer Module" → "Speaker FoneStar EP-36" with its NPN driver (§6.1) |
| 11 | Hardware Specification (button, RGB, red LED) | 7 (button), 8 (LEDs) | RGB: "Anode at +5 V; 10 kΩ base pull-down keeps the LED off during boot"; the NPN line stays (§6.1) |
| 12 | Hardware Connections | 9 | IR1 and IR2: add a Schottky (BAT85) between OUT and the GPIO (cathode on the sensor side) and move the 10 kΩ pull-up to the GPIO side. RGB already correct. Optional outline "Interface board (protoboard)". Legend by signal type (§3.5) |
| 13 | Hardware Layout | 10 | Caption: "Bring-up on breadboard → interface board on protoboard → commercial plastic enclosure with cut-outs. IR on the door/window frame; MQ-2 high on the wall or ceiling." Replace the renders by a photo or sketch of the real box once it is bought, if they no longer match it |
| 14 | GPIO Allocation | 11 | GPIO 16 / 20: "Input pin for IR sensor 1 / 2 (door/window): external 10 kΩ pull-up + Schottky; LOW = closed". GPIO 4 / 26 / 19: "Output pin to drive the NPN of the RGB LED red / green / blue channel (HIGH = on)". GPIO 18: "PWM0 to control the speaker's tone" |
| 15 | Power Budget | 12 | "Buzzer" → "Speaker" |
| 16 | IR Sensor Module Interface | 13 | Merge with 17; condensed table in §6.2 (thread `SafetySensorsThread`, 100 ms sampling, 2-read confirmation, baseline at boot, FAULT once, door/window wording; removed: any edge, blocking edge wait, 200 ms retrigger guard, retry every 10 s) |
| 17 | MQ-2 Sensor Module Interface | 13 | Merge with 16 (§6.2): `SMOKE_DETECTED` / `SMOKE_CLEARED`; `isSmokePresent()` removed; FAULT once, no retry |
| 18 | Keypad Interface | 14 | Merge with 19 (§6.2): validation by `AlarmLogic (PinGuard)`; add FAULT(KEYPAD) |
| 19 | Push-Button Interface | 14 | Merge with 18 (§6.2): validation by `AlarmLogic (IntrusionFsm)`; failure covered by FAULT(KEYPAD) |
| 20 | RGB LED Interface | 15 | §6.2: thread `AlarmControllerThread (AlarmOutputs)`; set directly by the controller (no command queue); remove "Device fault", "Fault blink", "Fault blink priority"; add the network-state line from old 52 |
| 21 | Red LED Interface | 16 | Merge with 22 (§6.2): `AlarmOutputs`, sequences to `/dev/vigil_pwm`, no queue; FAULT(SPEAKER); no RGB fault blink |
| 22 | Speaker Interface | 16 | Merge with 21 (§6.2): tones as step sequences from user space; warning restored after a beep; one FAULT(SPEAKER) |
| 23 | UPS / Power Monitor Interface | 17 | §6.2: no battery %; ≤ 6.7 V; ≤ 6.4 V → safe state (reboot if mains returns first); silent initial state; FAULT(POWER_MONITOR) |
| 24 | Software Tools | 18 | No change from the decisions |
| 25 | Software COTS | 19 | No change from the decisions. (If the logos have no captions, adding "libgpiod v1, Paho MQTT C++, SQLite, OpenSSL, nlohmann-json" helps, but it is optional) |
| 26 | Data Storage Specification | 20 | **Redesign:** outbox = `status`/`sent_at` of `EVENT_LOG`; every event (alert and log): INSERT PENDING → publish → SENT on PUBACK; remove `PENDING_NOTIFICATION` and `get_events` reads; owner `CommThread` (`EventStore`, `OutboxPublisher`); "SQLite is a library linked into the vigil process (no server)"; database failure at boot: new database + default PIN + FAULT(DATABASE) + DISARMED; while running: syslog only; no backup |
| 27 | Database ERD | 21 | **Redraw with 2 tables:** `EVENT_LOG` (`id`, `event_type`, `timestamp`, `details`, `status`, `sent_at`) and `PIN_CONFIG`; remove `PENDING_NOTIFICATION` and the 1 : 0..1 relation |
| 28 | Cloud Architecture | 22 | No change from the decisions. **Check** whether it shows an "ONT" box and a "recommended UPS" (an older document asked for them; the Guide of 4 Nov declined them) and keep it consistent with the report |
| 29 | Message Specification – Remote Commands | 23 | Remove the "Get Events" command and its reply |
| 30 | Message Specification – System Status & Connections | 24 | Remove `battery_pct` and "only in 10 % steps"; remove "Device Offline (clean)" with `reason` and the path "WillDiscarded / Clean DISCONNECT 0x00"; add "Planned stop: the Pi closes the connection without DISCONNECT → the broker publishes the Last Will at once"; keep "Device Online" and the Last Will |
| 31 | Real-Time Alerts (intrusion, fire, silenced, cleared) | 25 | PUBACK / no-PUBACK boxes: "row" → "event (stays PENDING in EVENT_LOG)". Fire Alarm Silenced note: remove "The intrusion version is a log event." (DEC-014) |
| 32 | Real-Time Alerts (power) | 26 | **Power Lost:** `details` null. **Power Restored:** remove "or boot on mains after a SHUTDOWN"; `details` null. **Low Battery:** "on battery, ≤ 6.7 V for 3 samples"; `details` = voltage (e.g. "6.68 V"). **Shutdown:** "≤ 6.4 V for 3 samples → safe state (database closed, sync; keeps running until the HAT cuts power)"; `details` null; remove "Waits up to 5 s for PUBACK, then Device Offline (clean), DISCONNECT 0x00, power off" |
| 33 | Real-Time Alerts (fault, lockout) | 27 | **FaultComponent:** IR1, IR2, MQ2, KEYPAD, SPEAKER, POWER_MONITOR, DATABASE (remove PWM_DRIVER). **Trigger:** "first failure of a hardware component this boot, or database recreated at boot". PIN Lockout unchanged |
| 34–35 | Log Events | 28–29 | "No PUBACK: Not buffered; recovered by get_events" → "Event stays PENDING; resent after reconnection"; "PUBACK: Done" → "Mark event SENT". Remove the Alarm Silenced (INTRUSION) message and its outcome boxes (DEC-014); where an order is shown, one keypad PIN that stops both alarms logs DISARMED → ALARM_SILENCED (FIRE) |
| 36 | Broker Connection | 30 | 0x86 / 0x87: "Retry every 5 min; log once" → "Retry with backoff (1 s → 60 s); log once per episode"; remove "auth errors every 5 min" from the note |
| 37 | MQTT – Connection and Alerts | 31 | Outbox for every event; after CONNACK: online → status → pending events; remove any `get_events` reference |
| 38 | MQTT – Remote Command | 32 | The log event after the reply goes through the outbox |
| 39 | MQTT – Connection Loss and Recovery | 33 | Remove "Wi-Fi loss ≤ 2 s (wpa_supplicant event)", keep "broker loss ≤ 60 s"; remove "(auth errors: every 5 min)"; "Resend PENDING alerts" → "Resend PENDING events"; step 18 "duplicates dropped by event_id" → "exact duplicates dropped; list in arrival order"; remove step 20 ("N alerts received") and the "App sync" block (steps 21–26) |
| 40 | MQTT – Low Battery Shutdown | 34 | **Redesign as "Low Battery and Safe State":** ≤ 6.7 V → LOW_BATTERY (voltage); ≤ 6.4 V → SHUTDOWN logged (PENDING), published if connected, database closed + sync, system keeps running; HAT cuts power → broker publishes the Last Will ≈ 45 s later; mains returns → Pi boots, restores the last state, sends pending events; mains returns **before** the cut → controlled reboot. Remove steps 14–21 (offline with reason, DISCONNECT 0x00, power off, POWER_RESTORED at the next boot) |
| 41 | Communication Stack | 35 | Application box: "C++ NetworkMQTTThread" → "C++ CommThread (MqttLink)". **Check** the footnote "udhcpc": the Buildroot image uses dhcpcd |
| 42 | Wi-Fi Specification | 36 | "Link loss: wpa_supplicant event detected in ≤ 2 s" → "Link loss: detected by the MQTT keep-alive (≤ 60 s)" |
| 43 | Package diagram | 37 | **Redraw:** packages `threads`, `logic`, `devices`, `hw`, `outputs`, `storage`, `net`, `core`, plus Cloud and Mobile App; system interfaces `/dev/vigil_pwm` (own driver), libgpiod, i2c-dev, `/dev/watchdog` (used by `main`) |
| 46 | Class Diagram – Overview | 38 | **Redraw** as the simplified diagram by package (≈ 12 classes: interfaces, real/simulated realisations, `WorkerThread`, composition of `AlarmLogic`, dependency direction). Must show every name used in the SOLID map. SOLID note |
| 44 | Class Diagram (1) | 39 | **Redraw** (detail): `threads`, `logic`, `core` |
| 45 | Class Diagram (2) | 40 | **Redraw** (detail): `devices`, `hw`, `outputs`, `storage`, `net` |
| 47 | Task overview | 41 | **Redraw:** 5 threads + `main`; EventQueue (all → controller) and CommQueue (controller + Paho → Comm); heartbeats → `main` → `/dev/watchdog`; remove the three command queues, "committed event" flows, the Supervisor box and "4 timerfds". SOLID note |
| 48 | Task priority | 42 | FIFO 80: SafetySensorsThread, AlarmControllerThread. FIFO 50: LocalInputThread, PowerMonitorThread. SCHED_OTHER: CommThread, `main`. Remove the FIFO 70 band. Legend "higher priority ↑" and one line per band (§6.3) |
| 49 | Intrusion FSM | 43 | Initial transition: "[last state ARMED] → ARMED / [else] → DISARMED (no event logged)". Motion wording per DEC-010. If the INTRUSION_ALARM → DISARMED transition lists ALARM_SILENCED (INTRUSION), remove it (DEC-014) |
| 50 | Fire FSM | 44 | The "[smoke present]" guard uses the state kept from `SMOKE_DETECTED` / `SMOKE_CLEARED` |
| 51 | Power management FSM | 45 | **Redraw:** silent initial state (no POWER_RESTORED "BOOT" or POWER_LOST at boot); LOW_BATTERY guard ≤ 6.7 V; SHUTDOWN → **SAFE_STATE**, entry action "SHUTDOWN alert, close DB, sync; keeps running"; SAFE_STATE + mains back → "controlled reboot"; remove the battery-level self-loop; POWER_LOST without percentage |
| 52 | Connectivity LED FSM | — | **Remove** (P4-10); colour table and network-state line move to new 15 |
| 53 | Flowchart – StartUp | 46 | **Redraw:** request each GPIO line separately (failure → FAULT, degraded start); open `/dev/vigil_pwm` (failure → FAULT(SPEAKER)); database: open + quick_check → OK: load PIN / failure: rename the file, new database, default PIN hash, FAULT(DATABASE) / first boot: create, no FAULT; no backup branch; read the last ARMED/DISARMED → initial state; create EventQueue + CommQueue; open the watchdog; start threads (Comm, Controller, then Power, SafetySensors, LocalInput); power state decided silently by the first 3 samples; MQ-2 warm-up in parallel; `main` enters its supervision loop |
| 54 + 57 | Flowchart – Control loop; AlarmControllerThread | 47 | **Merge:** wait on the EventQueue until the next deadline (max 1 s) → drain the queue → `AlarmLogic.handle(event, now)` / expired deadlines → `AlarmOutputs.apply()` → `CommCommand`s → heartbeat. No epoll, eventfd or timerfd |
| 55 | Flowchart – PIN validation | 48 | Names → `AlarmLogic` / `PinGuard`; logic unchanged, except: if a box logs ALARM_SILENCED (INTRUSION), remove it (DEC-014) |
| 56 + 59 | Flowchart – MQ2SensorThread; IRSensorsThread | 49 | **Merge into `SafetySensorsThread`:** every 100 ms read IR1, IR2, MQ-2 → `DebouncedInput` (2 reads for IR; 5 reads + warm-up for MQ-2) → transitions → events; a failing sensor → FAULT once, the others continue; heartbeat |
| 58 | Flowchart – OutputThread | — | **Remove** |
| 60 | Flowchart – LocalInputThread | 50 | `KeypadScanner` + `PinEntry` + `DebouncedInput` (button); line failure → FAULT(KEYPAD); heartbeat checked by `main` |
| 61 | Flowchart – PowerMonitorThread | 51 | `PowerSupplyMonitor`; 3-sample rules (mains lost/back, ≤ 6.7 V once per outage, ≤ 6.4 V critical); no battery-level events; consecutive I²C failures → FAULT(POWER_MONITOR); silent initial state |
| 62 + 63 | Flowchart – StorageThread; NetworkMQTTThread | 52 | **Merge into `CommThread`:** read the CommQueue (timeout) → LogEvent → `EventStore.append` (PENDING) → `OutboxPublisher` publishes pending events in `id` order, ≤ 5/s → SENT on PUBACK; `MqttLink`: connect, backoff 1 → 60 s; on CONNACK: online → status → pending; incoming `cmd` → `Protocol.parse` → EventQueue; SavePin → `PinStore` → result event; EnterSafeState → close the database; write failure → syslog; SIGTERM → close the connection **without** DISCONNECT. SOLID note |
| 64 | Flowchart – SupervisorThread | 53 | **Replace by the `main` loop:** every 1 s `HealthMonitor` checks the heartbeats (1 / 1 / 3 / 3 / 10 s) → all fresh? feed the watchdog : stop feeding (reset in 15 s); mains back in SAFE_STATE → `sync` + reboot; SIGTERM → stop threads, outputs off, close the database, watchdog magic close, close the connection without DISCONNECT |
| 65 | Linux Device Driver | 54 | **UAPI:** replace the `pattern_id` table with a sequence command (channel, repeat, step count ≤ 8, steps {frequency_hz, duty, duration_ms}); "a sequence with 0 steps stops the channel (output LOW)". "Tone and blink patterns stepped in the kernel" → "Sequences supplied by user space, stepped by hrtimers". Remove "Repeating patterns become the channel's base; one-shots play over it…". "OutputThread is the only client" → "AlarmOutputs (in AlarmControllerThread) is the only client". Keep duty ≤ 50 %, output LOW when stopped, fail-loud `release()`, error codes (EINVAL includes invalid sequences). SOLID note |
| 66 | Linux Device Driver – Architecture | 55 | **Redraw:** `AlarmControllerThread` → `AlarmOutputs` → `PwmDevice.write(sequence)`; kernel: char device (open / write / release, mutex) → per-channel sequence player (hrtimer) → PWM consumer API → `pwm-bcm2835` → GPIO18 / GPIO13. Remove the pattern tables and the base/one-shot logic |
| 67 | Linux Device Driver – Flowchart | 56 | **Redraw:** `write()` (size, copy, validate → lock → cancel timer → 0 steps? stop, output LOW : apply step 0 → start timer if more steps) and the hrtimer callback (next step; at the end repeat from the start or stop with the output LOW) |
| — | (new) End-to-end Walkthrough | 57 | **New** (§6.5) |
| 68 | Design Patterns | 58 | **Rewrite** with the 8 patterns of §6.6 |
| 69 | Threads Responsibilities | 59 | **Rewrite** (§6.3) |
| 70 | SOLID Principles | 60 | **Replace** by the evidence map (§6.6); remove "Each thread has one job", "new event = EventType value", "New tone = row in the driver pattern table", "The 8 WorkerThread subclasses used by SupervisorThread", `IEventSink` |
| 71 | Parameters Times | 61 | Title → "Timing Parameters"; table in §6.9 |
| 72 | GUI Interface (states) | 62 | No battery percentage; no history sync; "Offline" without time; no "Intrusion alarm silenced" item (DEC-014) |
| 73–75 | GUI Interface (mock-ups) | 63 | One slide: one updated mock-up (status card without battery %, Arm / Disarm / Change PIN, last 20 events in arrival order, intrusion text "Intrusion – door/window moved (IR1)"); no reconnection summary |
| 76–85 | Test cases (10 slides) | 64–68 | **Replace** by the test strategy and 24 acceptance tests (§6.7). The old tests go to the full catalogue **in the report**, corrected: remove "Event Sync", "History Query", "Driver Concurrency"; "Start-up: Power on → DISARMED" → "restores the last ARMED/DISARMED state"; "Retry every 5 minutes" → "retry with backoff up to 60 s"; "safe shutdown" → "safe state"; "Wi-Fi Loss" detected in ≤ 60 s; "Write channel, frequency, duty, enable" → "Write step sequences"; "object in front of IR" → "the door/window monitored by IR1 opens"; add the IR glitch, the FAULT per component and the corrupt database at boot |
| 86 | Error Handling – Critical Cases | 69 | **Rewrite** from §6.8 |
| 87 | Gantt Chart | 70 | **No change** (decision: not updated) |
| 88 | Thank you | 71 | — |

---

## 6. Text for new, merged and rewritten slides (English)

### 6.1 Hardware specification (new 6–8)

**New 6 — "Hardware Specification — Processing and Power"** (keep the photos of the Pi, the HAT and the batteries)

- **Raspberry Pi 4 Model B** — runs the whole system: a quad-core ARM Cortex-A72 (BCM2711) with a 40-pin header at 3.3 V logic (not 5 V tolerant), two hardware PWM channels (speaker and red LED), I²C for the power monitor and built-in dual-band Wi-Fi. The microSD card holds the Buildroot image and the SQLite database. There is no real-time clock: time comes from NTP.
- **UPS HAT** — two 18650 Li-ion cells in series (6.0–8.4 V); 5 V regulator up to 2.5 A to the Pi through the header; INA219 monitor at I²C address 0x42 (battery voltage and current); protection against over-charge, over-discharge, over-current, short circuit and reverse polarity, with balanced charging.
- **Batteries** — 18650 Li-ion, 2600 mAh, 2S: 7.2–7.4 V nominal, 8.4 V fully charged.
- **AC/DC adapter** — 8.4 V DC from the mains; charges the batteries and powers the HAT.

**New 7 — "Hardware Specification — Sensors and Inputs"**

- **MQ-2 gas/smoke module** — heated SnO₂ sensor; detects LPG, methane, hydrogen, alcohol and smoke (≈ 300–10 000 ppm); 5 V supply, heater ≈ 120–150 mA; digital output (LM393 comparator) active-low at 5 V, reduced to 3.3 V by a 10 kΩ / 20 kΩ divider.
- **IR reflective sensor ×2** — mounted on the door/window frame: LOW while the leaf is in front (closed), HIGH when it moves away (open); 5 V supply; response < 2 ms. Output open-collector or 5 V (to be measured); interface: Schottky diode (BAT85) in series + external 10 kΩ pull-up to 3.3 V, safe for both.
- **Keypad 3×4** — 12 keys (0–9, \*, #); 4 rows (inputs with pull-ups) and 3 columns (scanned outputs); 3.3 V logic.
- **Push-button** — arm button to GND, internal pull-up, active-low.

**New 8 — "Hardware Specification — Outputs"**

- **Speaker FoneStar EP-36** — 8 Ω, 0.3 W; driven by PWM0 through an NPN (BC337, 470 Ω base, 10 kΩ base pull-down), 33 Ω / 0.5 W series resistor and 1N4148 flyback diode; peak ≈ 120 mA; duty ≤ 50 %.
- **Red LED** — 5 mm, V_F 1.8–2.4 V, ≈ 6 mA through 220 Ω; driven by PWM1.
- **RGB LED** — common anode at +5 V; each cathode switched by an NPN (BC337, 1 kΩ base, 10 kΩ base pull-down keeps the LED off during boot) — GPIO HIGH = ON; 390 Ω (R), 220 Ω (G), 220 Ω (B), ≈ 8 mA per channel.

### 6.2 Interface slides (new 13–17)

Two tables side by side on slides 13, 14 and 16; one table on 15 and 17. Rows not listed here are removed.

**New 13 — "IR and MQ-2 Sensor Interfaces"**

| IR sensors (×2) | |
|---|---|
| Responsible thread | SafetySensorsThread (DebouncedInput ×2) |
| GPIO | 16 (IR1), 20 (IR2); Schottky + external 10 kΩ pull-up |
| Input state | LOW = door/window closed; HIGH = open |
| Activation | Periodic 100 ms sampling |
| Detection | Closed → open confirmed by 2 consecutive reads (one event per opening) |
| Initialization | First reading is the baseline (no event at boot) |
| Event generated | EVT_MOTION_DETECTED(sourceId) → INTRUSION_ALARM details IR1/IR2 |
| Alarm-state filtering | AlarmLogic (IntrusionFsm) |
| Fault | Read or request error → FAULT(IR1/IR2) once; sensor ignored until reboot |

| MQ-2 sensor | |
|---|---|
| Responsible thread | SafetySensorsThread (DebouncedInput) |
| GPIO | 21 via 10 kΩ / 20 kΩ divider, bias disabled; LOW = smoke |
| Sampling | 100 ms, continuous, armed or disarmed |
| Warm-up | First 60 s: no events (value to be measured) |
| Smoke detected | 5 consecutive LOW reads → SMOKE_DETECTED |
| Smoke cleared | 5 consecutive HIGH reads → SMOKE_CLEARED |
| Fault | FAULT(MQ2) once; sensor ignored until reboot |

SOLID note (§3.3).

**New 14 — "Keypad and Push-Button Interfaces"**

| Keypad | |
|---|---|
| Responsible thread | LocalInputThread (KeypadScanner + PinEntry) |
| Scanning / debounce | 15 ms / 2 consecutive stable scans (≈ 30 ms) |
| 0–9 | Append to the PIN buffer (max 8; 9th digit → error beep) |
| # / \* / 10 s idle | Submit / clear / clear |
| Multiple keys, long press | Ignored / one event until release |
| Valid PIN | 4–6 digits |
| Events | EVT_KEY_DIGIT (no digit), EVT_PIN_SUBMIT (complete PIN) |
| Validation | AlarmLogic (PinGuard); during a lockout nothing is checked |
| Fault | FAULT(KEYPAD) once; remote disarm still available |

| Push-button | |
|---|---|
| Responsible thread | LocalInputThread (DebouncedInput) |
| Sampling / debounce | 15 ms / 2 consecutive stable scans |
| Press | HIGH → LOW, one event until release → EVT_BUTTON_PRESS |
| DISARMED, no fire alarm | Key beep, 60 s exit delay, red LED on |
| Fire alarm active | Rejected, no beep |
| Other states | Ignored, error beep (no beep while an alarm sounds) |
| Validation / fault | AlarmLogic (IntrusionFsm) / covered by FAULT(KEYPAD) |

**New 15 — "RGB LED Interface"**

| Parameter | Specification |
|---|---|
| Responsible thread | AlarmControllerThread (AlarmOutputs, OutputPolicy) |
| Interface | libgpiod; GPIO 4 / 26 / 19 → NPN; HIGH = ON |
| Activation | Set directly by the controller when the power source or the broker connection changes |
| Mains + broker connected | Green |
| Mains + broker lost | Yellow (red + green) |
| Battery + broker connected | Blue |
| Battery + broker lost | Magenta (red + blue) |
| Broker connection | Connected after CONNACK; lost when the keep-alive fails (≤ 60 s) |
| Initial state | Broker "lost" until the first CONNACK: yellow on mains, magenta on battery |

**New 16 — "Red LED and Speaker Interfaces"**

| Red LED | |
|---|---|
| Responsible thread | AlarmControllerThread (AlarmOutputs) |
| Driver | /dev/vigil_pwm, PWM1 (GPIO13), step sequences |
| Disarmed / armed, exit and entry delay | OFF / steady ON |
| Active alarm | Flashing 250 ms ON / 250 ms OFF |
| Wrong PIN | 3 flashes (100 ms ON / 100 ms OFF); dropped during an alarm |
| Priority | Alarm flashing → wrong PIN → normal state |
| Fault | FAULT(SPEAKER) (same driver); the RGB LED keeps showing power and network |

| Speaker | |
|---|---|
| Responsible thread | AlarmControllerThread (AlarmOutputs) |
| Driver | /dev/vigil_pwm, PWM0 (GPIO18), duty ≤ 50 % |
| Key beep / error beep | 2 kHz 50 ms / 2 × 150 ms at 700 Hz, 100 ms gap |
| Entry warning | 1 kHz for 100 ms every 1 s (no tone in the exit delay) |
| Intrusion / fire | 800 / 1200 Hz alternating 250 ms / 2 / 3 kHz alternating 150 ms |
| Beeps | Played over the entry warning, which is then restored; dropped during fire or intrusion |
| Priority | Fire → intrusion → entry warning → beeps |
| Fault | FAULT(SPEAKER) once |

**New 17 — "UPS / Power Monitor Interface"**

| Parameter | Specification |
|---|---|
| Responsible thread | PowerMonitorThread (PowerSupplyMonitor, Ina219) |
| Interface | I²C-1 (/dev/i2c-1), INA219 at 0x42 |
| Sampling | 1 s; every decision needs 3 consecutive samples |
| Mains lost / restored | Current below −100 mA / above +50 mA; near zero keeps the previous state |
| Initial state | Decided by the first 3 samples, without an alert |
| Low battery | ≤ 6.7 V on battery, once per outage → LOW_BATTERY (voltage) |
| Critical battery | ≤ 6.4 V on battery → safe state; if mains returns before the HAT cuts power, the Pi reboots |
| Events | EVT_POWER_LOST, EVT_POWER_RESTORED, EVT_BATTERY_LOW, EVT_BATTERY_CRITICAL |
| I²C failure | FAULT(POWER_MONITOR) after 10 consecutive read failures |

### 6.3 Task priority and thread responsibilities (new 42 and 59)

**New 42 — "Task Priority"** — three bands with a "higher priority ↑" legend:

- **SCHED_FIFO 80 — SafetySensorsThread, AlarmControllerThread:** "the alarm path: a sensor event reaches the speaker without waiting for anything else."
- **SCHED_FIFO 50 — LocalInputThread, PowerMonitorThread:** "human-speed and slow inputs."
- **SCHED_OTHER — CommThread, main:** "disk and network work, never on the alarm path."

**New 59 — "Threads Responsibilities"**

| Thread | Role | Activation | Priority | Heartbeat timeout |
|---|---|---|---|---|
| LocalInputThread | Keypad scan and arm button | Every 15 ms | SCHED_FIFO 50 | 1 s |
| SafetySensorsThread | IR1, IR2, MQ-2 sampling and confirmation | Every 100 ms | SCHED_FIFO 80 | 1 s |
| PowerMonitorThread | INA219 voltage and current | Every 1 s | SCHED_FIFO 50 | 3 s |
| AlarmControllerThread | Runs AlarmLogic; drives speaker, LEDs | Event queue, until the next deadline (≤ 1 s) | SCHED_FIFO 80 | 3 s |
| CommThread | SQLite and MQTT (outbox, commands) | Comm queue | SCHED_OTHER | 10 s |
| main | Start-up; watchdog fed only while all heartbeats are fresh; SIGTERM | Every 1 s | SCHED_OTHER | watchdog 15 s |

### 6.4 Class diagram (new 38–40)

- **38 Overview:** about 12 classes — `WorkerThread` and its 5 subclasses, `AlarmLogic` with its parts, the 4 hardware interfaces with real and simulated realisations, `BlockingQueue`, `EventStore`, `MqttLink` — and the dependency direction (`logic` depends on nothing; `main` is the composition root).
- **39 Detail 1:** packages `threads`, `logic`, `core`.
- **40 Detail 2:** packages `devices`, `hw`, `outputs`, `storage`, `net`.
- UML rules: one relation type per meaning (realisation dashed with a hollow triangle; composition with a filled diamond; dependency dashed arrow); no crossing curved lines; the names of Master Context §E.3.

### 6.5 New 57 — "End-to-end Walkthrough — Intrusion and Fire" (P4-7)

**What it is.** One slide that follows a real event from the physical world to the phone, step by step, naming at each step the component that handles it and the time it takes. The rest of the deck presents the parts separately (sensors, threads, FSMs, driver, database, MQTT); this slide shows them working together.

**Why it is worth a slide.**

- It answers the question the professor will ask in some form: "what happens, exactly, when someone opens the door?"
- It justifies the real-time claims with a time budget per step, instead of a single "≤ 1 s".
- It shows that the design is coherent: every thread, queue and class introduced earlier appears in one flow, and nothing is missing.
- It is a natural anchor for the oral presentation, and it is something the other groups did not have.

**Layout.** Two columns, intrusion on the left and fire on the right, each a numbered sequence; the component of each step in bold and the time on the right. Optionally, a thin swimlane strip on top with the lanes Sensor → SafetySensorsThread → AlarmControllerThread → /dev/vigil_pwm → CommThread + SQLite → EMQX → App.

**Text:**

| Intrusion (system armed) | Time |
|---|---|
| 1. Door opens: IR1 goes HIGH; **SafetySensorsThread** confirms it on the 2nd read → EVT_MOTION_DETECTED (IR1) | ≤ 200 ms |
| 2. **AlarmControllerThread** (IntrusionFsm) → ENTRY_DELAY: warning tone sequence to **/dev/vigil_pwm**; entry deadline set | ms |
| 3. No valid PIN before the deadline → INTRUSION_ALARM: intrusion tone, red LED flashing | 30 s |
| 4. **CommThread** stores INTRUSION_ALARM as PENDING in **EVENT_LOG**, publishes it on `alert`, marks it SENT on PUBACK | ms |
| 5. **EMQX** delivers it; the app's foreground service shows "Intrusion – door/window moved (IR1)" | ≤ 5 s |
| 6. A valid PIN (keypad or app) → DISARMED | — |

| Fire (any state) | Time |
|---|---|
| 1. Smoke: MQ-2 output LOW; **SafetySensorsThread** confirms it after 5 reads → SMOKE_DETECTED | ≈ 0.5 s |
| 2. **AlarmControllerThread** (FireFsm) → FIRE_ALARM: fire tone (has priority), red LED flashing — no network needed | ≤ 1 s total |
| 3. **CommThread**: FIRE_ALARM alert through the outbox; remote disarm is rejected while it lasts | ≤ 5 s to the phone |
| 4. Valid keypad PIN → SILENCED for 10 min, system DISARMED | — |
| 5. After 10 min: smoke still present → FIRE_ALARM (REACTIVATED); otherwise HAZARD_CLEARED | 10 min |

Footer: "Offline, steps 4–5 wait in EVENT_LOG as PENDING and are sent in order after reconnection; the local alarm is never affected."

### 6.6 Design patterns and SOLID (new 58 and 60)

**New 58 — "Design Patterns":**

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

**New 60 — "SOLID Principles":**

| Principle | Where | Evidence |
|---|---|---|
| S — Single responsibility | Threads only schedule; `AlarmLogic` decides; `OutputPolicy` owns the output rules; `EventStore` / `OutboxPublisher` / `MqttLink` / `Protocol` | A new tone changes only `OutputPolicy`; a new message format changes only `Protocol` |
| O — Open/closed | `IGpioInput`, `IPwm`, `IPowerSensor`, `IMessageTransport`; `DebouncedInput` configured by parameters; tones as step sequences | New sensor = new `DebouncedInput` instance (plus one FAULT enum value); new tone = new sequence, driver unchanged; simulated hardware = new class, callers unchanged; RGB active level changed by configuration only (DEC-007) |
| L — Liskov substitution | `WorkerThread` subclasses; real and simulated hardware classes | Same contract tests pass on simulated (L1/L2) and real (L3) hardware; one heartbeat and error policy for all threads (no thread restarts) |
| I — Interface segregation | `IGpioInput` / `IGpioOutput` / `IPwm` / `IPowerSensor`; `EventStore` vs `PinStore`; `CommCommand` as small structs | No client depends on methods it does not use; the union-like command structs are gone |
| D — Dependency inversion | `logic` depends on nothing; devices and outputs depend on hardware interfaces; `OutboxPublisher` on `IMessageTransport`; `main` is the composition root | `AlarmLogic` is tested on the laptop with no hardware and injected time |

### 6.7 Test strategy and acceptance tests (new 64–68, DEC-008)

**New 64 — "Test Strategy":**

- **L1 – Host unit tests (GoogleTest):** alarm logic with injected time, devices on simulated hardware, storage on in-memory SQLite, message protocol.
- **L2 – Host integration:** simulation build + Mosquitto broker + mobile app.
- **L3 – Raspberry Pi:** real hardware, custom driver, power, real time under load, EMQX + mobile app.
- Every Analysis requirement is covered by at least one acceptance test.
- The same contract tests run on simulated and real hardware.
- The full test catalogue is in the report.

SOLID note (§3.3).

**New 65–68 — "Acceptance Tests (1/4 … 4/4)"**, 6 tests per slide:

| ID | Objective | Stimulus | Expected result | Level | Req. |
|---|---|---|---|---|---|
| AT-01 | Local arming with exit delay | Disarmed; press the arm button | Short beep; red LED on; ARMED when the exit delay ends; event shown in the app | L3 | Flexible Control, Entry/Exit Delay, Visual Status |
| AT-02 | Disarm during the entry delay | Armed; open the door (IR1); correct PIN before the delay ends | Warning tone during the delay; DISARMED; no alarm, no alert | L3 | Entry/Exit Delay, Flexible Control |
| AT-03 | Intrusion alarm | Armed; open the door; let the entry delay expire | Intrusion tone; red LED flashing; INTRUSION alert on the phone ≤ 5 s; on until disarmed | L3 | Dual Detection, Distinct Alarms, Instant Alerts |
| AT-04 | PIN lockout | Enter 5 wrong PINs | Error beep each time; PIN_LOCKOUT alert; PIN entry ignored until the lockout ends | L1, L3 | Security, Flexible Control |
| AT-05 | Fire alarm in any state | Smoke on the MQ-2 when disarmed, in exit delay and armed | Fire tone, distinct from intrusion; FIRE alert in all three states | L3 | Dual Detection, Distinct Alarms |
| AT-06 | Fire silence period | Fire alarm; correct PIN; smoke kept until the period ends | Silenced; smoke ignored during the period; alarm and alert again if smoke remains | L1, L3 | Fire Alarm Silence Period |
| AT-07 | Remote control | App: arm; disarm with PIN; disarm during a fire alarm | Armed at once (no exit delay); disarmed; disarm during fire rejected, alarm continues | L2, L3 | Flexible Control |
| AT-08 | PIN change from the app | Wrong current PIN; then correct PIN; reboot | Wrong: error, PIN unchanged. Correct: new PIN works after reboot; only a salted hash stored | L2 | PIN Management, Security |
| AT-09 | IR glitch rejection | Armed; one isolated 100 ms pulse on the IR input | No event; no entry delay | L1 | Dual Detection |
| AT-10 | MQ-2 warm-up | Cold boot in clean air | No smoke event during or after warm-up | L3 | Dual Detection |
| AT-11 | Mains failure and return | Unplug the 8.4 V adapter; plug it back | No reset; monitoring continues; POWER_LOST / POWER_RESTORED alerts; RGB colour changes | L3 | Power Resilience, Instant Alerts, Visual Status, Availability |
| AT-12 | Low battery and safe state | On battery to ≤ 6.7 V, then ≤ 6.4 V; reconnect mains | LOW_BATTERY with voltage; SHUTDOWN alert, database closed, system keeps running; Pi boots on mains with the last state restored | L3 | Power Resilience |
| AT-13 | Battery autonomy | Typical load on battery until safe state | ≥ 3 h (theoretical ≈ 4 h) | L3 | Power Resilience |
| AT-14 | Offline events | Stop the broker; generate events; restart it | RGB shows loss; local alarm works; all events delivered in order, ≤ 5 per second; no duplicates in the app | L2, L3 | Pending Notification Handling, Connectivity Monitoring, Event Logging |
| AT-15 | Offline detection | Cut the Pi's power; separately, stop the service | Power cut: "offline" within 1.5 × keep-alive. Planned stop: "offline" at once | L2, L3 | Instant Alerts |
| AT-16 | Secure channel | Wrong CA; app publishes on a device topic; expired command | Connection refused; publish rejected by the broker; command not executed | L2, L3 | Security |
| AT-17 | State after reboot | Reboot while armed; reboot while disarmed | Returns ARMED / DISARMED; a door already open does not trigger | L2, L3 | Availability |
| AT-18 | Watchdog recovery | Block one thread (test build) | Pi reset by the watchdog; last state restored | L3 | Availability |
| AT-19 | Component fault | Simulated read failure on IR1 | FAULT(IR1) sent once; everything else keeps working | L2 | Availability |
| AT-20 | Corrupt database at boot | Corrupt the database file; boot | New database, default PIN, FAULT(DATABASE), DISARMED | L2, L3 | Event Logging, Availability |
| AT-21 | Custom PWM driver | Write a valid sequence; an invalid one; close the device | Output follows the steps; invalid → EINVAL; output LOW when stopped | L3 | Distinct Alarms, Visual Status |
| AT-22 | Alarm latency under load | CPU stress, database writes and a pending backlog; then smoke or door opening | Alarm output ≤ 1 s | L3 | Real-Time Responsiveness |
| AT-23 | Keypad response and periodic tasks | Key presses and sampling loops under the same load | Key → beep ≤ 100 ms; 15 ms and 100 ms loops with no missed period | L3 | Real-Time Responsiveness |
| AT-24 | Long-term stability | 24 h with periodic events | No crash, no reset, no memory growth | L3 | Availability |

### 6.8 New 69 — "Error Handling — Critical Cases" (DEC-004)

| Component | Response | What keeps working |
|---|---|---|
| IR1 / IR2 | FAULT(IR1/IR2) once; sensor ignored until reboot | Everything else |
| MQ-2 | FAULT(MQ2) once | Everything except fire detection |
| Keypad / button | FAULT(KEYPAD) once | Remote disarm (a fire alarm cannot be silenced until reboot) |
| Speaker / PWM driver | FAULT(SPEAKER) once | RGB LED and alerts |
| INA219 | FAULT(POWER_MONITOR) once | Everything except mains-failure detection, low battery and safe state |
| Database at boot | New database, default PIN, FAULT(DATABASE), DISARMED | Everything |
| Database while running | syslog only | Local alarm and the PIN in memory |
| Broker / Wi-Fi | Not a fault: reconnect with backoff 1 s → 60 s; events stay PENDING; RGB shows the loss | All local functions |
| Unexpected error / hung thread | Heartbeat missed → watchdog resets the Pi → last ARMED/DISARMED state restored | — |

### 6.9 New 61 — "Timing Parameters"

| Parameter | Value |
|---|---|
| Exit delay / entry delay | 60 s / 30 s |
| Fire silence period | 10 min |
| PIN lockout | 5 wrong PINs → 5 min |
| Sampling | keypad and button 15 ms; IR and MQ-2 100 ms; INA219 1 s |
| Confirmation | IR 2 reads; MQ-2 5 reads; button 2 reads; power 3 samples |
| MQ-2 warm-up | 60 s (to be measured) |
| Low battery / safe state | ≤ 6.7 V / ≤ 6.4 V (3 samples, on battery) |
| Heartbeat timeouts / watchdog | 1–10 s per thread / 15 s |
| MQTT keep-alive (Pi / app) | 30 s / 60 s |
| Last Will published by the broker | ≈ 45 s after the Pi goes silent; at once on a planned stop |
| Broker-loss detection (RGB) | ≤ 60 s |
| Reconnect backoff | 1 s → 60 s, every failure |
| Command Message Expiry / app timeout | 30 s / 32 s |
| Outbox pacing | ≤ 5 messages/s |

### 6.10 GUI (new 62–63)

- **62 States:** status card priority fire > intrusion > fire silenced > armed/disarmed; power row "Mains" / "Battery" (no percentage); "Offline" without time; button rules per state.
- **63 Main screen:** one updated mock-up — status card, Arm / Disarm / Change PIN, list of the last 20 events in arrival order (example item "Intrusion – door/window moved (IR1)").

---

## 7. Diagrams to draw (draw.io, P4-8 B)

Claude drafts each `.drawio` source with SVG and PNG exports in `docs/diagrams/<area>/`; the team reviews them before they go on a slide. Every draft needs authorization first, like any other file.

| # | Diagram | New slide | Type | Work |
|---|---|---|---|---|
| D1 | Hardware connections (schematic) | 9 | Schematic | Schottky + pull-up on the IR lines; legend by signal type |
| D2 | Hardware layout | 10 | Sketch or photo | When the box is bought |
| D3 | Data storage specification | 20 | Block diagram | Redraw |
| D4 | Database ERD | 21 | ERD | Redraw with 2 tables |
| D5 | Message flow boxes (PUBACK / no PUBACK) | 25–29 | Small flow boxes | Edit labels |
| D6 | MQTT — connection and alerts | 31 | UML sequence | Edit |
| D7 | MQTT — remote commands | 32 | UML sequence | Edit |
| D8 | MQTT — connection loss and recovery | 33 | UML sequence | Edit (remove steps 20–26) |
| D9 | MQTT — low battery and safe state | 34 | UML sequence | Redraw |
| D10 | Communication stack | 35 | Layer diagram | Edit labels |
| D11 | Package diagram | 37 | UML package | Redraw |
| D12–D14 | Class diagram: overview + 2 detail slides | 38–40 | UML class | New |
| D15 | Task overview (concurrency view) | 41 | Block / UML | Redraw |
| D16 | Task priority | 42 | Bands | Redraw |
| D17 | Intrusion FSM | 43 | UML state | Edit initial transition |
| D18 | Fire FSM | 44 | UML state | Edit guard |
| D19 | Power FSM | 45 | UML state | Redraw |
| D20–D27 | Flowcharts: start-up, controller, PIN, SafetySensors, LocalInput, PowerMonitor, Comm, main | 46–53 | Flowchart | 5 redraw/new, 3 edit |
| D28 | Driver architecture | 55 | Block diagram | Redraw |
| D29 | Driver flowcharts (`write()`, hrtimer) | 56 | Flowchart | Redraw |
| D30 | Walkthrough swimlane strip (optional) | 57 | Swimlane | New |

Unchanged: system overview (4), hardware architecture (5), cloud architecture (22, after the check), Gantt (70).

---

## 8. Checklist before presenting

- [ ] Search the deck for: battery %, battery_pct, get_events, sync, N alerts, netlink, ≤ 2 s, Supervisor, restart, blink, power off, laboratory, oscilloscope, PIR, buzzer, PENDING_NOTIFICATION, timerfd, epoll, OutputThread, StorageThread, NetworkMQTTThread, IRSensorsThread, MQ2SensorThread, DISCONNECT, "every 5 min", "Intrusion alarm silenced", ALARM_SILENCED (INTRUSION), "Analysis" (no divergence notes). None may remain.
- [ ] Every thread, class, event and topic name matches Master Context §E and §I.
- [ ] The six SOLID notes (§3.3) are on the right slides and each matches a line of the SOLID map.
- [ ] Every diagram is readable at slide size and has its draw.io source in `docs/diagrams/`.
- [ ] Agenda and slide numbers match the 71-slide order.
- [ ] Raspberry Pi text rewritten; "Timing Parameters" title fixed.
- [ ] The full test catalogue is in the report.
- [ ] The new deck replaces the old one in the Claude Project and as PDF in `docs/design/`.

---

## 9. Effect on other documents (applied 2026-11-09)

These Phase 4 decisions change statements in the drafts that are not yet distributed. Claude proposes these edits and applies them only with approval (Guidelines §5.2):

| Document | Section | Change |
|---|---|---|
| Master Context | §B.8 | "Each needs one justification line in the Design presentation" → "Not shown in the presentation (P4-6); the reasons are kept here for oral questions" |
| Master Context | §G | New record **DEC-013 — Phase 4 presentation decisions** (P4-1…P4-11, Gantt not updated); DEC-004, DEC-006 and DEC-010: "justification line in the deck" → "prepared for oral questions" |
| Master Context | §K.2, §L.5 | Full test catalogue in the report; no annex; 71 slides; walkthrough slide; divergence notes not presented |
| Master Context | §M.1, §M.2 | Remove the Phase 4 and Gantt items; add "edit the deck in authorized batches" |
| Guidelines | §7 | Remove "Every divergence from the Analysis has its justification line" |
| Deck change list (Portuguese) | — | Archived when this plan is distributed |

**Status:** approved by David and applied on 2026-11-09 (Master Context v1.1 with DEC-013; Guidelines v1.1). The Portuguese change list is archived in `docs/archive/`.
