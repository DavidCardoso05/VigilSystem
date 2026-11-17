> **ARCHIVED — superseded on 2026-11-09 by `VIGILSYSTEM_MASTER_CONTEXT.md` v1.0. Kept for history only. Do not implement from this document.**
>
> Original location: Claude Project: `VigilSystem – Design Decisions Record.md` · Document date: 2 Nov 2026 · Superseded by: Master Context §G.3 (D-01…D-51), §E, §I.
> The text below this box is an unchanged copy of the original.

---

# VigilSystem – Design Decisions Record

Nov 2, 2026 · @David

## 1. Purpose and status

This document records every design decision taken during the design-phase review of VigilSystem (Group 8 · David Cardoso PG63873 · Ana Sofia Cunha A107017), and the reason for each. It is meant to be added to the project context, next to the hardware, broker and GUI documents. Where it differs from an older document, **this one wins**. It replaces the earlier "Design Deck: Changes To Make" list, which is now mostly applied.

| Item | Status (2 Nov 2026) |
| --- | --- |
| Design deck | 61 slides. Hardware (4–14) verified. Cloud, messages, MQTT, data and GUI (15–32, 44–46) done. Package, class, FSM and flowchart slides (33–34, 37–43) redone. Task slides, driver slide, parameters, tests and error handling still being updated (section 9). |
| Ownership | David: slides 15–32 and 44–46. Ana: 33–43 and 47–59. David is now also finishing Ana's remaining slides. |
| Related documents | VigilSystem\_Hardware\_Context.md · MQTT Broker Selection (EMQX + Mosquitto) · Mobile App GUI Design |

Slide numbers below refer to the 61-slide deck.

## 2. Software architecture

**Decision:** the event-queue model from the task overview is the only architecture. The older class-diagram model (VigilSystemController, SensorMonitorThread, DelayTimer, AlarmActuator) was dropped because two architectures in one deck contradicted each other.

| Thread | Role | Interface |
| --- | --- | --- |
| LocalInputThread | Keypad scan every 10–20 ms (debounce 2 stable reads); arm button (falling edge, 20–50 ms) | libgpiod |
| IRSensorsThread | 2 IR door/window sensors (GPIO16, GPIO20), blocking edge events | libgpiod |
| MQ2SensorThread | MQ-2 digital output (GPIO21); warm-up; N consecutive reads | libgpiod |
| PowerMonitorThread | INA219 every 1 s, 3-sample hysteresis; battery level estimated | i2c-dev, addr 0x42 |
| AlarmControllerThread | epoll on the Event Queue (eventfd) and timers (timerfd); runs the 4 FSMs; owns exit, entry, silence and lockout timers | — |
| OutputThread | Speaker and red LED through the PWM driver; RGB LED through GPIO | /dev/vigil\_pwm, libgpiod |
| StorageThread | SQLite: EVENT\_LOG, PENDING\_NOTIFICATION, PIN\_CONFIG | SQLite |
| NetworkMQTTThread | Paho C++, MQTT 5 over TLS to EMQX | Paho, OpenSSL |
| SupervisorThread | Heartbeats from all threads; restarts a hung thread; feeds the watchdog | /dev/watchdog (bcm2835\_wdt) |

**Inter-thread communication:** one Event Queue (std::queue + mutex + condition variable + eventfd) from all producers to AlarmControllerThread, and one `CommandQueue<T>` each for OutputThread, StorageThread and NetworkMQTTThread. In the class diagram the eight worker threads extend an abstract `WorkerThread`. Timer events and PIN results are raised inside AlarmControllerThread and do not go through the queue.

**Thread names:** IRSensorsThread, not PIRSensorsThread, because the sensors are IR reflective sensors (slide 9) and every other slide says IR.

**Priorities (proposed for slide 36):** SCHED\_FIFO 80 for MQ2SensorThread, AlarmControllerThread and OutputThread. These three form the fire path, so the alarm cannot sound faster than its slowest stage. SCHED\_FIFO 70 for IRSensorsThread; 50 for LocalInputThread and PowerMonitorThread; SCHED\_OTHER for Storage, Network and Supervisor. The Supervisor is low on purpose: if lower threads are starved it stops feeding the watchdog and the Pi resets.

