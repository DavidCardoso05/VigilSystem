> **ARCHIVED — superseded on 2026-11-09 by `VIGILSYSTEM_MASTER_CONTEXT.md` v1.0. Kept for history only. Do not implement from this document.**
>
> Original location: Claude Project: `VigilSystem_Mobile_App_GUI_Design.md` · Document date: 31 Oct 2026 · Superseded by: Master Context §I.7.
> The text below this box is an unchanged copy of the original.

---

# VigilSystem – Mobile App GUI (Design Phase)

Oct 31, 2026 · Group 8 · David Cardoso (PG63873) · Ana Sofia Cunha (A107017)

This document records the GUI decisions for the VigilSystem mobile app, the interactive prototype built in Figma Make, how the GUI is presented in the report, and the plan to build the real app. It complements "VigilSystem – MQTT Broker Selection" (app scope, topics, foreground service, delivery rules) and does not change the broker-only architecture.

---

## 1. Summary of decisions

| Topic | Decision |
| --- | --- |
| App role | Thin remote control. The Pi validates the PIN and holds the state and history; the app sends commands on `cmd`, shows the replies from `response`, and never validates anything itself. |
| Platform | Flutter, Android first, portrait only, one phone. |
| Design system | Material Design 3 (Flutter defaults), Roboto font, Material Symbols (rounded) icons. |
| Theme | Light only. |
| Navigation | One single screen. No login, settings, history screen or bottom navigation bar. |
| Alerts | A large "Recent alerts" section on the main screen (not a second screen), holding the last 20 items. |
| Status states | The `status` message carries only armed / disarmed / alarm. Entry and exit delays are not shown as separate app states. |
| Colors | Standard semantic colors: green = armed (OK), grey = disarmed (neutral), amber = warning (battery, offline), red = alarm. Color is always paired with an icon and a text label. |
| Battery level | Reported and shown only in 10% steps (100, 90, 80…), so the device sends fewer status messages. |
| Frame size | Android Compact, 412 × 917 dp. Figma units match Flutter logical pixels (dp). |
| Prototype | Generated with Figma Make from a written specification and refined with follow-up prompts. It includes a "Demo controls" panel that simulates the Raspberry Pi; this panel is not part of the app. |

---

## 2. Color tokens

| Token | Main | Light tint | Used for |
| --- | --- | --- | --- |
| Armed / OK | `#2E7D32` | `#E8F5E9` | Armed state, mains power, device online, restored / cleared events |
| Disarmed / neutral | `#5F6368` | `#F1F3F4` | Disarmed state, arm / disarm / PIN-changed events |
| Warning | `#B26A00` | `#FFF4E0` | Battery power, device offline, power outage, wrong PIN, fire alarm silenced |
| Alarm | `#C62828` | `#FDECEA` | Intrusion and fire / gas alarms |
| Primary (neutral buttons) | `#1A4D8F` | – | Arm button, dialog buttons |
| App background | `#FAFAFA` | – | Screen background; cards are white |

Layout: 16 dp side margins, 8 dp spacing grid, card radius 16 dp, dialog radius 28 dp, fully rounded buttons, minimum touch target 48 dp, maximum elevation level 1 except dialogs, 200 ms transition when the status card changes state.

---

## 3. Main screen anatomy

| # | Part | Content | Data source |
| --- | --- | --- | --- |
| 1 | Top app bar | Title "VigilSystem", subtitle "Home 1" | App config file |
| 2 | Status card | Large state icon (48 dp) and label; rows Alarm, Power, Device; "Updated HH:MM". Light-tint background with a 4 dp left border in the state color. | Retained `status` and `connection` messages |
| 3 | Action row | Arm (filled), Disarm (filled tonal), Change PIN (outlined); equal width, 48 dp high | Publishes on `cmd`, waits for `response` |
| 4 | Recent alerts | Header "Recent alerts" with "Last 20"; items grouped under "Today" / "Yesterday", newest first; scrolls independently | `alert`, `event`, and `get_events` sync replies on `response`; stored locally in `shared_preferences` |

---

## 4. Screen states

| State | Card color | Icon | Label | Extra content |
| --- | --- | --- | --- | --- |
| Disarmed | Grey | `lock_open` | Disarmed | Alarm "None" |
| Armed | Green | `shield` / `lock` | Armed | Alarm "None" |
| Armed on battery | Green card, amber Power row | `shield` / `lock` | Armed | Power "Battery – 80%" (amber) |
| Intrusion alarm | Red, icon pulses slowly | `directions_run` | INTRUSION ALARM | "Motion detected at HH:MM" |
| Fire / gas alarm | Red | `local_fire_department` | FIRE / GAS ALARM | "Smoke or gas detected at HH:MM. Silence the alarm at the keypad. Remote disarm is not allowed during a fire alarm." |
| Device offline | Amber | `cloud_off` | Device offline | Alarm and Power show the last known values with "(last known)"; their text **and icons** are grey. Device row: "Offline – last seen HH:MM" (amber). |

