> **ARCHIVED — superseded on 2026-11-09 by `VIGILSYSTEM_MASTER_CONTEXT.md` v1.0. Kept for history only. Do not implement from this document.**
>
> Original location: Claude Project: `VigilSystem — Design Phase Revision Plan.md` · Document date: 4 Nov 2026 · Superseded by: Master Context §G.3, §L; `docs/presentation/DESIGN_DECK_CHANGES.md`.
> The text below this box is an unchanged copy of the original.

---

# VigilSystem — Design Phase Revision Plan

Nov 4, 2026 · @David

## Verdict and how to read this review

The deck is not yet implementation-ready on its own. Its interface tables (slides 15–22) and MQTT/message slides (28–41) are more detailed than any previous group's, but it has one factually wrong slide (45, priorities), several diagrams that contradict the Implementation Guide (46, 48, 49, 51, 52), five of nine threads without any behaviour specification, no driver diagram, no physical-deployment decision and no design-pattern map. Fixing roughly 12 slides and adding about 10 is enough; the architecture itself does not need to change.

**What was reviewed:** all 71 slides of `G8_VigilSystem_D_Presentation.pptx` (every diagram image extracted and read at full resolution), the G9 review (2 pages), the ChatGPT Revision Guide, the Implementation Guide (project doc, 4 Nov 2026), the Hardware Context and Class Diagram project docs, and the G4 (64 slides), G5 (67), G10 (38) and G12 (43) design decks. No source was unreadable.

**Slide numbering warning.** Three numbering schemes are in circulation, so every reference here uses the **current 71-slide deck**:

| Source | Deck it numbers | Mapping to the current deck |
| --- | --- | --- |
| G9 review | older deck (its "slide 47" GUI table is now slide 55) | match by title |
| ChatGPT guide | 68-slide deck | its 1–14 = same; its 15 = now 15+16; its 16 = 17; its 17 = 18+19; its 18–19 = 20–21; a Power slide was added (22); its N ≥ 20 = **N + 3** |
| Implementation Guide | 61-slide PDF of 3 Nov | match by title |

**Labels.** Status: KEEP / CHANGE / ADD / REMOVE / REDESIGN. Priority: CRITICAL (blocks the Design Phase or is wrong) · HIGH (implementers would have to guess) · MEDIUM (consistency or professor-format) · OPTIONAL. Every recommendation says whether it is **\[Guide\]** (already decided in the Implementation Guide), **\[G9\]** (reviewer/professor feedback), **\[Default\]** (a provisional default in Guide §18) or **\[New\]** (my proposal, not yet approved by the team).

**Source priority used:** G9 (professor expectations) → current deck → Implementation Guide → ChatGPT guide → previous groups. Where they disagree, section "Contradictions" says which one I recommend and why.

## What the professor expects (from the G9 review)

Only six G9 points are explicitly attributed to the professor; the rest are the reviewers' own reading. Treat the first table as mandatory and the second as strong advice.

**Explicitly attributed to the professor**

| # | Expectation | G9 wording | Current deck | Gap |
| --- | --- | --- | --- | --- |
| P1 | Physical deployment: breadboard, protoboard or PCB, and where each sensor is mounted | "The professor asks where the physical deployment is made" | Absent (slide 12 is a schematic only) | New slide needed |
| P2 | Design patterns + SOLID map: pattern → class → problem solved | "The professor wants a map" | Absent | New slide needed |
| P3 | Class diagram following the professor's structure and UML rules | "does not follow the structure asked by the professor or the UML rules" | Slide 43: crossing curved lines, mixed relation types | Redraw |
| P4 | Task slide with the role of each task | "The professor asked for a task slide with the role of each task" | Slide 44 is a graph only | New table slide |
| P5 | One flowchart per task, and function flowcharts tied to their thread | "The professor asks for one flowchart per task" | 3 flowcharts for 9 threads (50–52) | 4 new slides |
| P6 | Is SQLite a library or a server? | "The professor asked this question in the design reviews" | Not stated on slides 25–26 | One sentence on slide 25 |

**Reviewers' own recommendations (not attributed to the professor)**

| # | Recommendation | My assessment |
| --- | --- | --- |
| R1 | Problem statement as one sentence ("Design and implement an embedded system that…") | Accept — cheap and standard |
| R2 | Schematic colours must match the legend; check 5 V parts against 3.3 V GPIOs (RGB anode at 5 V) | Accept — a real electrical issue, see slide 12 |
| R3 | Complete **and** simplified class diagram grouped by package | Accept — directly helps P3 |
| R4 | Priority bands need a legend, not colours only | Accept — and slide 45 also has wrong numbers |
| R5 | Flowcharts with one entry and one exit (PIN validation has two starts) | Accept — standard flowchart rule |
| R6 | Driver needs diagrams and a flowchart, not only text | Accept — the driver is the course's mandatory deliverable |
| R7 | "StorageThread is the only one that accesses" SQLite | **Accept with correction** — untrue in the current design (see Contradiction C10) |
| R8 | Slide 26 is a schema, not an ERD | Partly accept — it is a physical ERD; rename it or add a conceptual view |
| R9 | GUI should be a structural draft, not detailed mock-ups; the state/action table is good | Accept, but do not add slides (Guide §2.4 declined extra GUI slides) |
| R10 | Test tables need an "Objective" column; column names are inconsistent | Accept |

The professor's underlying test is the same as yours: **could another team build it without asking?** P1–P6 are the items they will check first.

## Contradictions between sources

Twenty-two contradictions were found; C1–C8 would make two implementers build different systems. "Recommend" says which version to keep and why.

| ID | Where | Conflict | Recommend |
| --- | --- | --- | --- |
| C1 | Slide 45 vs Guide §5.2 / D-05 | Slide shows LocalInput + PowerMonitor at **SCHED\_FIFO 80** and Storage/Network/Supervisor as "**SCHED\_FIFO** default scheduling". Guide: FIFO **50** and **SCHED\_OTHER**. | Guide. The slide is simply wrong; with it, keypad scanning would pre-empt nothing less than the fire path and Storage would run real-time with disk I/O (breaks RT-5). |
| C2 | Slide 48 (and 41 note) vs Guide N-1 | Network FSM starts at a "connected at boot?" diamond. Guide: always **NET\_LOST** at start; only `EVT_NET_RESTORED` (after CONNACK) enters NET\_CONNECTED. | Guide. At `EVT_SYSTEM_STARTUP` no MQTT session can exist yet. |
| C3 | Slides 48, 51 (control loop) and class diagram 43 vs Guide N-4 / issue 4 | `EVT_RETRY_TIMER` drawn as a controller event / FSM self-loop. Guide: internal to NetworkMQTTThread, never queued. | Guide. Backoff is network-thread business; the controller only sees lost/restored. |
| C4 | Slide 46 (DISARMED entry action "DISARMED event") and 52 (No-branch logs DISARMED) vs Guide O-1 / issue 18 | Entry action logs DISARMED at boot and when a valid PIN is typed while already disarmed. | Guide. Move logging onto the transitions; no event when nothing changed. |
| C5 | Slide 20 ("Wrong PIN → Alarm flashing → Normal") vs Guide §6.10, BR-PIN-7, slide 52 | Slide lets the wrong-PIN blink override alarm flashing; Guide drops it. | Guide (decision D4 below). |
| C6 | Slide 49 vs slide 22 and Guide P-2, P-7, P-9 | Power FSM lacks BATTERY\_MODE → SHUTDOWN on critical voltage, boot-on-battery POWER\_LOST alert and battery-level self-loop; slide 22 produces `EVT_BATTERY_CRITICAL` and `EVT_BATTERY_LEVEL` that the FSM never consumes. | Guide. |
| C7 | Slide 51 vs Guide BR-LOOP-1–5 | Exclusive "Event type?" switch to one FSM; no queue drain, heartbeat, 1 s timeout, `EVT_FAULT`, `EVT_KEY_DIGIT`, `EVT_PIN_SAVE_RESULT`. | Guide. A keypad PIN during a fire alarm must reach Fire **and** Operational FSMs. |
| C8 | Slides 15, 16, 19–22, 25 vs class diagram 43 | Events/types used on the interface slides (`EVT_FAULT`, `EVT_BATTERY_LEVEL`, `isSmokePresent()`, `CommandQueue<OutputCmd>`, `IEventSink`) are missing from the class diagram; `OutputCmd`/`StorageCmd`/`NetCmd` are never defined in the deck. | Add them to the class diagram (Guide §5.4–5.5, G14). |
| C9 | Slide 44 vs Guide §5.2 | "Main / Supervisor Thread" merged into one box that "spawns" everything. Guide: `main` starts threads then `sigwait()`; Supervisor only monitors. Also missing: Output/Storage → Event Queue (faults, PIN-save result) and Storage → Network committed-event path. | Guide. |
| C10 | G9 R7 vs slide 25 and Guide §7.8 | G9 asks to write "StorageThread is the only one that accesses SQLite". In the design NetworkMQTTThread calls `getPendingAlerts()`, `markSent()` and `getEventsSince()` itself (slide 25 even draws Network → SQLite). | Keep the design; write the true rule (decision D5). |
| C11 | Slides 38, 41, 48 vs Guide §7.9 / issue 3 | Wi-Fi loss detected by "wpa\_supplicant event". Guide: netlink `LinkMonitor` (`RTMGRP_LINK`, `IFF_RUNNING`). | Guide (decision D6). |
| C12 | Slides 40–41 vs Guide §12.1/§12.5 | DHCP client "udhcpc" on slides; Buildroot config and boot order use **dhcpcd** (`S41dhcpcd`). | Use whichever the image actually contains; the Guide says dhcpcd. |
| C13 | Slide 53 vs Guide §8.3 | Driver error list includes `-EBUSY`; the UAPI contract defines EINVAL, EFAULT, EIO, EINTR only. | Guide (mutex is interruptible → EINTR, never EBUSY). |
| C14 | Slide 16 vs Guide §7.4 | MQ-2 "Fault recovery: retry according to hardware-access policy" — no such policy exists anywhere. | Decide (D12): retry the line request every 10 s, as IR. |
| C15 | Slide 36 vs Guide §10.7 | Status publish (step 20) only inside the "PUBACK received" branch; rule is outbox → alert → status regardless of the PUBACK. | Guide. |
| C16 | Slide 34 vs Guide §6.6 step 6 | PIN\_CHANGED trigger "valid Change PIN command"; Guide logs it only after Storage confirms the save. | Guide. |
| C17 | Slide 66 "Delivered once" vs Guide §10.8 | QoS 1 + sync is at-least-once; the app deduplicates by `event_id`. | Guide wording. |
| C18 | Slide 69 vs Guide §6.8 / §14 | "Database rebuilt" unconditionally; "IR or MQ-2 stuck" detection. Guide: backup restore first, rebuild last; only request/read errors are detected (no stuck-sensor algorithm). | Guide. |
| C19 | Slide 60 "object in front of IR 1" vs slide 15 | Object in front = reflection = LOW = **closed**; the trigger is LOW → HIGH (door opens). The test describes the non-trigger. | Fix test text. |
| C20 | Slide 4 vs slides 13 and 16 | Overview draws two MQ-2 modules with an ellipsis; design has exactly **one** MQ-2 (GPIO21) and two IR sensors. | Draw one MQ-2, two IR. |
| C21 | Class diagram 43 vs Guide RT-6 and §12.4 | Queues use `std::mutex` (Guide: priority-inheritance `PiMutex`); `GpioLines` holds `gpiod_line_request*` (libgpiod v2) but the target has v1.6. | Guide; make the GPIO handle an opaque implementation detail. |
| C22 | Project instructions (Portuguese) vs everything else | PIR, four custom drivers, `CommunicationThread`, red LED on GPIO26, RGB on 4/13/19. | Obsolete (Guide §2.3). Update the project instructions so nobody reads them as current. |

**ChatGPT guide vs Guide §2.4.** ChatGPT recommends rewording the app timeout message ("unknown outcome"). The Guide lists "changing the app command timeout … or the timeout wording" as **declined on purpose**. Reject it, unless you reopen that decision with a new reason.

## Slide-by-slide review