## 2.1 Drivers

**Decision:** exactly **one** custom Linux device driver, a PWM platform driver on the kernel PWM framework (pwm-bcm2835), exposed as `/dev/vigil_pwm`. It has two channels: PWM0 on GPIO18 drives the speaker, PWM1 on GPIO13 drives the red LED (steady, flashing, blinking). Tone and blink patterns run in the kernel with high-resolution timers; a command carries channel, frequency, duty cycle, enable and pattern id. Everything else uses mainline interfaces: libgpiod for keypad, button, IR, MQ-2 and the RGB LED, and i2c-dev for the INA219.

**Why:** the course requires at least one driver, and PWM is the only device with timing that benefits from kernel code. There is **no** LedDriver, KeypadDriver, GpioDriver or BuzzerDriver. The red LED is a PWM channel, and the RGB LED is three plain GPIO lines. The project context's old list of four custom drivers is obsolete.

**Consequences:** our own device-tree overlay claims GPIO18 and GPIO13, so the stock `pwm-2chan` overlay must not be loaded at the same time. If the PWM driver fails, the red LED fails with it, so the fallback indicator is the RGB LED plus a FAULT alert.

## 3. Cloud and communication

**Broker only.** EMQX Serverless is the only cloud component: no backend, REST API, cloud database or push service. The Pi is the source of truth and the broker only relays. Why: no subscription, nothing extra to secure, and the PIN never leaves the device in stored form. The phone receives alerts through an MQTT connection kept open in an Android foreground service. The old package diagram's "Backend / REST API / Cloud Database / Push" was removed for this reason.

| Decision | Rule | Why |
| --- | --- | --- |
| Outbox | Every alert gets a `PENDING_NOTIFICATION` row **before** it is published; SENT only after PUBACK; PENDING rows resent oldest first, ≤ 5/s | The Pi needs up to 60 s to notice a dead link and connects with Clean Start = 1, so alerts published in that window would otherwise be lost. 5/s stays under EMQX's 10 publishes/s |
| Log events | Not buffered; recovered by `get_events` | Keeps the outbox small; the app can ask for history |
| `get_events` | Returns the **20 newest** events with `event_id > since`, oldest first, one message, no paging | The app only keeps 20, so sending more wastes messages |
| Reconnection order | "online" → full retained status → PENDING alerts | The app shows the current state before older alerts arrive |
| App subscriptions | status, connection, alert, event, response; `get_events` after every CONNACK (or after "online") | `connection` is needed for "Device offline" |
| Client ID | App generates `vigil-app-<8 hex>` at first launch, stores it, reuses it | Two phones with the same ID would disconnect each other |
| `request_id` | `<client_id>-<counter>`; a phone ignores replies it did not request (event batches still merged) | All phones share the `response` topic |
| Never stop connecting | 0x86/0x87: retry every 5 min, log once to the system log. Other errors: backoff 1 s → 60 s | A stopped Pi would stay offline after the problem is fixed elsewhere, e.g. a password changed in the console |
| Detection bounds | Broker loss ≤ 60 s (2 × 30 s keep-alive); Wi-Fi loss ≤ 2 s (wpa\_supplicant event); broker publishes the Last Will after 45 s | Gives the connectivity requirement a number |
| Security | TLS 1.2/1.3 on each hop, certificate and hostname verified on both Pi and phone. TLS is hop-by-hop, so the broker sees `cmd` payloads, including the PIN: accepted trade-off, and `cmd` is never retained or stored | Stated so nobody assumes end-to-end encryption |

**Credentials.** The Pi uses user `vigil-device` (password in `/etc/vigil/secrets/mqtt_pass`, chmod 600); phones use `vigil-app` (config file inside the app). The ACL allows the device to publish status, alert, event, response and connection and to subscribe to cmd; the app the reverse. Everything else is denied. There is no login screen because there is no backend: disarm and PIN change are authenticated by the PIN, validated only on the Pi. Known limit: the app credential is shared, so per-phone EMQX users would be the way to revoke a lost phone. This is described but not adopted.