Power row values: "Mains" (green plug icon) or "Battery – NN%" (amber battery icon, NN a multiple of 10).

---

## 5. Button rules

| State | Arm | Disarm | Change PIN |
| --- | --- | --- | --- |
| Disarmed | Enabled | Disabled | Enabled |
| Armed | Disabled | Enabled (tonal) | Enabled |
| Intrusion alarm | Disabled | Enabled, **filled red** (most prominent) | Enabled |
| Fire / gas alarm | Disabled | Enabled (tonal); the device rejects the disarm | Enabled |
| Device offline | Disabled | Disabled | Disabled |

General rule: Arm is disabled when the system is armed or an alarm is active; Disarm is disabled when the system is disarmed and no alarm is active. When the device is offline, all three buttons are disabled and a helper text appears below them: "Commands are unavailable while the device is offline."

---

## 6. Recent alerts list

- Keeps at most 20 items; when a new item arrives, the oldest is removed (verified in the prototype).
- Each item: colored circular icon, title, one line of detail, time on the right.
- Every new event is added at the top with its time.

| Title | Detail | Icon | Color |
| --- | --- | --- | --- |
| Intrusion alarm | Motion detected at home | `directions_run` | Red |
| Fire / gas alarm | Smoke detected by the MQ-2 sensor | `local_fire_department` | Red |
| Fire / gas still detected | Smoke persists after the silence period | `local_fire_department` | Red |
| Fire alarm silenced | Alarm silenced at the keypad | `notifications_off` | Amber |
| Hazard cleared | No smoke detected after the silence period | `check_circle` | Green |
| Power outage | Running on battery (NN%) | `battery_alert` | Amber |
| Power restored | Back on mains power | `power` | Green |
| Wrong PIN | Incorrect PIN entered at the keypad / in the app | `key` | Amber |
| System armed | Armed remotely from the app / at the arm button | `lock` (closed) | Grey |
| System disarmed | Disarmed remotely from the app / at the keypad | `lock_open` | Grey |
| PIN changed | PIN updated from the app | `key` | Grey |

---

## 7. Dialogs and command feedback

The app never validates. After every command it shows "Waiting for device…" with a circular progress indicator, then the device's reply.

| Command | Dialog | Device reply | What the UI shows |
| --- | --- | --- | --- |
| Arm | "Arm the system?" – "The system arms immediately, with no exit delay." Cancel / Arm | Success | State becomes Armed; snackbar "System armed" |
| Disarm | "Disarm system" – one masked numeric PIN field with show/hide; Cancel / Disarm | Correct PIN | Dialog closes; state becomes Disarmed (an intrusion alarm is silenced); snackbar "System disarmed" |
| | | Wrong PIN | Dialog stays open; field error "Wrong PIN. Try again."; "Wrong PIN" event added |
| | | Fire alarm active | Alert dialog with red fire icon: "Disarm rejected" – "A fire alarm is active. Silence it at the keypad." – OK |
| Change PIN | "Change PIN" – masked fields "Current PIN" and "New PIN"; Cancel / Save | Success | Snackbar "PIN changed"; "PIN changed" event added |
| | | Wrong current PIN | Field error on Current PIN: "Current PIN is wrong." |
| | | Save failed | **Dialog-level** error with an error icon, above the buttons: "The device could not save the new PIN. Try again." No field is marked red. |
| Any | – | No reply | "No response from the device. The command was not executed." (see open point 1) |

---

## 8. Android notifications

**Heads-up notifications:** fully opaque, slide down from the top for 5 s, show the app icon, "VigilSystem", the time, a bold title, one line of text and a colored accent stripe on the left. Tapping opens the app at the top of the alert list.

| Event | Title | Text | Accent |
| --- | --- | --- | --- |
| Intrusion | Intrusion alarm | Motion detected at home. Tap to open. | Red |
| Fire / gas | Fire / gas alarm | Smoke or gas detected at home. | Red |
| Smoke persists after silence period (critical) | Fire / gas still detected | The fire alarm has been reactivated. | Red |
| Power outage | Power outage | The system is running on battery (NN%). | Amber |
| Power restored | Power restored | The system is back on mains power. | Green |
| Hazard cleared | Hazard cleared | No more smoke or gas detected. | Green |
| Buffered events after reconnection | N alerts received | Events detected while the device was offline. | Amber |
| Same, batch includes intrusion or fire | N alerts received | Including: Intrusion alarm / Including: Fire / gas alarm (most recent if both) | Red |