Each slide: status and priority, then what is right, what is wrong or missing, replacement content, why, and sources. Slides with identical verdicts are grouped but every number is named.

### Part A — Introduction and hardware (slides 1–14)

#### Slide 1 — Cover

**KEEP — no changes required.** Optional: add "Design Phase — rev. 2 (dd Nov 2026)" so the professor can tell this from the version G9 reviewed.

#### Slide 2 — Agenda

**CHANGE · MEDIUM.** Update it last, after the new slides exist. Replace "Parameters Times" with "Timing Parameters"; add "Physical Deployment", "Design Patterns & SOLID", "Thread Responsibilities & Event Contracts", "Thread Flowcharts", "Device Driver Design", "Design Decisions & Limitations". Remove the empty line before "Gantt Chart". \[G9 P1–P5\]

#### Slide 3 — Problem Statement

**CHANGE · MEDIUM.** The three bullets are context, not a statement (G9 R1). Keep them as "Motivation" under one statement:

> **Problem statement.** Design and implement a subscription-free embedded home-security system on a Raspberry Pi 4 that detects door/window intrusion when armed and smoke/gas 24/7, sounds distinct local alarms within 1 s, can be armed and disarmed locally (button and PIN keypad) and remotely (Android app over MQTT 5/TLS), and keeps working through mains and Internet outages (UPS battery, local SQLite outbox).

Every number in it is already a design target (slides 54, 68). \[G9 R1; Guide §1.1\]

#### Slide 4 — System Overview

**CHANGE · MEDIUM.** Correct as a component picture, but it contradicts the design (C20): it shows **two** smoke modules with an ellipsis (one MQ-2 exists) and an ellipsis between IR modules (exactly two exist). Draw 1 MQ-2 and 2 IR sensors with no ellipses. Rename "Buzzer" → "Speaker", "Number Pad" → "Keypad 3×4", "Cloud" → "EMQX broker (MQTT 5/TLS)". Optional: one caption "Local alarm works without the network." \[Guide §4.1; C20\]

#### Slide 5 — Hardware Architecture

**CHANGE · MEDIUM.** Block structure is fine. Labels contain typos and are not tied to blocks: "libgpiod&", "libgpiodd", "GPIO,libgpiod". Use one label per arrow:

| Arrow | Label |
| --- | --- |
| Input (IR ×2, MQ-2) → Pi | GPIO 16/20/21 · libgpiod (MQ-2 via 5 V→3.3 V divider) |
| User interface → Pi | GPIO (keypad 7 lines, button GPIO6) · libgpiod |
| Pi → Output | Speaker: PWM0/GPIO18 · red LED: PWM1/GPIO13 via **/dev/vigil\_pwm (custom driver)**; RGB: GPIO 4/26/19 · libgpiod |
| Power & battery ↔ Pi | 5 V via header + I²C-1 (INA219 @ 0x42) · i2c-dev |
| Pi ↔ Communication | Wi-Fi 802.11n/ac · MQTT 5 over TLS :8883 |

Reason: this slide is where an implementer first sees which Linux interface owns which device; today only the custom driver is named. \[Guide §4.2, D-09\]

#### Slide 6 — Hardware Specification: Raspberry Pi 4

**CHANGE · OPTIONAL.** Datasheet bullets are adequate. Add two facts the software depends on: "GPIO 3.3 V, **not 5 V tolerant**, 8 mA default drive" and "PWM block shared with analogue audio — audio disabled". Drop "display port" and "PoE" (unused). \[Guide §4.1, §4.3\]

#### Slides 7–8 — UPS HAT, adapter, batteries

**KEEP — no changes required.** Ratings are correct and consistent with slide 14. Mains detection and thresholds belong to slide 22, which already has them. Optional on slide 7: "No mains-present signal: mains loss is inferred from INA219 current (slide 22)." \[Hardware Context §4\]

#### Slide 9 — MQ-2 and IR sensor modules

**CHANGE · HIGH.** MQ-2 bullets are fine. The IR bullets omit the one fact that decides the circuit: the **output stage**. "Control output 100 mA" suggests NPN open-collector, but the Hardware Context still lists it as "to verify".

- Add to IR: "Output: NPN open-collector (to be confirmed by measurement) → 3.3 V pull-up on GPIO. If the output is 5 V push-pull, a 10 kΩ/20 kΩ divider is used as for the MQ-2." (decision D3)
- Add: "Mounted on the frame, aimed at the door/window leaf: closed = reflection = LOW; open = HIGH."
- Add to MQ-2: "Educational sensor, not a certified smoke detector; reacts to alcohol vapour."

Reason: without the output type the builder cannot know whether the slide-12 pull-up is safe for a 3.3 V pin. \[Hardware Context §7, §17; Guide §4.3\]

#### Slide 10 — Keypad and speaker

**CHANGE · MEDIUM.** Rename "Buzzer Module (Speaker)" → "Speaker FoneStar EP-36, 8 Ω / 0.3 W, with NPN driver" (the part is a speaker; "buzzer" implies a self-oscillating device that cannot play tones). Add the driver line: "2N2222A/BC337, 470 Ω base, 10 kΩ base pull-down, 33 Ω/0.5 W series, 1N4148 flyback; peak ≈ 122 mA; duty ≤ 50 %." Keypad text: add "columns open-drain (only one driven LOW at a time)". \[Guide §4.1, §4.3\]

#### Slide 11 — RGB LED, red LED, push-button

**CHANGE · HIGH** (follows the slide-12 decision). The RGB block says "8 mA per channel" but not how it is switched. If decision D2 (NPN low-side switches) is adopted, add "Each cathode switched by an NPN (BC337/2N2222A, 1 kΩ base) — GPIO logic HIGH = ON". If D2 is rejected, add "Anode +5 V, cathodes sunk by GPIO through 390/220/220 Ω; GPIO HIGH (3.3 V) = OFF — red channel must be verified fully off". Push-button: add "Ref. 716 on breadboard, Ref. 1188 Ø12 mm on the enclosure panel". \[G9 R2; Hardware Context §6, §12\]

#### Slide 12 — Hardware Connections (schematic)

**REDESIGN · CRITICAL.** The circuit content is good (divider, speaker driver, keypad matrix). G9 found the legend unreliable, and I confirm it after reading the full-resolution image:

| Wire | Drawn colour | Legend meaning of that colour | Problem |
| --- | --- | --- | --- |
| IR1/IR2 OUT → GPIO16/20 | blue | I²C SDA | wrong colour |
| Red LED → GPIO13; RGB R → GPIO4 | red | 5 V | wrong colour |
| RGB G → GPIO26, RGB B → GPIO19 | green / blue | not in legend / SDA | missing / wrong |
| Arm button → GPIO6 | teal | not in legend | missing |
| MQ-2 DO → GPIO21 | grey | not in legend | missing |

Fix: give **signal type** colours, not component colours: red = 5 V, black = GND, orange = 3.3 V, blue = I²C, purple = PWM, green = GPIO input, brown = GPIO output. Then every legend entry is a function, and the builder can trust it.

Electrical issue (G9 R2): the RGB anode sits on **+5 V** while the cathodes are sunk by 3.3 V GPIOs. With the pin HIGH, 5 − 3.3 = 1.7 V remains across the red die (V\_F min 1.8 V), so it may glow; when the pin is an input at boot (GPIO19/26 pulled down) the LEDs light and the pin is held at 5 V − V\_F, close to the 3.3 V + 0.3 V absolute limit for the red channel. **Decision D2 \[New\]:** switch each cathode with an NPN low-side transistor (BC337 or 2N2222A, already in the BOM; 1 kΩ base, 10 kΩ base pull-down), so no 5 V node touches a GPIO and logic becomes active-high. G10 slides 10–11 use exactly this (BC547 per channel). Then update slides 11, 19 and the Guide §4.3 ("active-low flag" removed).

Also add: IR OUT showing "open-collector → 10 kΩ to 3.3 V" (or the divider if D3 finds push-pull), and a small "Boot-safe levels" note (config.txt `gpio=` lines from Guide §12.2). \[G9 R2; Guide §4.3, §12.2\]

#### Slide 13 — GPIO Allocation

**CHANGE · HIGH.** Pins are correct and match slide 12. The table lacks the columns implementers need. Replace it with:

| BCM | Pin | Signal | Dir / bias | Active level | Owner thread |
| --- | --- | --- | --- | --- | --- |
| 17, 27, 22, 23 | 11, 13, 15, 16 | Keypad rows 1–4 | In, pull-up | LOW = key in scanned column | LocalInput |
| 24, 25, 5 | 18, 22, 29 | Keypad cols 1–3 | Out, open-drain | driven LOW when scanned | LocalInput |
| 6 | 31 | Arm button | In, pull-up | LOW = pressed | LocalInput |
| 16, 20 | 36, 38 | IR1, IR2 | In, pull-up | HIGH = door open | IRSensors |
| 21 | 40 | MQ-2 DO (divider) | In, **bias disabled** | LOW = smoke/gas | MQ2Sensor |
| 13 | 33 | Red LED (PWM1, ALT0) | PWM | HIGH = on | Output (/dev/vigil\_pwm) |
| 18 | 12 | Speaker (PWM0, ALT5) | PWM | idle LOW | Output (/dev/vigil\_pwm) |
| 4, 26, 19 | 7, 37, 35 | RGB R, G, B | Out | HIGH = on with D2 (LOW = on without) | Output |
| 2, 3 | 3, 5 | I²C-1 SDA/SCL → INA219 0x42 | I²C | — | PowerMonitor |

Footnote: "Bias for GPIO21 must be disabled: the internal ≈ 50 kΩ pull-down would lower the HIGH level to ≈ 2.1 V." \[Guide §4.2–4.3\]

#### Slide 14 — Power Budget

**CHANGE · OPTIONAL.** Numbers are consistent (0.77 A × 5 V ≈ 3.9 W; 16 Wh / 3.9 W ≈ 4 h). Write "theoretical" next to the autonomy and "acceptance target ≥ 3 h (test slide 64)". If D2 is adopted the RGB current moves off the GPIOs; the total does not change.

### Part B — Hardware interface contracts (slides 15–22)

These eight tables are the strongest part of the deck; no previous group specified interfaces this precisely (G4, G5 and G12 only list pins). Most of them need only the missing rows below. Add the same three rows to every table for consistency: **GPIO / bias**, **Activation (period or blocking wait + timeout)**, **Heartbeat timeout**.

#### Slide 15 — IR Sensor Module Interface

**CHANGE · HIGH.** Correct: thread, levels, rising-edge trigger, 200 ms retrigger guard, no event at initialisation, filtering by the controller, fault event, 10 s retry. Missing rows:

| Parameter | Behaviour |
| --- | --- |
| GPIO | 16 (IR1), 20 (IR2); input, pull-up; one libgpiod request, both edges |
| Activation | Blocking edge wait, 1 s timeout (heartbeat each pass) |
| Retrigger guard | Same sensor within 200 ms → ignored (rename row "Debounce" to this) |
| Falling edge (door closes) | Updates `lastState`; no event |
| Payload | `sourceId` = 1 or 2 → `INTRUSION_ALARM` details `IR1`/`IR2` |
| Fault detection | Line request or read error only (no stuck-sensor detection) → `EVT_FAULT(IR1/IR2)` once per boot |
| Known limitation | Door already open when the system arms: no new rising edge, so no detection until it closes and reopens \[Default G9, decision D10\] |

Also rename the event in prose: it is a **door/window-opened** event; `EVT_MOTION_DETECTED` can keep its name (it is in the class diagram) but slide 60 and the GUI should say "door/window opened". \[Guide §7.3\]

#### Slide 16 — "IR and MQ-2 Sensor Module Interface"

**CHANGE · HIGH.** Title is wrong (slide covers MQ-2 only): rename "MQ-2 Sensor Module Interface". The 5-sample confirmation, 100 ms period and atomic `isSmokePresent()` are good. Missing or wrong:

| Parameter | Behaviour |
| --- | --- |
| GPIO | 21, input, **bias disabled**, active-low (LOW = smoke) |
| Warm-up | First 60 s after start: no event, `isSmokePresent()` = false, fire detection inactive \[Default — value to be measured\] |
| Confirmation latency | 5 × 100 ms = 0.5 s worst case + one period |
| Event | One `EVT_SMOKE_DETECTED` per clear → present transition after warm-up |
| Fault recovery | **Replace** "retry according to hardware-access policy" with: "`EVT_FAULT(MQ2)` once; re-request the line every 10 s; resume monitoring when it succeeds" \[New — decision D12, mirrors IR\] |
| Consumer of the level | Fire FSM guard at the silence timeout (F-7/F-8) |

Reason: "hardware-access policy" is defined nowhere (C14), so two implementers would recover differently from the one sensor that protects life. \[Guide §7.4, G2\]

#### Slide 17 — Keypad Interface

**CHANGE · MEDIUM.** Very complete. Add:

| Parameter | Specification |
| --- | --- |
| GPIO | Rows 17/27/22/23 in, pull-up; columns 24/25/5 open-drain, one driven LOW at a time, ≈ 20 µs settle before reading |
| Key map \[row\]\[col\] | `1 2 3 / 4 5 6 / 7 8 9 / * 0 #` |
| Re-acceptance | A key is reported again only after all keys were released for 2 scans |
| 9th digit (buffer full) | Not stored, no `EVT_KEY_DIGIT`; ERROR\_BEEP \[New — decision D8\] |
| During lockout | Digits still buffered and beeped; `#` → controller replies with ERROR\_BEEP, nothing hashed \[Guide BR-PIN-4\] |
| Feedback owner | KEY\_BEEP is commanded by the controller on `EVT_KEY_DIGIT` (dropped while an alarm sounds) |
| Status of the 10 s timeout, `#`/`*` convention | Mark as \[Default G3\] or approve them |

&#91;Guide §6.6, §7.2\]

#### Slide 18 — Push-Button Interface

**CHANGE · HIGH.** Debounce and one-event-per-press are specified. What the button does in each state is not (G12). Add a state table:

| System state | Button press result |
| --- | --- |
| DISARMED, fire NORMAL or SILENCED | KEY\_BEEP, start 60 s exit delay, red LED on |
| DISARMED, fire FIRE\_ALARM | Rejected, no beep (alarm sounding) |
| EXIT\_DELAY | Ignored + ERROR\_BEEP (does **not** restart or cancel the delay) |
| ARMED, ENTRY\_DELAY | Ignored + ERROR\_BEEP (beep plays over the entry warning) |
| INTRUSION\_ALARM | Ignored, no beep |

This is \[Default G12\]; the "does not restart" line is \[New\]. Also: "No audible exit-delay tone" \[Default G11\]. \[Guide O-2, O-14\]

#### Slide 19 — RGB LED Interface

**CHANGE · MEDIUM.** Colour table is clear. Change "Output type: Digital GPIO (active-low)" according to D2 (active-high with NPN switches). Add:

| Parameter | Specification |
| --- | --- |
| GPIO | R = 4, G = 26, B = 19 |
| Network input | Broker connection (NET\_CONNECTED/NET\_LOST), not Wi-Fi alone |
| Initial colour | Set by `EVT_SYSTEM_STARTUP`: Network starts NET\_LOST, so yellow (mains) or magenta (battery) until the first CONNACK |
| Fault blink end | Latched until reboot (faults are reported once per boot) \[New — decision D11\] |
| Blink timing owner | OutputThread toggles every 500 ms using its `popFor` timeout |

Replace "OFF until initial power/network status is applied" with the initial-colour row. \[Guide §4.4, §6.10, §7.7\]

#### Slide 20 — Red LED Interface

**CHANGE · CRITICAL** (contradiction C5). The slide makes the wrong-PIN blink override alarm flashing; the Guide, the PIN flowchart and `OutputThread` rules drop it. Replace the "Output priority" footer and the two "Wrong PIN indication" rows with:

| Parameter | Specification |
| --- | --- |
| GPIO / channel | GPIO13, PWM1, `/dev/vigil_pwm` channel LED |
| Base modes | OFF (disarmed) · ON (exit delay, armed, entry delay) · FLASH 250/250 ms (any fire or intrusion alarm) |
| Wrong PIN | One-shot BLINK 3 × 100/100 ms over the base mode, then the base resumes (done in the driver) |
| Wrong PIN during FLASH | **Dropped**: the continuing alarm is the feedback that the PIN failed \[Guide BR-PIN-7, decision D4\] |
| Repeated wrong PIN during a blink | Restarts the one-shot |
| Driver failure | `EVT_FAULT(PWM_DRIVER)`; RGB fault blink is the fallback indicator |

Remove "Custom linux PWM driver" wording and write "`vigil_pwm` (our platform driver on pwm-bcm2835)". \[Guide §4.4, §6.10, §8.3\]

#### Slide 21 — Speaker Interface

**CHANGE · MEDIUM.** Tone plan and priority are correct and match Guide §4.4/§8.4. Add:

| Parameter | Specification |
| --- | --- |
| GPIO / channel | GPIO18, PWM0, `/dev/vigil_pwm` channel SPEAKER; idle level LOW |
| Beeps during FIRE or INTRUSION | Dropped (not queued) |
| Exit delay | No tone \[Default G11\] |
| Who steps the pattern | Kernel hrtimer in the driver; OutputThread only selects the pattern (one `write()`, returns at once) |
| Fault split | `EVT_FAULT(SPEAKER)` = I/O error on the speaker channel; `EVT_FAULT(PWM_DRIVER)` = device cannot be opened |

Fix the duty row: "Maximum 50 % (enforced by the driver: speaker duty 1–50 %, else EINVAL)". \[Guide §4.3, §7.7, §8.3\]

#### Slide 22 — UPS / Power Monitor Interface

**CHANGE · MEDIUM.** Thresholds, hysteresis and events are complete. Add:

| Parameter | Specification |
| --- | --- |
| INA219 set-up | Calibration 0x05 = 4096, config 0x00 = 0x3EEF; current LSB 0.1 mA; bus V = (reg 0x02 ≫ 3) × 4 mV |
| Current sign | Negative = discharging (to verify on the board) |
| Boot state | Median of 3 quick samples in `main` before threads start; > −100 mA → mains |
| Battery % | `clamp((V − 6.0) / 2.4 × 100, 0, 100)`, 10 % steps |
| Low battery | ≤ 30 % for 3 samples, on battery, **once per outage** (re-armed when mains returns) |
| Critical | ≤ 6.4 V for 3 samples, on battery, from BATTERY\_MODE or LOW\_BATTERY |
| Event payloads | POWER\_LOST = battery %; BATTERY\_LOW = %; BATTERY\_CRITICAL = voltage; BATTERY\_LEVEL = % |
| I²C failure | Keep the row; decide D13 (syslog only, or a new `FaultComponent` UPS) |

&#91;Guide §4.5, §7.5\]

### Part C — Software, data and communication (slides 23–41)

#### Slide 23 — Software Tools

**CHANGE · OPTIONAL.** Seven logos without a word of purpose. Add one caption per logo: Buildroot 2026.08 (image), VS Code (IDE), Figma (GUI prototype — declare AI use if Figma Make was used, Guide §18.3), PlantUML (UML), Git/GitHub (versioning), draw.io (diagrams). Add the missing build tools: GCC 14 (Bootlin aarch64 stable) + CMake, C++17. G10 slides 13–15 go further and show the Buildroot configuration screens; one line "Base raspberrypi4\_64\_defconfig + vigil fragment (Paho, SQLite, OpenSSL, libgpiod v1, chrony, wpa\_supplicant)" is enough here.

#### Slide 24 — Software COTS

**CHANGE · MEDIUM.** Logos only; **nlohmann-json** is missing although every payload depends on it. Replace the logos with a table so implementers cannot pick an incompatible library:

| COTS | Version / API | Used by |
| --- | --- | --- |
| libgpiod | **v1.6.x** C API (v2 needs kernel headers ≥ 5.10) | LocalInput, IRSensors, MQ2Sensor, Output |
| i2c-dev | kernel char device `/dev/i2c-1` | PowerMonitor |
| Eclipse Paho MQTT C++ (on Paho C) | MQTT 5, `async_client` | NetworkMQTT |
| OpenSSL | TLS 1.2/1.3; PBKDF2-HMAC-SHA256 | NetworkMQTT; PIN hashing |
| SQLite 3 | C API, linked in-process | Storage |
| nlohmann-json | header-only | Protocol |
| pthreads / epoll / eventfd / timerfd | glibc + kernel | all threads |
| EMQX Serverless | MQTT 5 broker | cloud |
| Flutter + mqtt5\_client | Android app | app |

&#91;Guide §12.1, §12.4, D-48, D-49\]

#### Slide 25 — Data Storage Specification

**CHANGE · HIGH.** The alert contract ①–⑥ is excellent. Two professor-level gaps:

1. **Library vs server (G9 P6).** Add the sentence: "SQLite is an embedded **library** linked into the `vigil` process (no server, no socket). One `Database` object, one connection, opened `SQLITE_OPEN_FULLMUTEX` and guarded by a priority-inheritance mutex."
2. **Who accesses it (G9 R7 / C10).** Do not write "StorageThread is the only one" — the diagram itself draws NetworkMQTTThread → SQLite. Write the true rule (decision D5): "`Database` is owned by StorageThread, which performs **all inserts** (events, outbox rows, PIN). NetworkMQTTThread calls three mutex-protected methods of StorageThread: `getPendingAlerts()`, `markSent()`, `getEventsSince()`. No other thread touches SQLite. The controller never does."

Also add the degraded-mode line: "Write failure → `EVT_FAULT(DATABASE)`; event ids continue in RAM; alerts kept in a RAM outbox (lost on reboot); reopen every 60 s." \[Guide §7.8\]

#### Slide 26 — Database Entity Relationship Diagram

**CHANGE · MEDIUM.** G9 R8 is half right: the slide is a **physical** ERD (it does show the 1 : 0..1 "generates" relationship), but it reads as a schema dump. Two options; I recommend (a):

- (a) Rename it "Database — Logical ERD and Physical Schema" and add, at the top, a 3-box conceptual line: `EVENT_LOG (id, event_type, timestamp, details)` — 1 *generates* 0..1 — `PENDING_NOTIFICATION (id, event_id, status, sent_at)`; `PIN_CONFIG (id, pin_hash)` with "no relationship". Move the CHECK lists and PRAGMAs to the annex.
- (b) Keep this slide and add a separate conceptual ERD slide (costs a slide for little information).

Add the cardinality in words under the line: "each alert event has exactly one outbox row; log events have none; PIN\_CONFIG holds exactly one row". \[Guide §9.2\]

#### Slide 27 — Cloud Architecture

**KEEP — no changes required.** Broker-only, ports, NTP/DNS and the hop-by-hop TLS note are all stated. Optional caption: "No backend, no cloud database, no push service (D-10)."

#### Slide 28 — Message Specification — Remote Commands

**CHANGE · MEDIUM.** Check orders are correct and match Guide §10.4. Add to the Change PIN note: "5. A save already pending → SAVE\_FAILED (detail `BUSY`) \[Default G13\]. 6. OK only after the new hash is committed; the RAM hash changes only then." Add to every command box: "Message Expiry 30 s (required; commands without it are ignored)" and "`request_id`: string 1–64 chars, `<client_id>-<counter>`". Define the `detail` values: LOCKED\_OUT = remaining minutes rounded up; INVALID\_NEW\_PIN = "4-6 digits"; otherwise absent \[Default G5\]. Add one line "Malformed JSON, unknown `type`, missing `request_id`, retained `cmd` → ignored, syslog, **no reply**" \[Default G7\]. \[Guide §10.4, §10.6\]

#### Slide 29 — Status & Connections

**KEEP — minor addition.** Complete. Add one line to the status note: "EXIT\_DELAY is reported as DISARMED and ENTRY\_DELAY as ARMED" (the note says it indirectly). \[Guide §6.11\]

#### Slides 30–32 — Individual Real-Time Alerts

**CHANGE · MEDIUM.** Message shape and PUBACK handling are complete. Several `details : String` fields have no defined values. Replace with enumerations:

| Alert | details |
| --- | --- |
| INTRUSION\_ALARM (30) | `"IR1"` \| `"IR2"` — the sensor that started the entry delay \[Default G4\] |
| POWER\_LOST (31) | battery % as a string, e.g. `"80"` \[Default G4\] |
| POWER\_RESTORED (31) | `null` \| `"BOOT"` (boot on mains after SHUTDOWN/POWER\_LOST) |
| LOW\_BATTERY (31) | battery %, e.g. `"30"` (`battery_pct` is a variable name, not a value) |

On slide 31 Power Lost: add "or boot on battery when the last power event was not POWER\_LOST" (Guide P-2). On slide 32 Device Fault: change trigger to "first line-request/read/write error of the component this boot". \[Guide §10.4\]

#### Slides 33–34 — Individual Log Events

**CHANGE · MEDIUM.** Slide 33 Armed trigger: write "Exit-delay **timeout** (local) or Remote Arm — not the button press". Slide 33 Disarmed: add "Not logged when the system was already disarmed" (C4). Slide 34 PIN Changed: trigger "after StorageThread confirms the new hash is saved" (C16). Add once: "If the Pi is not connected, log events are not published at all (recovered by get\_events)". \[Guide §6.12, §7.9\]

#### Slide 35 — Broker Connection

**KEEP — no changes required.** CONNACK handling and both clients' parameters are fully specified. Optional: "TLS: CA file + host-name verification; never fall back to plain MQTT" (it is on slide 69 already).

#### Slide 36 — MQTT Connection and Alerts sequence

**CHANGE · MEDIUM.** Move step 20 ("PUBLISH updated status") **out of** the `alt [PUBACK received]` frame so it follows step 17 in both branches (C15). Rename step 15 "smoke detected → local alarm (independent of network)". Optional: split the Pi lifeline into AlarmController / Storage / NetworkMQTT so the outbox order (insert → commit → publish) is visible — or leave that to the new internal sequence on the event-contract slide (N5). \[Guide §10.7\]

#### Slide 37 — Remote Command sequence