**Wi-Fi Specification slide:** prepared (802.11n/ac on CYW43455 + brcmfmac, WPA2-PSK via wpa\_supplicant, boot order brcmfmac → wpa\_supplicant → udhcpc → DNS → chrony → VigilSystem). It is not in the deck yet; the professor's example slide was a DDS/UDP template, and our equivalent is the communication stack plus this link-layer slide.

## 4. System behaviour rules

| Rule | Decision | Why |
| --- | --- | --- |
| Arming | Button → 60 s exit delay. Remote Arm → armed at once (also cancels a running exit delay). Both rejected while a fire alarm is active; allowed during the silence period | Nobody should arm while a fire is sounding; the silence period is no longer an alarm |
| Intrusion | IR ignored when disarmed and during the exit delay; when armed it starts the 30 s entry delay (warning tone, no notification). Timeout → intrusion alarm + alert | Lets the owner enter and disarm |
| Fire | Monitored 24/7. A fire alarm stays on until silenced; there is no automatic return to NORMAL | Fail-loud |
| Fire silence | A valid **keypad** PIN during a fire alarm stops the tone, starts the 10 min silence period and leaves the system **DISARMED**; it never re-arms | People are inside looking for the cause and would trigger the IR sensors |
| Silence timeout | No smoke → NORMAL + HAZARD\_CLEARED alert. Smoke → FIRE\_ALARM again (alert details REACTIVATED). Smoke is ignored during the silence period | Analysis requirement |
| Priority | Fire > intrusion > fire silenced > none. With both active the fire tone plays and the app shows the fire card; one valid keypad PIN silences both | One speaker and one status card can show only one alarm |
| Remote Disarm order | 1. fire alarm → REJECTED\_FIRE\_ACTIVE; 2. disarmed, no alarm → ALREADY\_DISARMED; 3. lockout → LOCKED\_OUT; 4. PIN → OK or INVALID\_PIN. Steps 1–3 do not check the PIN and log no WRONG\_PIN | A fire alarm can only be silenced on site; never test a PIN when the answer is already known |
| Remote Arm order | Fire alarm → REJECTED\_FIRE\_ACTIVE; armed, entry delay or intrusion → ALREADY\_ARMED; otherwise OK | The app can be stale (keypad used, second phone), so the Pi always answers |
| PIN lockout | One shared counter for keypad and app. 5 consecutive wrong PINs → 5 min lockout + PIN\_LOCKOUT alert. No PIN is checked during the lockout, even during a fire alarm (the alarm keeps sounding). Counter resets after a valid PIN or when the lockout ends | Closes PIN guessing: 10,000 combinations at 10 msg/s took about 17 min. A shared counter stops an attacker alternating sources. Adopted after first being declined |
| Low battery | ≤ 30 % (about 6.7 V, 3 samples) → one LOW\_BATTERY alert per outage | Warning before the shutdown |
| Shutdown | ≤ 6.4 V → SHUTDOWN alert (waits ≤ 5 s for PUBACK), retained offline message with reason LOW\_BATTERY\_SHUTDOWN, DISCONNECT 0x00 (no Last Will), sync SQLite, power off. At the next boot on mains → POWER\_RESTORED | Protects the SD card and tells the owner why the device is offline |
| Faults | One FAULT alert type; details IR1, IR2, MQ2, SPEAKER, PWM\_DRIVER or DATABASE; sent once per fault | Error handling says "notify", which needed a message |
| Database failure at start-up | Try the backup `vigil.db.bak` first; only if it has no PIN, load a default PIN from a root-only config, send FAULT (DATABASE) and require a PIN change | A default PIN is a security risk, so it is the last resort |

**Not adopted:** per-phone broker credentials and a PIN for Remote Arm (both discussed as hardening; arming without a PIN is a nuisance, not a breach).

## 5. Data, messages and status

**Database (SQLite on the SD card):**