**Permanent foreground-service notification:** "VigilSystem – monitoring" – "Connected to your home system". Silent, low-priority channel, muted style, no accent color.

**Notification shade:** shows the latest alert notifications expanded, with the monitoring notification at the bottom. Tapping an alert returns to the app.

Implementation note: the reconnection summary maps to an Android notification group with a summary notification.

---

## 9. System behavior reflected in the GUI

- **Intrusion** can only happen while the system is armed.
- **Fire / gas** is monitored 24/7, armed or disarmed. A new fire detection cannot start while a fire alarm is already active.
- **Fire silence period:**
  1. "Silence at keypad" returns the system to its previous armed / disarmed state, adds "Fire alarm silenced" (amber) and starts the silence period. New smoke detections are ignored during it.
  2. When the period ends and the smoke is gone: "Hazard cleared" event and notification.
  3. When the period ends and smoke persists: the fire alarm is reactivated, with the "Fire / gas still detected" event and a critical notification.
- **Device offline:**
  - The device keeps detecting and buffers its alerts in `PENDING_NOTIFICATION`.
  - The app keeps the last known values; the app cannot see state changes until reconnection.
  - On reconnection, the buffered events are delivered in chronological order with their original timestamps (verified in the prototype), the list is updated, one summary notification is shown, and the status card shows the device's current state.
- **One clock:** status bar time, "Updated", event times, notification times, "Motion detected at" and "last seen" all come from the same time source.

---

## 10. Power outage and connectivity

**Finding:** in a typical home, the router and the fiber ONT are on mains power with no backup. When the grid fails, they switch off at once. The Pi keeps running on the UPS HAT but has no internet, so the "Power outage" alert cannot be sent in real time; the router is already off when the UPS HAT reports the grid loss over I²C.

**Cases where the Pi stays online:**
- The router and ONT have their own small UPS.
- Only the Pi's circuit loses power (tripped breaker or RCD, unplugged or failed charger) while the router's circuit stays on.

In a wide-area blackout, operator equipment and mobile networks may also fail after their own batteries run out (e.g. the April 2025 Iberian blackout).

**What happens without a router UPS:**
1. The broker publishes the Last Will about 45 s after the outage (1.5 × the 30 s keep-alive), and the app shows Device offline. The app cannot tell whether the cause is power, internet or a crashed Pi.
2. Local protection continues on battery: buzzer, LEDs, "PIR", MQ-2 and keypad.
3. When power returns, the Pi delivers in order: "Power outage" (original time), anything detected during the outage, then "Power restored".

**Agreed for the report:**
- State a **recommended installation**: router and ONT on a small UPS.
- Qualify the requirement "instant alerts upon power grid failure": instant only with a router on backup power; otherwise the alert is delivered when connectivity returns.

---

## 11. Interactive prototype (Figma Make)

**How it was made:** one detailed specification prompt in Figma Make, followed by three rounds of follow-up fix prompts. The app part was checked against this document; everything listed in sections 3–9 was verified working.

**Demo controls panel** (beside the phone, dashed border, "not part of the app"):

| Group | Control | Rule |
| --- | --- | --- |
| Alarm events | Trigger intrusion | Only while armed |
| | Trigger fire / gas | Disabled while a fire alarm is active and during the silence period |
| | Silence fire at keypad | Only during a fire alarm |
| | Silence period ends – smoke cleared | Only during the silence period |
| | Silence period ends – smoke persists | Only during the silence period |
| Device events | Power outage / Power restored | Toggle the power source |
| | Battery −10% | Only while on battery; no event added |
| | Device offline / Device online | Toggle connectivity; events triggered while offline are queued |
| Other | "Device does not respond" switch | Commands time out (3 s in the demo) |
| | Open notification shade | Shows the Android shade |
| | Reset to Disarmed | Resets the demo |
| Info | "Queued on device: N", "Silence period active", state line (State · Power · Device) | – |

Demo PINs: `1234` is the correct PIN; new PIN `0000` simulates a failed save.

---

## 12. Presentation in the report

Planned section "Mobile Application – GUI Design":