**CHANGE · OPTIONAL.** Correct and matches slide 28. The footer already lists LOCKED\_OUT for Change PIN (ChatGPT's item is done); Arm correctly has none, since it needs no PIN. Add a second small diagram or note for **Change PIN** (asynchronous save) only if the new Change PIN flowchart (N7) is not created.

#### Slide 38 — Connection Loss and Recovery

**CHANGE · MEDIUM.** Replace "Wi-Fi loss ≤ 2 s (wpa\_supplicant event)" with "(netlink link monitor, `IFF_RUNNING` on wlan0)" (C11, decision D6). Add after step 07: "in-flight map cleared; unacknowledged alerts stay PENDING". Step 12: the app then publishes `get_events` (already in App sync; fine). \[Guide §7.9\]

#### Slide 39 — Low Battery Shutdown

**CHANGE · OPTIONAL.** Correct. Add the coordination timeouts the Guide fixes: shutdown waits ≤ 5 s for the SHUTDOWN PUBACK, the controller waits ≤ 10 s for Network, ≤ 3 s for the DB checkpoint; the watchdog is disabled (magic close) before power-off. Mark step 21 "if the Pi restarts by itself when mains returns — to verify on hardware (Guide issue 7)". \[Guide §6.9\]

#### Slide 40 — Communication Stack

**CHANGE · OPTIONAL.** Good. Fix "udhcpc" → the DHCP client actually in the image (dhcpcd per Guide §12.5; C12). Add on the Pi application box: "Paho callbacks only post to NetworkMQTTThread's queue; only that thread touches MQTT state".

#### Slide 41 — Wi-Fi Specification

**CHANGE · MEDIUM.** Same two fixes: link loss "netlink link monitor ≤ 2 s" and the DHCP client name. Replace the yellow note "not connected at boot → NET\_LOST" with "The application always starts in NET\_LOST and never waits for Wi-Fi or NTP" (C2). Add the saved-date script (no RTC) as step 0 so TLS can succeed before NTP. \[Guide §12.5\]

### Part D — Software architecture and behaviour (slides 42–53)

This is where the professor's explicit requests (P2–P5) land and where most contradictions sit.

#### Slide 42 — Package diagram

**CHANGE · MEDIUM.** It is closer to a component diagram than a UML package diagram, which is acceptable if it is consistent. Fixes: (1) only "GPIO input threads" and PowerMonitor have links to driver interfaces — the libgpiod arrow from the input-threads package is drawn from the package frame, so it is unclear which threads use it; label it "LocalInput, IRSensors, MQ2Sensor"; (2) draw Storage → Network as **"committed event (IEventSink)"** one way and Network → Storage as "outbox read, markSent, get\_events" (today one double arrow); (3) add Output/Storage → Event Queue "faults, PIN-save result"; (4) put packages in the same names you will use for the simplified class diagram (N3): `core` (Event, queues), `threads`, `logic` (AlarmLogic, PinHasher, OutputPolicy), `hw` (GpioLines, Ina219, PwmDevice, Watchdog), `storage`, `net`. Then P3's "grouped by package" request is met by the same names on both slides. \[Guide §3.2, §5.1\]

#### Slide 43 — Class Diagram

**REDESIGN · CRITICAL** (P3). G9 is right that it cannot be read: curved lines cross over classes and relation types are hard to tell apart. Content gaps (C8, C21):

| Add / change | Detail |
| --- | --- |
| EventType | add `EVT_BATTERY_LEVEL`, `EVT_FAULT`, `EVT_PIN_SAVE_RESULT`; **remove** `EVT_RETRY_TIMER` from "raised inside AlarmControllerThread" |
| Event | add `+monoNs: uint64_t` |
| CommandQueue\<T> | add `+popFor(timeout): optional<T>`, `+close()` |
| Command types | add `OutputCmd`, `StorageCmd`, `NetCmd` («struct» with their `Kind` enums) and `SpeakerPattern`, `RedLedMode`, `RgbColour`, `FaultComponent` enums |
| MQ2SensorThread | add `-smokePresent: atomic<bool>`, `+isSmokePresent(): bool` |
| AlarmControllerThread | add `-pendingPinChange`, `-activeFaults`, `-entrySensor`, `-batteryPct`; association to `AlarmLogic` |
| New classes | `IEventSink` «interface» (StorageThread → NetworkMQTTThread), `AlarmLogic`, `PinHasher`, `Watchdog`, `LinkMonitor`, `Protocol`, `PiMutex` |
| Interfaces (for SOLID I/D claim) | `IGpioInput`, `IGpioOutput`, `IPwm`, `IPowerSensor` realised by `GpioLines`, `PwmDevice`, `Ina219` (Guide §5.7 claims them; the diagram must show them) |
| Mutex type | `PiMutex` instead of `std::mutex` in queues (RT-6) |
| GpioLines | replace `gpiod_line_request*` with an opaque handle (target is libgpiod v1) |
| StorageThread | replace `saveEvent` + `insertPendingAlert` with `logEvent(cmd)` = one transaction (Guide §7.8) |

Layout rules that solve G9's complaint: orthogonal connectors only; generalisation arrows up to `WorkerThread`; compositions down to the hw classes; dependencies (dashed) left to queues; one package frame per layer. Use **two slides**: N3 (simplified, by package, only main classes and relationships) placed before this one, and this full diagram as "Class Diagram — complete" (may be split by package across two slides, or moved to the annex with a readable PDF). \[G9 P3, R3; Guide §5.4–5.5, §7.10, G14\]

#### Slide 44 — Task overview

**CHANGE · HIGH** (C9). Split "Main / Supervisor Thread": `main` creates queues, opens devices, starts threads in order, then waits in `sigwait()`; `SupervisorThread` only checks heartbeats, restarts a hung worker and feeds `/dev/watchdog`. Replace the "spawns and monitors" fan with a single "heartbeat" dashed group arrow. Add the missing flows: Output → EQ `EVT_FAULT`; Storage → EQ `EVT_FAULT`, `EVT_PIN_SAVE_RESULT`; Storage → Network "committed event (event\_id)"; Network → Storage "outbox read / markSent / get\_events". Rename labels: "CONNECTION\_RESTORED / Commands" → `EVT_NET_RESTORED/DISCONNECTED, EVT_REMOTE_*`; "POWER\_LOST / RESTORED" → `EVT_POWER_*, EVT_BATTERY_*`; "Buzzer & LEDs" → "Speaker & LEDs". Then add the role table on the new slide N4 right after it (P4).

#### Slide 45 — Task priority

**CHANGE · CRITICAL** (C1). Two labels are wrong: third band must read **SCHED\_FIFO · priority 50**, fourth band **SCHED\_OTHER (nice 0)**. Add the legend G9 R4 asked for: an arrow "higher priority ↑", and under each band one line of justification:

| Band | Threads | Why |
| --- | --- | --- |
| SCHED\_FIFO 80 | MQ2Sensor, AlarmController, Output | Fire path: the alarm is only as fast as its slowest stage |
| SCHED\_FIFO 70 | IRSensors | Intrusion has a 30 s entry delay anyway |
| SCHED\_FIFO 50 | LocalInput, PowerMonitor | Human-rate and 1 Hz events |
| SCHED\_OTHER | Storage, NetworkMQTT, Supervisor | Disk/network I/O may block; a starved Supervisor stops feeding the watchdog → reset |

Add footnote: "Shared queues/DB use priority-inheritance mutexes; RT throttling left at 95 %." \[Guide §5.2, D-05, RT-6, RT-9\]

#### Slide 46 — Operational FSM

**CHANGE · HIGH** (C4). Structure and guards are right, and the "\[Remote Disarm only: …\]" guard placement (pending in the Guide) is already fixed. Changes:

1. DISARMED entry action: remove "DISARMED event". Put `/ DISARMED(LOCAL|REMOTE) event` on the four incoming transitions, and `/ ALARM_SILENCED(INTRUSION) event` additionally on the INTRUSION\_ALARM → DISARMED keypad transition.
2. ARMED entry "ARMED event" is ambiguous (LOCAL vs REMOTE): put `/ ARMED(LOCAL)` on the exit-timeout arrow and `/ ARMED(REMOTE)` on both remote-arm arrows.
3. Add the self-loops that are decisions, not omissions: EXIT\_DELAY `EVT_EXIT_DELAY_TIMEOUT [fire_state = FIRE_ALARM] / stay` (O-13); ARMED/ENTRY\_DELAY/INTRUSION\_ALARM `EVT_REMOTE_ARM / reply ALREADY_ARMED`; DISARMED `EVT_REMOTE_DISARM / reply ALREADY_DISARMED`; button in other states per slide 18 table (O-14).
4. Note: add "`EVT_FAULT` and network/power events do not change this FSM".

#### Slide 47 — Fire FSM

**CHANGE · MEDIUM.** Content is right. Two arrows between FIRE\_ALARM and SILENCED have their labels stacked in the middle, so it is unclear which label belongs to which arrow — separate them vertically. Add the FIRE\_ALARM self-loop `EVT_SMOKE_DETECTED / ignored` (F-3) and `EVT_REMOTE_ARM / reply REJECTED_FIRE_ACTIVE`. Under NORMAL: "no smoke events during the 60 s MQ-2 warm-up". On the F-5 arrow add "(keypad only; not during PIN lockout)". \[Guide F-1…F-8\]

#### Slide 48 — Network FSM

**REDESIGN · HIGH** (C2, C3, C11). Remove the "connected at boot?" diamond: `EVT_SYSTEM_STARTUP → NET_LOST`. Remove the `EVT_RETRY_TIMER` self-loop and put the backoff rule in a note "handled inside NetworkMQTTThread; never reaches the controller". Change NET\_CONNECTED entry to "RGB normal colour" only and add a note: "`online`, status and outbox resend are done by NetworkMQTTThread **before** it raises EVT\_NET\_RESTORED". This makes the diagram an AlarmController FSM (its owner) rather than a mix of two threads. Fix the note: "Wi-Fi loss ≤ 2 s (netlink link monitor)".

#### Slide 49 — Power management FSM

**CHANGE · HIGH** (C6). Add:

- BATTERY\_MODE → SHUTDOWN: `EVT_BATTERY_CRITICAL [≤ 6.4 V]` (P-7, voltage sag under load).
- Boot branch "no" (battery): action `/ POWER_LOST alert unless last power event = POWER_LOST` (P-2).
- Boot branch "yes" (mains): action `/ POWER_RESTORED(BOOT) alert if last power event ∈ {SHUTDOWN, POWER_LOST}` (P-1) — now only in the note.
- Self-loop on every state `EVT_BATTERY_LEVEL / update battery_pct, publish status` (P-9).
- AC\_POWERED self-loop `EVT_BATTERY_LOW, EVT_BATTERY_CRITICAL / ignored` (P-8).
- SHUTDOWN entry: keep, add "wait ≤ 5 s PUBACK, ≤ 10 s network, ≤ 3 s DB; disable watchdog" (or reference slide 39).

#### Slide 50 — Flowchart — StartUp

**CHANGE · HIGH** (C12). Replace with this sequence (single entry, single exit):

1. Read `broker.conf` and `vigil.conf` (defaults if missing); `openlog`; `mlockall`; block signals.
2. Create Event Queue and 3 Command Queues.
3. Open `/dev/watchdog` (15 s); main feeds it until Supervisor runs.
4. Open `/dev/vigil_pwm`, request GPIO lines → **failure? → queue `EVT_FAULT(...)`, continue degraded** (never abort).
5. Database decision (keep your three-way diamond) **plus** a fourth branch "no DB file (first boot) → create DB, store default PIN hash, no FAULT"; the default-PIN branch must also "rename bad file to vigil.db.corrupt-\<time>, create new DB"; the OK branch adds "write backup".
6. Read INA219 (3 samples) → `mainsAtBoot`, `batteryPct`; read last power event from EVENT\_LOG.
7. Start threads in order: Storage, Output, AlarmController, NetworkMQTT, IRSensors, MQ2Sensor, LocalInput, PowerMonitor, Supervisor.
8. Controller handles `EVT_SYSTEM_STARTUP`: DISARMED, fire NORMAL, power per step 6, NET\_LOST; LED off, speaker off, RGB set.
9. Draw MQ-2 warm-up as a **note** "runs in parallel inside MQ2SensorThread", not as a step (issue 14).
10. End: "main waits in sigwait()".

&#91;Guide §6.8\]

#### Slide 51 — Flowchart — Control loop

**REDESIGN · CRITICAL** (C7). Replace the exclusive "Event type?" switch with:

1. `epoll_wait(eventfd + 4 timerfds, timeout 1 s)` → `heartbeat()`.
2. Timeout? → back to 1.
3. Timerfd ready? → read expiry count → build `EVT_*_TIMEOUT`.
4. Eventfd ready? → read it once → **loop: `tryPop()` until the queue is empty** (one read may stand for many events).
5. For each event: `EVT_PIN_SUBMIT` / remote PIN commands → subroutine "PIN validation" (slide 52); `EVT_REMOTE_CHANGE_PIN` / `EVT_PIN_SAVE_RESULT` → subroutine "Change PIN" (N7); otherwise **dispatch to every FSM that handles it, Fire FSM first, then Operational, Power, Network**.
6. `EVT_KEY_DIGIT` → KEY\_BEEP; `EVT_FAULT` → first time per component: FAULT alert, fault blink.
7. Recompute outputs (speaker base, red LED, RGB) and status; send only what changed.
8. Back to 1.

Use subroutine (double-sided) boxes for the FSMs and PIN validation, as G4 slides 44–49 do with `Class::method()` boxes. Remove `get_events` (Network handles it) and the retry timer. \[Guide BR-LOOP-1–6, issues 23–26\]

#### Slide 52 — Flowchart — Pin validation

**REDESIGN · HIGH** (G9 R5, C4). Single start: "PIN request (source = LOCAL | REMOTE, pin)". First decision "source?": REMOTE → fire alarm? → already disarmed? → join; LOCAL → join. Then lockout? → hash/compare → correct? Wrong branch unchanged. Correct branch must become:

- Reset counter.
- Fire alarm active? (LOCAL only) → F-5 (silence, 10 min timer, ALARM\_SILENCED(FIRE) alert).
- opState ≠ DISARMED? → stop timers, DISARMED, log DISARMED(source); if it was INTRUSION\_ALARM and source LOCAL → ALARM\_SILENCED(INTRUSION).
- Nothing changed (already disarmed, no fire) → **no event**, KEY\_BEEP only.
- REMOTE → reply OK, then status.
- Single end "return to control loop".

Add the log order note: DISARMED → ALARM\_SILENCED(INTRUSION) → ALARM\_SILENCED(FIRE). Add on the lockout box: "counter reset at lockout end (EVT\_LOCKOUT\_TIMEOUT)". \[Guide §6.6\]

#### Slide 53 — Linux Device Driver

**CHANGE · CRITICAL** (G9 R6). The text content is correct and well-reasoned. It needs pictures and a precise interface, which are the course's mandatory deliverable. Keep this slide as "Driver — decision and interface" and fix: remove `-EBUSY` (C13); replace "command struct: channel, pattern, frequency, duty cycle, enable" with the 8-byte UAPI table below; add "Speaker duty 1–50 %, output LOW when disabled; `release()` leaves outputs as they are (fail-loud)". Then add the two new driver slides N9a/N9b (architecture + flowcharts).

| Field | Type | Values |
| --- | --- | --- |
| channel | u8 | 0 = SPEAKER (PWM0/GPIO18), 1 = LED (PWM1/GPIO13) |
| pattern\_id | u8 | 0 CONSTANT, 1 KEY\_BEEP, 2 ERROR\_BEEP, 3 WARNING, 4 INTRUSION, 5 FIRE, 6 LED\_FLASH, 7 LED\_BLINK |
| duty\_percent | u8 | 0–100; speaker 1–50 |
| enable | u8 | 0 stop (pin LOW), 1 start |
| frequency\_hz | u32 | CONSTANT only: speaker 100–10 000, LED 1–10 000 |

`write()` returns `sizeof(cmd)` or −1 with EINVAL / EFAULT / EIO / EINTR. \[Guide §8.1–8.5\]

### Part E — Parameters, GUI, verification and planning (slides 54–71)

#### Slide 54 — Parameters Times

**CHANGE · HIGH.** Rename "Timing Parameters". The 12 rows are correct but many timing values that drive code live only on other slides or nowhere. Add a **Status** column (decided / default / to measure) and these rows:

| Parameter | Value | Status |
| --- | --- | --- |
| Keypad + button scan / debounce | 15 ms / 2 stable scans | decided |
| PIN entry inactivity timeout | 10 s | default G3 |
| Wrong-PIN limit | 5 consecutive | decided |
| IR edge-wait timeout / retrigger guard | 1 s / 200 ms | default |
| MQ-2 sampling / confirmation / warm-up | 100 ms / 5 reads / 60 s | default — measure warm-up |
| INA219 period / hysteresis | 1 s / 3 samples | decided |
| Low battery / critical | ≤ 30 % / ≤ 6.4 V | decided |
| Controller epoll timeout | 1 s | decided |
| Heartbeat timeouts | LocalInput, MQ2 1 s; IR, Power, Controller, Output 3 s; Storage, Network 10 s | default |
| Watchdog | 15 s (bcm2835 maximum) | default |
| Outbox resend pacing | ≤ 5 alerts/s | decided |
| Shutdown waits | PUBACK 5 s, network 10 s, DB 3 s | decided |
| App command timeout | 32 s | default |
| PBKDF2 iterations | 20 000 (target 20–50 ms/hash) | measure |
| RGB fault blink / red LED flash / wrong-PIN blink | 500/500 ms; 250/250 ms; 3 × 100/100 ms | decided |

&#91;Guide §15\]

#### Slide 55 — GUI Interface (state/action table)

**CHANGE · MEDIUM.** G9 calls this "a good start". Rows to add so the app and the Pi agree: "Fire alarm **silenced**" (Arm ✓, Disarm ✗, Change PIN ✓; card amber); "PIN lockout" (commands sent; replies LOCKED\_OUT with minutes); "Shut down – low battery" (offline with reason). Replace the cell "Queued on Raspberry Pi" with "Alerts delivered after reconnection (outbox)". Add a footer: "The Pi is the source of truth; the app only displays retained status and replies." \[Guide §13.3–13.4\]

#### Slides 56–58 — GUI Interface (mock-ups)

**REDESIGN · MEDIUM** (G9 R9). The mock-ups are polished but not what the design phase asks for, and the intrusion card says "Motion detected" although the sensors detect door/window opening. Replace the three slides by **one or two structural wireframe slides**: grey boxes for Status card / Action row / Recent alerts (≤ 20) / Dialogs, each annotated with "data source" and "action → MQTT message → reply handling":

| Element | Shows / does | Source or message |
| --- | --- | --- |
| Status card | Highest of fire > intrusion > fire silenced > armed/disarmed; power; device online | `status`, `connection` (retained) |
| Arm | Confirm dialog → `cmd {type:arm}` | replies OK / ALREADY\_ARMED / REJECTED\_FIRE\_ACTIVE |
| Disarm | PIN dialog (masked) → `cmd {type:disarm, pin}` | OK / INVALID\_PIN / ALREADY\_DISARMED / LOCKED\_OUT / REJECTED\_FIRE\_ACTIVE |
| Change PIN | Current + new PIN → `cmd {type:change_pin}` | OK / INVALID\_PIN / INVALID\_NEW\_PIN / SAVE\_FAILED / LOCKED\_OUT |
| Recent alerts | ≤ 20 by `event_id`, duplicates dropped | `alert`, `event`, `get_events` |
| Waiting state | Spinner, 32 s timeout | — |

This also respects Guide §2.4 ("extra GUI slides" declined): fewer slides, not more. Keep one mock-up thumbnail if you want visual context. Change "Motion detected" to "Door/window opened (IR1)" in the app texts.

#### Slides 59–68 — Test cases

**CHANGE · MEDIUM** (G9 R10). Common fix for all ten slides: columns **ID · Objective · Stimulus / steps · Expected result · Requirement** (G12 slide 38 uses an ID column; G9 asks for the objective). Example rewrite of the first row of slide 59:

| ID | Objective | Stimulus | Expected result | Req. |
| --- | --- | --- | --- | --- |
| LI-01 | Verify that one arm-button press arms with exit delay | Disarmed, no fire; press button once | KEY\_BEEP ≤ 100 ms; red LED on; status stays DISARMED; ARMED(LOCAL) after 60 s | Flexible control |

Per-slide additions:

| Slide | Fix | Add rows |
| --- | --- | --- |
| 59 Local interface | Rename "Test Description" → "Stimulus"; add Objective | `1234#` submit; `*` clear; `#` empty; 10 s timeout; 9th digit; two keys at once; long press; 5 wrong → lockout; PIN during lockout; lockout end; button in each state (slide 18 table) |
| 60 Sensors | "object in front of IR 1" → "open door/window monitored by IR1 (LOW→HIGH)" (C19) | 200 ms retrigger; door open at arming (limitation); IR/MQ-2 line failure → FAULT once; MQ-2 5-sample filter (4 LOW reads → no event) |
| 61–62 Alarm logic | — | Remote arm in exit delay (cancels timer) / when armed / in fire alarm; remote disarm when disarmed; one PIN silences fire + intrusion (log order); smoke during silence; lockout during fire; no DISARMED event when already disarmed |
| 63 Alarm output | — | Wrong PIN during alarm → blink dropped (D4); beep over entry warning then warning resumes; GPIO18 LOW when idle (scope); PWM device missing → RGB fault blink + FAULT |
| 64 Power | — | INA219 sign check; LOW\_BATTERY once per outage; critical directly from BATTERY\_MODE; boot on mains after shutdown → POWER\_RESTORED(BOOT); boot on battery → POWER\_LOST |
| 65 Communication 1 | "App notified, even when closed" → "…with the foreground service running" | LOCKED\_OUT; INVALID\_NEW\_PIN; SAVE\_FAILED(BUSY); clean offline with reason; two phones, unique client IDs |
| 66 Communication 2 | "Delivered once" → "delivered at least once, oldest first; app deduplicates by event\_id" (C17) | Reconnection order online → status → alerts; malformed / retained / no-expiry command ignored; link lost before PUBACK |
| 67 Data storage | — | Alert insert + outbox row in one transaction; SENT only after PUBACK; corrupt DB with backup / without backup; old PIN still valid after failed save |
| 68 Real time | Add "Method" column | Method: `Event.monoNs` stamps, GPIO toggle + logic analyser, `cyclictest`, `stress-ng`; broker-loss ≤ 60 s and Wi-Fi-loss ≤ 2 s detection |

State on slide 68: "Figures are acceptance targets, to be measured". \[Guide §16.3–16.5\]

#### Slide 69 — Error Handling – Critical Cases

**CHANGE · HIGH** (C18). Add columns **Detected by (thread, mechanism)** and **Severity**, as G5 slide 63 and G12 slide 39 do. Correct two rows and add three:

| Area | Error | Detected by | Response | Severity |
| --- | --- | --- | --- | --- |
| Storage | DB fails at start | main: open / `quick_check` | Restore backup → else new DB + default PIN + FAULT(DATABASE) | High |
| Storage | DB fails while running | StorageThread write error | FAULT(DATABASE); RAM outbox; retry reopen every 60 s; alarm + PIN unaffected | High |
| Sensors | GPIO request/read error | IR / MQ2 thread | FAULT once; ignore sensor; retry request every 10 s ("stuck sensor" is **not** detected — remove the word) | Critical (MQ-2) |
| Start-up | Device/line missing | main | Degraded start, never refuse to start the alarm | High |
| Clock | Not synced after boot | NetworkMQTT (TLS "not yet valid") | Keep retrying; saved-date script; local alarm unaffected | Low |
| Power monitor | I²C fails | PowerMonitor | Retry next period; syslog after 10 (decision D13) | Medium |

&#91;Guide §14\]

#### Slide 70 — Gantt Chart

**CHANGE · OPTIONAL.** Today (4 Nov) the design-review bar should be running. Add implementation milestones with "done when" (Guide §19 M1–M10), at least: driver loads and passes slide-63 tests; AlarmLogic unit tests green; MQTT end-to-end on Mosquitto; hardware threads on the Pi; 24 h run. G5 slide 65 adds an estimated-cost table; optional.

#### Slide 71 — Closing

**KEEP — no changes required.** Insert the new "Design Decisions, Defaults and Limitations" slide (N10) just before it.

## New slides to create

Thirteen new slides (two optional) take the deck from 71 to about 82 after the GUI slides shrink by one or two. Each answers a professor request (P1–P5) or removes a guess the implementers would otherwise make.

| ID | Title | Insert after current slide | Form | Priority | Must update for consistency |
| --- | --- | --- | --- | --- | --- |
| N1 | Physical Deployment & Assembly | 12 (Hardware Connections) | Layout drawing + table | CRITICAL (P1) | 4, 11, 12 |
| N2 | Design Patterns & SOLID Map | 43 (class diagram) | Table | CRITICAL (P2) | 42, 43 |
| N3 | Class Diagram — Simplified by Package | 42 (before the full diagram) | UML class (main classes only) | HIGH (P3) | 42, 43 |
| N4 | Thread Responsibilities | 44 (task overview) | Table | CRITICAL (P4) | 44, 45, 54 |
| N5 | Event & Command Contracts | N4 | Table + short internal sequence | HIGH | 43, 44, 51 |
| N6a–d | Thread Flowcharts (2 threads per slide) | 51 (control loop) | Flowcharts | CRITICAL (P5) | 15–22, 54 |
| N7 | Flowchart — Change PIN (asynchronous) | 52 (PIN validation) | Flowchart | HIGH | 28, 37, 51 |
| N8 | Flowchart — Shutdown (low battery / SIGTERM) | N7 | Flowchart | MEDIUM | 39, 49 |
| N9a | Driver Architecture | 53 | Block/layer diagram | CRITICAL (G9 R6) | 5, 42 |
| N9b | Driver Flowcharts (`write()`, hrtimer, probe/remove) | N9a | Flowcharts | CRITICAL (G9 R6) | 53, 63 |
| N10 | Design Decisions, Defaults & Limitations | 70 (before closing) | Table | HIGH | all |
| N11 (opt.) | Dry Run — Intrusion then Disarm | 68 | Truth table | OPTIONAL | — |
| N12 (opt.) | Estimated Cost | 70 | Table | OPTIONAL | — |

### N1 — Physical Deployment & Assembly

No source decides this yet (Hardware Context only mentions "breadboard / protoboard" and a panel button "for the enclosure"). **Decision D1 \[New\], recommended:**

| Item | Decision |
| --- | --- |
| Bring-up | Solderless breadboard + Dupont wires, one circuit at a time |
| Final prototype | Soldered **protoboard (perfboard) interface board** carrying the speaker driver, MQ-2 divider, RGB transistors, resistors and screw/JST connectors; linked to the Pi through a 40-pin ribbon or stacking header above the UPS HAT |
| Enclosure | Wall-mounted box (3D-printed or ABS) holding Pi + UPS HAT + interface board, ventilated; front panel: keypad, Ø12 mm arm button, red LED, RGB LED, speaker grille |
| Panel location | Inside, next to the main entrance (owner disarms within the 30 s entry delay) |
| IR sensors | On the door/window **frame**, beam on the leaf, gap per sensor datasheet; 3-wire cable (5 V, GND, OUT) to the box |
| MQ-2 | High on wall or ceiling, away from kitchen steam and bathrooms; 4-wire cable (5 V, GND, DO; AO unused) |
| Not chosen | Custom PCB: no time in the Gantt and no benefit for 2 transistors and 10 resistors |

Draw it as a floor-plan sketch with the four mount points and a front view of the panel. G4 slide 20, G5 slides 31–32 and G12 slide 24 all show their physical build; VigilSystem is the only deck without one.

### N2 — Design Patterns & SOLID Map

| Pattern / principle | Where (class) | Problem it solves |
| --- | --- | --- |
| Producer–Consumer (Event Queue) | 6 producers → `EventQueue` → `AlarmControllerThread` | Many asynchronous sources, one deterministic decision maker |
| Command | `OutputCmd`, `StorageCmd`, `NetCmd` via `CommandQueue<T>` | Controller never blocks on sound, disk or network |
| Active Object | Output, Storage, NetworkMQTT threads, each with its own queue | Serialises access to one device / DB / socket without locks in callers |
| Monitor Object | `EventQueue`, `CommandQueue<T>` (PI mutex + condvar) | Thread-safe hand-over, no priority inversion |
| Template Method | `WorkerThread::start()/run()/heartbeat()` | Same lifecycle and heartbeat for 8 threads |
| Table-driven FSM | `AlarmLogic` (4 FSMs, pure, time injected) | Behaviour testable on a laptop |
| Adapter / Facade | `GpioLines` (libgpiod v1), `Ina219` (i2c-dev), `PwmDevice`, `MqttClient` (Paho), `Database` (SQLite) | Hides C APIs; v1→v2 swap touches one class |
| Observer (callback interface) | `IEventSink` implemented by NetworkMQTTThread | Storage tells Network an event is committed without depending on it |
| Transactional Outbox | `PENDING_NOTIFICATION` | No alert lost between DB commit and MQTT delivery |
| Supervisor / Watchdog | `SupervisorThread` + `/dev/watchdog` | Hung thread restarted, else hardware reset |
| RAII | all fd / line / SQLite handles | No leaked resources on error paths |
| S — Single responsibility | one job per thread; `PinHasher`, `Protocol` | — |
| O — Open/closed | new tone = driver table row; new event = enum value | — |
| L — Liskov | any `WorkerThread` is supervisable | — |
| I — Interface segregation | `IGpioInput`, `IGpioOutput`, `IPwm`, `IPowerSensor` | — |
| D — Dependency inversion | constructor injection from `main`; real vs simulated hardware | — |

Only claim the I/D rows if the class diagram shows those interfaces (slide 43 change list). Do not claim the GoF **State** pattern: the FSMs are switches in `AlarmLogic` (Guide D-47). \[Guide §5.5, §5.7\]

### N3 — Class Diagram, Simplified by Package

One box per main class, no attributes, orthogonal lines, grouped in the six packages named on slide 42: `threads` (8 workers + Supervisor), `core` (Event, EventQueue, CommandQueue, commands), `logic` (AlarmLogic, PinHasher), `hw` (GpioLines, Ina219, PwmDevice, Watchdog + 4 interfaces), `storage` (Database), `net` (MqttClient, LinkMonitor, Protocol). Show only: generalisation to WorkerThread, the controller → EventQueue / CommandQueue associations, thread → hw compositions, and `IEventSink`. \[G9 R3\]

### N4 — Thread Responsibilities

| Thread | Role (one line) | Activation | Policy / prio | Consumes | Produces | HW / interface | Heartbeat |
| --- | --- | --- | --- | --- | --- | --- | --- |
| main | Start-up, start threads, stop on SIGTERM | once; then `sigwait()` | — | signals | — | config, devices | — |
| LocalInput | Scan keypad and button, build PIN buffer | periodic 15 ms | FIFO 50 | — | KEY\_DIGIT, PIN\_SUBMIT, BUTTON\_PRESS | libgpiod (8 lines) | 1 s |
| IRSensors | Detect door/window opening | edge wait, 1 s timeout | FIFO 70 | — | MOTION\_DETECTED(1/2), FAULT | libgpiod 16, 20 | 3 s |
| MQ2Sensor | Confirm smoke/gas, 24/7 | periodic 100 ms | FIFO 80 | — | SMOKE\_DETECTED, FAULT; `isSmokePresent()` | libgpiod 21 | 1 s |
| PowerMonitor | Detect mains loss, battery level | periodic 1 s | FIFO 50 | — | POWER\_*, BATTERY\_* | i2c-dev INA219 | 3 s |
| AlarmController | Own all logic: 4 FSMs, timers, PIN | epoll, 1 s timeout | FIFO 80 | all events | OutputCmd, StorageCmd, NetCmd | none | 3 s |
| Output | Drive speaker, red LED, RGB | queue `popFor` ≤ 500 ms | FIFO 80 | OutputCmd | FAULT | /dev/vigil\_pwm, libgpiod 4/26/19 | 3 s |
| Storage | Persist events, outbox, PIN | queue `popFor` 1 s | OTHER | StorageCmd | PIN\_SAVE\_RESULT, FAULT; committed event → Network | SQLite | 10 s |
| NetworkMQTT | Broker link, publish, commands, resend | queue `popFor` 100 ms + netlink | OTHER | NetCmd, Paho callbacks | NET\_*, REMOTE\_* | Paho/TLS, netlink | 10 s |
| Supervisor | Heartbeats, restart, watchdog | periodic 1 s | OTHER | heartbeats | — | /dev/watchdog | watchdog |

&#91;Guide §5.2\]

### N5 — Event & Command Contracts

Left: event catalogue (`EventType` → producer → payload → handled by) exactly as Guide §5.4, including the three additions and the rule "`EVT_PIN_SUBMIT` payload is secret: never logged; buffers zeroed". Right: command catalogue (`OutputCmd` kinds, `StorageCmd` kinds, `NetCmd` kinds, Guide §5.5). Bottom: a 5-step internal sequence for one alert — Controller → `StorageCmd::LogEvent(isAlert)` → Storage inserts EVENT\_LOG + outbox row, commits → `IEventSink::onEventCommitted(event_id)` → Network publishes oldest PENDING → PUBACK → `markSent`. Ownership rule: "queues pass copies; no pointers to shared state cross threads."

### N6a–N6d — Thread flowcharts (one per thread, two per slide)

The professor asks for one flowchart per task (P5); five threads have none. Use G4's style (slides 44–49): each starts with "START \<thread>", shows initialisation once, then the loop with its blocking point, and calls subroutines as `Class::method()` boxes. Content:

| Slide | Thread | Loop steps |
| --- | --- | --- |
| N6a | LocalInput | init lines → \[sleep to next 15 ms deadline\] → for col 1..3: drive LOW, wait 20 µs, read rows, release → read button → exactly one key stable 2 scans and not yet reported? → digit: buffer < 8 ? append + KEY\_DIGIT : ERROR (D8); `#`: buffer empty? nothing : PIN\_SUBMIT(buffer), wipe; `*`: wipe → button HIGH→LOW stable 2 scans? BUTTON\_PRESS → 10 s since last key? wipe → heartbeat |
| N6a | IRSensors | request lines (fail → FAULT once, retry 10 s) → read initial levels → \[waitEdge 1 s\] → for each edge: update lastState; rising and > 200 ms since last? MOTION\_DETECTED(id) → heartbeat |
| N6b | MQ2Sensor | request line (fail → FAULT, retry 10 s) → \[sleep to next 100 ms\] → read → warm-up over? → count consecutive LOW/HIGH → 5 LOW and was clear? set smokePresent, SMOKE\_DETECTED : 5 HIGH? clear smokePresent → heartbeat |
| N6b | PowerMonitor | configure INA219, start state from main → \[sleep to next 1 s\] → read V, I (fail: count, syslog at 10) → 3-sample rules: lost / restored / level step / low (once per outage) / critical (once) → heartbeat |
| N6c | Output | open PWM + RGB lines → \[popFor 500 ms\] → command? translate (beep dropped if base FIRE/INTRUSION; blink dropped if FLASH) → `PwmDevice::writeCommand` → error? reopen + retry once → still error? FAULT → faultBlink and 500 ms elapsed? toggle RGB → heartbeat |
| N6c | Storage | open DB (done by main) → \[popFor 1 s\] → LogEvent: BEGIN, insert, outbox row if alert, COMMIT, `onEventCommitted` · SavePinHash: UPDATE, PIN\_SAVE\_RESULT, backup · CloseForShutdown: checkpoint, close → error? FAULT once, degraded, retry 60 s → heartbeat |
| N6d | NetworkMQTT | read broker.conf → connect (TLS) → \[popFor 100 ms + netlink poll\] → Connected: subscribe, online, status, drain, NET\_RESTORED · ConnectionLost / link down: NET\_DISCONNECTED, clear in-flight, schedule backoff · MessageArrived: validate → REMOTE\_\* or get\_events reply · Delivered: PUBACK 0x00/0x10 → markSent · LoggedEvent / Status / Response → publish if connected · backoff due? reconnect → heartbeat |
| N6d | Supervisor | open watchdog (from main) → \[sleep 1 s\] → for each worker: heartbeat older than its timeout? restart (2 s join) → > 3 restarts in 10 min or join failed? stop feeding : feed |

### N7 — Flowchart: Change PIN (asynchronous)

Start `EVT_REMOTE_CHANGE_PIN(request_id, pin, new_pin)` → lockout? LOCKED\_OUT(minutes) → hash/compare current → wrong? INVALID\_PIN, WRONG\_PIN(REMOTE), counter (5th → lockout) → reset counter → new\_pin 4–6 digits? else INVALID\_NEW\_PIN → save pending? SAVE\_FAILED(BUSY) → hash with new salt → `StorageCmd::SavePinHash` → remember pendingPinChange → **end (controller continues)**. Second entry from `EVT_PIN_SAVE_RESULT`: ok? replace RAM hash, reply OK, PIN\_CHANGED event : SAVE\_FAILED, old PIN active → clear pending. Two entry points are acceptable here only if drawn as two separate functions (`handleChangePin`, `onPinSaveResult`), each with one start and one end. \[Guide §6.6\]

### N8 — Flowchart: Shutdown

Two starts drawn as two functions: low-battery (Guide §6.9 steps 1–6) and SIGTERM (main: stop threads in reverse start order, stop both PWM channels, RGB off, close DB, magic-close watchdog, exit). G4 slide 43 shows a shutdown flowchart.

### N9a — Driver Architecture

Layer diagram top to bottom: OutputThread → `PwmDevice::writeCommand()` → `write(/dev/vigil_pwm, struct vigil_pwm_cmd)` → *kernel*: char device fops (open/write/release, mutex) → pattern engine (per channel: base + one-shot, hrtimer) → PWM consumer API (`pwm_apply_might_sleep` / `pwm_apply_atomic`) → pwm-bcm2835 → PWM0/GPIO18 → NPN → speaker; PWM1/GPIO13 → red LED. Side box: device-tree overlay `vigil-pwm` (pinmux ALT5/ALT0, `compatible = "esrg,vigil-pwm"`, `pwm-names = speaker, led`). Side box: pattern table (Guide §8.4).

### N9b — Driver Flowcharts

- **write():** size == 8? → copy\_from\_user (fail → EFAULT) → validate channel/pattern/duty/freq (fail → EINVAL) → lock (interrupted → EINTR) → hrtimer\_cancel(channel) → enable 0? disable PWM, base = off : one-shot? active = pattern : base = pattern → apply step 0 (fail → EIO) → more steps? hrtimer\_start → unlock → return 8.
- **hrtimer callback:** next step → end of one-shot? switch to base (repeat patterns from step 0) → apply atomically → constant/off? NORESTART : forward by step ms, RESTART.
- **probe / remove:** get both PWMs (defer if not ready) → disable outputs → init timers + mutex → chrdev region, cdev, class, device → `/dev/vigil_pwm`; remove in reverse, both outputs disabled.

### N10 — Design Decisions, Defaults & Limitations

One table: decision → chosen option → status (approved / default / to verify on hardware). Include D1–D13 below and the accepted limitations: door open at arming (G9), lockout counter reset on reboot (issue 17), always DISARMED after reboot (G10), no PIN accepted during lockout even during a fire alarm (fail-loud, D-32), TLS hop-by-hop, MQ-2 not certified, alerts "instant" only if the router has its own UPS.

### N11 / N12 — Optional

N11: a dry-run truth table like G5 slide 64 / G12 slides 40–41, rows = event sequence (arm, exit timeout, door opens, entry timeout, correct PIN), columns = opState, speaker pattern, red LED, RGB, published messages. Useful for the oral defence; not required. N12: cost table as G5 slide 65.

## What goes in the deck, the annex, or nowhere

The Implementation Guide is already a good technical annex; the deck should carry decisions and behaviour, and point to the annex for full contracts. Hand the annex over with the slides, and say so on slide 2.

**A. Main presentation (decisions, behaviour, critical interfaces)**

- Problem statement, physical deployment (N1), corrected schematic and GPIO table with direction/bias/active level/owner.
- Interface tables 15–22 with the added rows.
- Thread responsibilities (N4), corrected priorities (45), event/command contract summary (N5).
- Simplified + full class diagram, design-pattern map (N2).
- All four FSMs with complete transitions; flowcharts: start-up, control loop, PIN validation, Change PIN, one per thread, driver.
- Driver decision, UAPI table and diagrams.
- MQTT message and sequence slides (already there).
- Timing parameters with status column; error handling with detection and severity; test tables with objectives.
- Design decisions, defaults and accepted limitations (N10).

**B. Technical annex (needed to implement, too detailed to present)**

| Content | Source to reuse |
| --- | --- |
| Full FSM transition tables O-1…O-14, F-1…F-8, P-1…P-9, N-1…N-4 | Guide §6.3–6.5 |
| Complete `Event` / `OutputCmd` / `StorageCmd` / `NetCmd` definitions | Guide §5.4–5.5 |
| Output arbitration pseudocode and status mapping | Guide §6.10–6.11 |
| `vigil_pwm.h` UAPI header, pattern tables, device-tree overlay source | Guide §8.2–8.4 |
| SQL schema, statements, PRAGMAs, PBKDF2 string format, backup procedure | Guide §9 |
| MQTT topic tree, ACL, JSON examples for every message, validation rules | Guide §10 |
| config.txt `gpio=` lines, init-script order, Buildroot packages, BR2\_EXTERNAL | Guide §12 |
| Config file keys and secrets layout | Guide §11 |
| Full test catalogue incl. the ≈ 35 rows still to add | Guide §16 |
| Complete class diagram as a vector PDF (readable zoom) | slide 43 source |

**C. Do not add**

- Pseudocode or C++ snippets on slides (the professor assesses design, not code).
- More GUI mock-ups (G9 R9; Guide §2.4 declined extra GUI slides).
- A separate conceptual ERD slide if slide 26 is relabelled (option a).
- Repo layout, CMake targets, git rules, roadmap ownership (Guide §3, §19–20) — project management, not design.
- A sequence diagram per alert type: slide 36's note "every alert follows the same flow" is enough.
- Per-phone credentials, Firebase, REST backend — declined (Guide §2.4).

## ChatGPT Revision Guide — verdict on each recommendation

Seven of its items are already done in the 71-slide deck; most of the rest are correct and are folded into the slide-by-slide review. Three are rejected and one is redirected. Numbers below are ChatGPT's (68-slide deck) → current slide.

| ChatGPT item | Current slide | Verdict | Note |
| --- | --- | --- | --- |
| 15: delete "200 ms" from MQ-2 | 16 | **Already done** | MQ-2 now has its own slide without it |
| 15: MQ-2 warm-up, GPIO21, bias | 16 | Accept | Plus a defined recovery policy (D12) |
| 15: IR open-at-arming limitation | 15 | Accept | Document as limitation (D10) |
| 16: keypad pins, open-drain, rules | 17 | Mostly done | Add pins, key map, 9th digit (D8) |
| 17: push-button + RGB details | 18, 19 | Partly done | Add state table (G12); RGB depends on D2 |
| 18: red LED new priority must be approved | 20 | Accept the warning, **reject the new priority** | Keep Guide rule (D4) |
| 19: speaker GPIO18 LOW idle, ≤ 50 % | 21 | Accept | "Correct error-beep text" — already correct |
| "Add a Power Monitoring Interface slide" | 22 | **Already done** | Add INA219 set-up and boot rows |
| 21: COTS versions (libgpiod v1.6) | 24 | Accept | Plus nlohmann-json |
| 22: alert transaction ordering | 25 | **Already done** | Slide 25 ①–⑥ |
| 23: SQL in annex | 26 | Accept | Annex B |
| 25: Change PIN async + BUSY | 28 | Accept | And N7 flowchart |
| 27–31: details values | 30–34 | Accept | Several are already on the slides |
| 34: "Add LOCKED\_OUT to Change PIN" | 37 | **Already done** | Footer lists it |
| 38: wpa\_supplicant → netlink | 38, 41, 48 | Accept | C11 |
| 39–40: package/class diagram content | 42–43 | Accept | Plus G9 layout demands |
| 42: exact priorities | 45 | Accept — **and it missed** that the slide shows wrong numbers | C1 |
| 43: local PIN must silence fire despite the guard | 46 | **Already done** | Guard now says "Remote Disarm only" |
| 45: network FSM starts NET\_LOST | 48 | Accept | C2 |
| 46: BATTERY\_MODE → SHUTDOWN | 49 | Accept | C6 |
| 47–49: flowcharts | 50–52 | Accept | Plus single entry (G9) |
| 50: driver UAPI, remove EBUSY | 53 | Accept | Plus diagrams (G9) |
| 51: more timing parameters | 54 | Accept | Status column |
| 52–55: four GUI slides with more content | 55–58 | **Reject direction** | G9 wants less detail; Guide §2.4 declined extra GUI slides. Add states to the table, reduce mock-ups |
| 55: "response timeout = unknown outcome" wording | 58 / app | **Reject** | Declined on purpose (Guide §2.4); 32 s > 30 s expiry makes "not executed" true for undelivered commands |
| 62: qualify "even when closed" | 65 | Accept | Foreground service |
| 63: "at least once" | 66 | Accept | C17 |
| 65: measurement method | 68 | Accept | "Method" column |
| 66: remove unconditional "rebuilt", stuck sensors | 69 | Accept | C18 |
| 02: agenda additions | 2 | Accept, modified | Use the new slide titles |
| 09: "rename IR as not PIR" | 9 | **Already done** | Slides already say IR |
| SAFETY: lockout prevents local fire silence | 30, 47 | **Already decided** | D-32, fail-loud; list in N10 as accepted, do not reopen |
| Part E: hand over annex documents | — | Accept | Annex B |

What ChatGPT did not cover and the professor/G9 require: physical deployment (P1), design-pattern map (P2), class-diagram readability (P3), thread role table (P4), per-thread flowcharts (P5), SQLite library vs server (P6), schematic colour/voltage issues, driver diagrams. Those are the CRITICAL items of this plan.

## Comparison with G4, G5, G10 and G12

VigilSystem already exceeds all four decks on interface contracts, message specifications, MQTT sequences and parameters; none of them has anything like slides 15–22 or 28–39. The previous groups are stronger on physical build, per-thread flowcharts and verification tables. Adopt only what fills a VigilSystem gap:

| Practice | Where | Gap it fills in VigilSystem | Adopt? |
| --- | --- | --- | --- |
| Physical build: per-circuit schematics, 3D-printed enclosure | G4 slides 15, 20; G5 31–32 (PCB layout + 3D case); G12 21 ("implemented on protoboard"), 24 | P1 — VigilSystem has no deployment slide | **Yes** → N1 |
| NPN low-side driver for a common-anode RGB LED | G10 slides 10–11 (BC547 per channel, GPIO current ≈ 1.3 mA) | RGB anode at 5 V on 3.3 V GPIOs (G9 R2) | **Yes** → D2 |
| One flowchart per thread with mutex/condvar steps and `Class::method()` subroutine boxes | G4 44–49; G5 47–55; G12 32–35 | P5 — five threads without flowcharts | **Yes** → N6a–d |
| Separate start-up and shutdown flowcharts | G4 43 | No shutdown flowchart | **Yes** → N8 |
| Main-thread and worker-thread flowcharts tied to the concurrency model (epoll) | G10 18–21 | Control loop must show epoll drain and heartbeat | **Yes** → slide 51 redesign |
| Task overview with named queues, signals and mutexes on every arrow | G4 38 | Slide 44 arrows lack event/command names | Yes → slide 44 |
| Error-handling table with criticality and handling mechanism | G5 63; G12 39 (Severity column) | Slide 69 lacks detection and severity | Yes → slide 69 |
| Test table with ID column | G12 38 | No IDs; G9 asks for objectives | Yes → slides 59–68 |
| Dry-run truth tables per thread/scenario | G4 63; G5 64; G12 40–41 | Not required by G9 | Optional → N11 |
| Buildroot configuration screens | G10 13–15 | COTS versions unspecified | Partly — one line on slide 23, versions table on 24 |
| Password hashing parameters on a slide | G10 25 (Argon2id: salt, memory, iterations) | PBKDF2 parameters only in Guide | Optional — add to slide 25 or annex |
| Estimated cost table | G5 65 | Not required | Optional → N12 |
| Task priority as labelled bands | G4 41–42; G5 46; VigilSystem 45 | VigilSystem already uses bands — needs legend and correct values | Fix only |

**Not worth copying:** G4 37 describes its driver in two sentences of text only — VigilSystem's driver slide is already better, and G9 wants more, not that. G5 38–45 spreads the class diagram over eight slides without a simplified view — use N3 + one complete diagram instead. G10's client–server security slides (26–28) address threats VigilSystem does not have (no user accounts, no server).

## Final assessment

### 1. Critical gaps (must be closed before the Design Phase is complete)

1. **Slide 45 shows wrong scheduling values** (FIFO 80 instead of 50; SCHED\_FIFO instead of SCHED\_OTHER) — incorrect, not just incomplete.
2. **No physical deployment** (P1) — N1.
3. **No design-pattern / SOLID map** (P2) — N2.
4. **Class diagram unreadable and inconsistent** with the interface slides (P3, C8) — slide 43 + N3.
5. **No thread role table** (P4) — N4.
6. **Five threads without any flowchart** (P5) — N6a–d.
7. **Control-loop flowchart dispatches to one FSM only** (C7) — a keypad PIN during a fire alarm would be handled wrongly.
8. **Driver has no diagram or flowchart** (G9 R6) — N9a/b.
9. **Schematic legend untrustworthy and RGB 5 V/3.3 V issue** (G9 R2) — slide 12, D2.
10. **Red-LED rule contradicts the Guide** (C5) — slide 20.

### 2. Decisions still open

| ID | Ambiguity | Options and implications | Recommend | Basis |
| --- | --- | --- | --- | --- |
| D1 | Physical build | Breadboard only (fragile, fails G9) · protoboard board in an enclosure (cheap, robust) · custom PCB (no time) | Protoboard + enclosure, sensors on frame/ceiling | **New** |
| D2 | RGB LED drive | Keep 5 V anode sunk by GPIO (no parts, but 5 V-related node on a 3.3 V pin, faint glow) · NPN per cathode (+3 transistors, logic active-high) · 3.3 V anode (G/B V\_F 2.8–3.6 V → too dim) | NPN low-side | **New** (G10 precedent) |
| D3 | IR output stage | Open-collector → pull-up to 3.3 V · 5 V push-pull → 10 k/20 k divider | Measure; draw both on slide 12 until known | Hardware Context §17 |
| D4 | Wrong-PIN feedback during an alarm | Override flashing (slide 20; 100 ms blinks hard to see within 250 ms flashing) · drop it (Guide) | Drop; the continuing alarm is the feedback | **Guide** BR-PIN-7 |
| D5 | Who touches SQLite | Only Storage (Network must ask via commands and wait) · Storage writes + Network uses 3 mutex-protected methods | Second, written explicitly | **Guide** §7.8 |
| D6 | Wi-Fi loss detection | wpa\_supplicant control events (extra dependency) · netlink link monitor | Netlink | **Guide** §7.9 |
| D7 | Network FSM initial state | Boot check · always NET\_LOST | NET\_LOST | **Guide** N-1 |
| D8 | Keypad 9th digit | Overwrite oldest · ignore silently · ignore + ERROR\_BEEP | Ignore + ERROR\_BEEP | **New** |
| D9 | Arm button outside DISARMED | Restart exit delay · cancel arming · ignore + ERROR\_BEEP | Ignore + ERROR\_BEEP (none while an alarm sounds) | **Default** G12 |
| D10 | Door already open when arming | Accept (no detection until it re-opens) · refuse arming (needs a new reply code and beep) · start entry delay at once | Accept and document now; refuse-to-arm as future work | **Default** G9 |
| D11 | When the RGB fault blink stops | Never until reboot · when component recovers | Latched until reboot (matches "FAULT once per boot") | **New** |
| D12 | MQ-2 GPIO recovery | Undefined today | FAULT once, re-request line every 10 s | **New** (mirrors IR) |
| D13 | INA219 failure | Syslog only (owner never learns power monitoring is gone) · new `FaultComponent` UPS → FAULT alert | Add `UPS` FAULT (no schema change; `details` is free text) | **New** vs Default G8 |
| D14 | MQ-2 warm-up length | 60 s default | Measure, then fix the value on slide 54 | Default |
| D15 | PBKDF2 iterations | 20 000 default | Measure for 20–50 ms on the Pi | Default |
| D16 | libgpiod version | v1.6 (stable toolchain) · v2 (bleeding-edge toolchain, full rebuild) | v1.6, hidden in `GpioLines` | **Guide** §12.4 |

### 3. New slides

N1 Physical Deployment (after 12) · N3 Simplified Class Diagram (before 43) · N2 Design Patterns & SOLID (after 43) · N4 Thread Responsibilities and N5 Event & Command Contracts (after 44) · N6a–d Thread Flowcharts (after 51) · N7 Change PIN and N8 Shutdown (after 52) · N9a/b Driver Architecture and Flowcharts (after 53) · N10 Decisions & Limitations (before 71). Contents: section "New slides to create".

### 4. Existing slides to modify, by priority

| Priority | Slides | Action |
| --- | --- | --- |
| CRITICAL | 45 | Correct bands to FIFO 50 and SCHED\_OTHER; legend and justification |
| CRITICAL | 51 | Redesign control loop: drain, heartbeat, dispatch to all FSMs (Fire first), faults, key digits |
| CRITICAL | 43 | Redraw orthogonally; add missing events, command types, interfaces, IEventSink, PiMutex |
| CRITICAL | 12 | Signal-type colour legend; RGB driver per D2; IR output per D3 |
| CRITICAL | 20 | Replace wrong-PIN priority with Guide rule (D4) |
| CRITICAL | 53 | Remove EBUSY; UAPI table; link to N9a/b |
| HIGH | 46, 48, 49 | Move DISARMED/ARMED logging to transitions; Network starts NET\_LOST, no retry timer; Power adds BATTERY\_MODE→SHUTDOWN, boot alerts, level loop |
| HIGH | 50, 52 | Start-up: watchdog, degraded start, first boot, INA219, warm-up as parallel note; PIN validation: one entry, no event when nothing changes |
| HIGH | 9, 11, 13, 15, 16, 18, 25, 44, 54, 69 | Add rows/columns listed per slide; MQ-2 title and recovery; button state table; SQLite library + access rule |
| MEDIUM | 2, 3, 4, 5, 10, 17, 19, 21, 22, 24, 26, 28, 30–34, 36, 38, 41, 42, 47, 55, 56–58, 59–68 | Wording, missing values, legend, test objective column |
| OPTIONAL | 6, 14, 23, 39, 40, 70 | Clarifications |
| KEEP | 1, 7, 8, 27, 29, 35, 37, 71 | No change (29 and 37: one-line additions only) |

### 5. Lessons from previous groups

Adopt G4/G5/G12's physical-build slide, G10's NPN RGB driver, G4/G5/G12's per-thread flowcharts with sync steps, G4's shutdown flowchart, and G5/G12's severity and ID columns. Keep VigilSystem's interface and MQTT slides as they are: they are already the best of the five decks.

### 6. Implementation readiness

| Area | Deck alone | Deck + Implementation Guide as annex |
| --- | --- | --- |
| Hardware & wiring | Medium (legend, RGB, IR output, deployment open) | Medium until D1–D3 closed |
| Device interfaces (15–22) | High | High |
| Threads, priorities, queues | Low (wrong slide 45, no role table, no command types) | High |
| Controller behaviour (FSMs, PIN) | Medium (contradictions C2–C7) | High |
| Other threads' behaviour | Low (no flowcharts) | High |
| Driver | Medium (text only) | High |
| Storage / MQTT / app contract | High | High |
| Verification | Medium | High |

With the deck alone, an implementation team would have to guess: the real thread priorities; whether a keypad PIN during a fire alarm also disarms (the control loop says one FSM); how five threads loop; the command structures; the wrong-PIN LED rule; whether Network starts connected; how the driver is structured; how MQ-2 recovers; how the hardware is built and mounted. After the CRITICAL and HIGH items, only measured values (D3, D14, D15) and the hardware verifications in Guide §4.7 remain open, which is normal for a design phase.

### 7. Action plan (in order)

- [ ] Fix slide 45 priorities and add the legend (15 min, removes the only factually wrong slide).
- [ ] Approve D1–D13 as a team; write N10 with the outcome and update the Implementation Guide §18 to match.
- [ ] Redraw slide 51 control loop; redraw slide 52 with one entry and the "no event" branch.
- [ ] Fix FSM slides 46, 48, 49 and 47 labels.
- [ ] Fix slide 20 (D4) and slide 16 (title, warm-up, recovery D12).
- [ ] Create N4 Thread Responsibilities and update slide 44 arrows/labels.
- [ ] Create N6a–d thread flowcharts, then N7 Change PIN and N8 Shutdown.
- [ ] Redraw slide 43 (complete) and create N3 (simplified) with the slide-42 package names; then N2 pattern map.
- [ ] Create N9a/N9b driver diagrams; update slide 53 (UAPI, no EBUSY).
- [ ] Rework slide 12 legend and RGB/IR circuits (D2, D3); update 9, 11, 13, 19.
- [ ] Create N1 Physical Deployment.
- [ ] Add rows to slides 15, 17, 18, 21, 22, 25 (library + access rule), 28, 30–34, 54, 69.
- [ ] Rework test slides 59–68 (ID, Objective, added rows, wording fixes C17, C19).
- [ ] Shrink GUI slides 56–58 to wireframes; extend table on 55.
- [ ] Minor text: slides 2–5, 10, 23, 24, 26, 36, 38–41, 70; then refresh the Agenda.
- [ ] Consistency pass: search the deck for "Buzzer", "PIR", "motion", "wpa\_supplicant", "udhcpc", "EVT\_RETRY\_TIMER", "EBUSY", "FIFO 80" and resolve each.
- [ ] Update the Claude project instructions (they still describe four custom drivers and the old pin table, C22).