| Table | Decision |
| --- | --- |
| EVENT\_LOG | `event_type` CHECK: ARMED, DISARMED, WRONG\_PIN, PIN\_CHANGED, PIN\_LOCKOUT, ALARM\_SILENCED, INTRUSION\_ALARM, FIRE\_ALARM, HAZARD\_CLEARED, POWER\_LOST, POWER\_RESTORED, LOW\_BATTERY, SHUTDOWN, FAULT. Times as ISO 8601 UTC text. `id` = `event_id` in every MQTT message; it orders events and drives the sync. Key digits and connectivity changes are **not** logged; connection rejections go to the system log |
| PENDING\_NOTIFICATION | Outbox (section 3); one row per alert, `event_id` UNIQUE |
| PIN\_CONFIG | Single row, loaded into RAM at start-up, never published. `pin_hash = pbkdf2_sha256$<iterations>$<salt>$<hash>` (salt included, no extra column). Why PBKDF2: a plain hash of a 4–6 digit PIN is reversed instantly by anyone with the SD card |

**Topics:** `vigil/home1/` + status, connection (both retained), alert, event, response, cmd.

**Alerts** (topic `alert`, outbox, phone notification): INTRUSION\_ALARM, FIRE\_ALARM (DETECTED or REACTIVATED), ALARM\_SILENCED (FIRE), HAZARD\_CLEARED, POWER\_LOST, POWER\_RESTORED, LOW\_BATTERY, SHUTDOWN, FAULT, PIN\_LOCKOUT.

**Log events** (topic `event`, no outbox): ARMED, DISARMED, WRONG\_PIN, PIN\_CHANGED, ALARM\_SILENCED (INTRUSION). The silenced *fire* alarm was moved to `alert` so the owner is notified; the intrusion version stays a log event.

**Status message (retained):**

- `armed_state` is DISARMED, ARMED or INTRUSION\_ALARM. The exit delay is reported as DISARMED (status becomes ARMED when it ends) and the entry delay as ARMED. Why: fewer messages; the internal FSM keeps both delay states.
- `fire_state` is NORMAL, FIRE\_ALARM or SILENCED. NORMAL is kept because status needs a value for "no fire".
- `power_source` is GRID or BATTERY; `battery_pct` is sent in 10 % steps only.

**Connection topic:** "online" (with timestamp) after CONNACK; clean offline message with reason and timestamp before a planned shutdown; Last Will "offline" without timestamp, because its payload is fixed at CONNECT and cannot carry the disconnection time. The app therefore shows "Offline" with no time.

**Command replies:**

- Remote Disarm: OK, REJECTED\_FIRE\_ACTIVE, ALREADY\_DISARMED, LOCKED\_OUT, INVALID\_PIN.
- Remote Arm: OK, REJECTED\_FIRE\_ACTIVE, ALREADY\_ARMED.
- Change PIN: OK, LOCKED\_OUT, INVALID\_PIN, INVALID\_NEW\_PIN (new PIN must be 4–6 digits), SAVE\_FAILED.
- `cmd` uses a 30 s Message Expiry. The Pi uses a clean session (session expiry 0), so a stale command is never executed later.

## 6. Mobile app and GUI

The app is a thin remote control (Flutter, Android, one screen). It never validates anything; the Pi decides.

| Topic | Decision | Why |
| --- | --- | --- |
| No login screen | Not needed | There are no user accounts because there is no backend. Disarm and PIN change need the PIN, and the ACL limits the app credential to sending commands |
| Status card mapping | Fire alarm (red) > intrusion alarm (red) > "Fire alarm silenced" (amber) > Disarmed / Armed. With fire and intrusion active, the fire card is shown and Disarm is in the tonal style, because the Pi rejects it | Follows the priority rule |
| Offline | Card "Device offline", last known values in grey, all buttons disabled, Device row "Offline" (no time); "Shut down – low battery" when the reason says so | Matches the Last Will limitation |
| ALREADY\_ARMED / ALREADY\_DISARMED | Dialog closes, snackbar "The system is already armed / disarmed", card updated from status; no error, no Wrong PIN event | The app view can be stale |
| LOCKED\_OUT | Dialog error "Too many wrong PINs. Try again in N min."; PIN\_LOCKOUT alert row and notification | Lockout rule |
| New alert rows | Fire alarm silenced (amber), Intrusion alarm silenced (grey), Low battery, System shut down, Device fault, PIN entry locked | New alert and event types |
| Alert list | At most 20 items everywhere (app, slides, broker doc), ordered by `event_id` | Single number; correct order after reconnection |
| Reconnection | One "N alerts received" summary notification instead of one per resent alert | Avoids a burst of notifications |