1. **Scope and design principles:** thin remote control, Flutter, Material 3, light theme, single screen, color rule. Link to the broker document.
2. **Screen anatomy:** annotated main-screen screenshot with numbered callouts, plus the table from section 3.
3. **Screen states:** one figure with the six states side by side, plus the button-rules table.
4. **Interaction flows:** the table from section 7, each row linked to the sequence diagrams of the analysis (PIN Submission, Remote Disarm, PIN Change, Remote Arm), with dialog screenshots.
5. **Notifications:** heads-up and notification-shade screenshots, plus the table from section 8.
6. **Offline and reconnection behavior:** offline state, ordered delivery with summary notification, power-outage limitation and recommended installation.
7. **Requirements traceability:** functional requirement → GUI element.

Practical rules:
- Crop the demo panel out of every app figure; capture all screenshots at the same browser zoom.
- Explain the demo panel once.
- State that the prototype was generated with Figma Make from a written specification and refined iteratively (check the course rules on declaring AI tools), and include the prototype link.
- Slides: annotated screen, states grid and one flow (Disarm with its outcomes), or a live demo of the prototype.

---

## 13. Implementation plan (Flutter app)

**Environment:**
- Flutter SDK and Android Studio (Android SDK and emulator); `flutter doctor` all green.
- A real Android phone with USB debugging, because foreground services and battery optimization differ on the emulator.
- `flutter create --platforms=android vigil_app`.

**Packages** (as chosen in the broker document): `mqtt5_client`, `flutter_foreground_task`, `flutter_local_notifications`, `shared_preferences`.

**App layers:**

| Layer | Responsibility |
| --- | --- |
| `MqttService` | TLS connection, persistent session (fixed client ID, Clean Start = 0), QoS 1 subscriptions, publishing `cmd` with Message Expiry |
| `AppState` (`ChangeNotifier`) | Status, connection, alerts and the pending command |
| `AlertStore` | Last 20 events and the last `event_id` in `shared_preferences` |
| `NotificationService` | Heads-up alerts, summary group, permanent monitoring notification |
| UI widgets | `StatusCard`, `ActionRow`, `AlertList` and the dialogs, mirroring the prototype; status colors defined once in a theme extension |

**Isolate note:** `flutter_foreground_task` runs its task in a separate Dart isolate. The MQTT connection lives in the service, and the UI receives data through the plugin's task ↔ UI messaging, not by calling `MqttService` directly. Read the plugin's example before writing the MQTT layer.

**Android requirements:**
- `POST_NOTIFICATIONS` permission (Android 13+).
- Foreground-service type `specialUse` and its permission (Android 14+), to avoid the Android 15 `dataSync` time limit.
- Start at boot.
- Battery-optimization exemption request.

**Milestones:**
1. Static UI with fake data and a debug switch to cycle through the states, using the prototype as reference.
2. Connection to the local Mosquitto over TLS (test CA); `mosquitto_pub` plays the Pi (retained `status`, alerts). The Pi code is not needed yet.
3. Commands and replies: Arm, Disarm, Change PIN, with the waiting state, timeout and all error messages from section 7.
4. Foreground service and notifications; tests: alert with the app closed, after a phone reboot, after more than 24 h.
5. Event sync: `get_events` since the last `event_id`, duplicate discarding, 20-item limit.
6. Switch to EMQX via the config file; repeat connection, ACL rejection, end-to-end latency and ordered delivery tests.
7. Run the full test-case table from the broker document and record the results.

Milestones 1–3 only need a laptop, a phone and Mosquitto, so they can run in parallel with the embedded work on the Pi.

---

## 14. Open points

1. **Command timeout vs Message Expiry.** The app sets a 30 s Message Expiry on `cmd`. If the app reports "The command was not executed" before the expiry, the Pi could still reconnect and execute the command (e.g. a disarm the user believes failed). Either the app waits at least the expiry interval, or both are shortened (e.g. 10 s expiry, about 12 s app wait). Decide and write it into the MQTT specification. The prototype's 3 s timeout is for the demo only.
2. **Power outage in the prototype:** either (a) add a demo switch "Router on backup power" (off: a power outage also takes the device offline and queues the outage event; "Power restored" reconnects and delivers the queue), or (b) keep the demo as is and describe the limitation only in the report. To confirm.
3. **Device offline helper text** (proposed, to confirm): "The alarm keeps working locally. Events will be delivered when the device reconnects."
4. **Requirement wording** for power-failure alerts in the report (see section 10).
5. **Out of scope** (unchanged from the broker document): iOS, Firebase push, full history screen, configuring delays and the silence period from the app, multiple homes or users.