**GUI slides (44–46):** no more GUI slides will be added (decided by David). Known capture details that remain: the broken Disarmed icon in two captures, and the red Disarm button in the fire + intrusion capture should be tonal.

## 7. Diagrams

**FSMs and flowcharts both stay.** FSMs describe behaviour that depends on the current state, which is the alarm logic. Flowcharts describe procedures that run: start-up, the control loop, PIN validation. The control-loop flowchart is `AlarmControllerThread::run()`, and each FSM becomes a `switch` on the state inside `handleEvent()`. Drawing the FSM logic as a flowchart would need a "which state?" diamond for every event.

**Drawing method:** Graphviz DOT, rendered at 200 dpi for readability. FSMs use circle states with an entry action inside, a filled start dot and `event [guard]` on the arrows. Flowcharts use dark terminators (#1F3A5F), light-blue process boxes (#E8F0FA), yellow decisions (#FFF4D6) and red error boxes (#FDE8E8). Why entry actions: they remove the repeated actions on every arrow, which roughly halved the text.

| FSM | States | Key points |
| --- | --- | --- |
| Operational (37) | DISARMED, EXIT\_DELAY, ARMED, ENTRY\_DELAY, INTRUSION\_ALARM | Button and Remote Arm guarded by `[fire_state != FIRE_ALARM]`; Remote Arm also from EXIT\_DELAY; every non-disarmed state returns to DISARMED on EVT\_PIN\_CORRECT or EVT\_REMOTE\_DISARM `[fire_state != FIRE_ALARM && PIN valid]`. The arrows are labelled individually (grey colouring was not visible). Pending fix: put the guard on its own line so it clearly belongs only to Remote Disarm |
| Fire (38) | NORMAL, FIRE\_ALARM, SILENCED | No FIRE\_ALARM → NORMAL arrow; silence by EVT\_PIN\_CORRECT (keypad), not a "silence button"; smoke ignored in SILENCED; remote disarm rejected in FIRE\_ALARM |
| Network (39) | NET\_CONNECTED, NET\_LOST | Entry actions publish online + status and resend PENDING; retry self-loop with backoff |
| Power (40) | AC\_POWERED, BATTERY\_MODE, LOW\_BATTERY, SHUTDOWN (final) | LOW\_BATTERY at ≤ 30 %, SHUTDOWN at ≤ 6.4 V |

| Flowchart | Key points |
| --- | --- |
| Start-up (41) | Queues → PWM module + GPIO lines → database check (backup, then default PIN + FAULT) → PIN loaded once → launch threads → DISARMED → MQ-2 warm-up (IR needs none) → normal operation |
| Control loop (42) | epoll wait → dispatch to the 4 FSMs or to commands (change\_pin, lockout timer) → apply transition → send commands to Output, Storage, Network |
| PIN validation (43) | Remote checks fire and already-disarmed first; both paths check the lockout, hash with PBKDF2, count wrong PINs (5 → lockout); a correct PIN during a fire alarm silences it and disarms |

**Class diagram (34):** WorkerThread base class with generalization arrows; `EventType` split into queued events and events raised inside the controller; `CommandQueue<T>`; `PwmDevice.writeCommand(channel, patternId, frequencyHz, dutyPercent, enable)`; StorageThread outbox methods `insertPendingAlert`, `markSent(id: int)`, `getEventsSince(eventId: int)` (max 20).

## 8. Parameters and tests

**Parameters (slide 48):**

| Parameter | Value |
| --- | --- |
| Default entry delay / exit delay / silence period | 30 s / 60 s / 10 min (configurable in the device config file) |
| Wrong-PIN limit / PIN lockout | 5 attempts / 5 min |
| MQTT keep-alive (Pi / app) | 30 s / 60 s |
| Broker offline detection (Last Will published) | ≈ 45 s (1.5 × keep-alive); "Last Will delay" was renamed because it is not the MQTT 5 Will Delay property |
| Pi broker-loss / Wi-Fi-loss detection | ≤ 60 s / ≤ 2 s |
| Reconnect backoff / after auth error | 1 s → 60 s / every 5 min |
| Command Message Expiry (`cmd`) | 30 s |
| Outbox resend pacing | ≤ 5 alerts/s |
| Shutdown wait for PUBACK | 5 s |
| Session expiry (Pi / app) | 0 / maximum accepted by EMQX |
| Keypad scan / debounce; button debounce | 10–20 ms / 2 stable reads; 20–50 ms |
| INA219 polling / hysteresis | 1 s / 3 samples |
| **Still to define** | MQ-2 warm-up time, app command timeout, PIN entry timeout, supervisor/watchdog timeout |

**Tests (slides 49–58):** David is updating them by hand. About 35 rows were identified as missing:

- **Local interface:** lockout, PIN during lockout, lockout end, arm button during a fire alarm.
- **Sensors:** sensor fault.
- **Alarm logic:** remote arm in exit delay / in fire alarm / when armed, remote disarm when disarmed, fire silence while armed, fire + intrusion with one PIN, smoke during silence, lockout during fire.
- **Output:** PWM driver failure, speaker idle level.
- **Power:** low battery alert, critical shutdown, boot after shutdown.
- **Communication:** locked out, invalid new PIN, save failed, undetected link loss, reconnection order, clean shutdown message, two phones, unique client ID, fire-silenced notification.
- **Data storage:** outbox row, database restore, no database and no backup, not logged.
- **Real time:** broker-loss and Wi-Fi-loss detection times.

**Rows to correct:**

- Slide 54 "Low Battery … (if implemented)"
- Slide 56 "Under 5" → "At most 5"
- Slide 57 Event Logging (remove "connectivity") and History Query (20 newest)

## 9. Declined options and open items

**Declined on purpose (do not reopen without a reason):**

| Option | Decision |
| --- | --- |
| Cloud backend, REST API, cloud database, Firebase push | No: broker only (section 3) |
| Last Will with a disconnection time | Not possible: payload fixed at CONNECT; the app shows "Offline" only |
| "Skipped events" count in `get_events` | Not added |
| App command timeout vs Message Expiry, PUBACK 0x10 offline detection, timeout message wording | Left as designed |
| Extra GUI slides (dialogs, offline, reconnection summary) | Not added |
| Slide 19: ONT box and "recommended small UPS" | Not added |
| Slide 26 note wording ("keypad" vs Remote Disarm for intrusion silence) | Not changed |
| Per-phone broker credentials, PIN on Remote Arm | Described, not adopted |
| Four custom drivers (LED, keypad, GPIO, buzzer) | Replaced by one PWM driver |

**Still open in the deck (last check, 2 Nov 2026):**

1. Slides 35–36 still say PIRSensorsThread; slide 36 lacks the scheduling policy and numbers (section 2).
2. Slide 47 lacks the "only custom driver" line and the driver interface (device node, file operations, command struct, error codes, overlay note).
3. Slides 48–59: parameters, tests and error handling, being done by hand (section 8).
4. Wi-Fi Specification slide not yet in the deck.
5. Small diagram fixes:
   - Operational FSM: put the guard on its own line.
   - PIN flowchart: fire branch also silences the intrusion alarm.
   - Control loop: change\_pin goes through the lockout and PIN check; overlapping branch lines and stray dots.
   - Class diagram: reroute the RGB line.
   - Package diagram: link the input threads to libgpiod, PowerMonitor to i2c-dev, and the Supervisor to the threads.
   - Slide 29 footer: Change PIN lacks LOCKED\_OUT.
   - Power FSM: smaller font than the others.

**Outside the deck:** the GUI doc and broker doc still need the matching text changes (client ID, request\_id, 20 events, CommunicationThread → NetworkMQTTThread). In the project context, section 6.1 (four drivers, old class names) and section 4 ("PIR", the old pin table) are superseded by this document.

**Review history:** first cross-check of the cloud/app design → full-deck comparison with Group 4 → split into David's and Ana's changes → lockout adopted → package, class, FSM and flowchart diagrams redrawn in Graphviz/PlantUML → final consistency checks.
