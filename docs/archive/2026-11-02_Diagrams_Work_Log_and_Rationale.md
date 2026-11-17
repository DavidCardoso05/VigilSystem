> **ARCHIVED — superseded on 2026-11-09 by `VIGILSYSTEM_MASTER_CONTEXT.md` v1.0. Kept for history only. Do not implement from this document.**
>
> Original location: Claude Project: `VigilSystem – Design-Phase Diagrams Work Log and Rationale.md` · Document date: 2 Nov 2026 · Superseded by: Master Context §E.4, §G.
> The text below this box is an unchanged copy of the original.

---

# VigilSystem – Design-Phase Diagrams: Work Log and Rationale

Nov 2, 2026 · @David

## Purpose and scope

This document records the eight design-phase diagrams produced for VigilSystem (Group 8): four state machines and three flowcharts, plus the first Operational FSM iterations. For each one it states what was drawn, why it was drawn that way, and what the check against the project context found.

It complements the existing project documents (Hardware Context, MQTT Broker Selection, Mobile App GUI Design) and follows the agreed architecture: AlarmControllerThread runs the four FSMs, one custom PWM driver (/dev/vigil\_pwm), libgpiod and i2c-dev for everything else, and EMQX Serverless as the only cloud component. Section "Open issues" collects every contradiction found, so they can be fixed in the report and slides.

## Deliverables

All files were rendered and checked visually before delivery. The Graphviz files render with `dot -Tpng -Gdpi=200 <file>.dot -o <file>.png`.

| Diagram | Source file | Rendered file | Tool | Status |
| --- | --- | --- | --- | --- |
| Operational FSM (slide version) | operational\_fsm.puml | Operational\_FSM.png / .svg | PlantUML | Superseded by your Graphviz version |
| Operational FSM (full guards and actions) | operational\_fsm\_detailed.puml | – | PlantUML | Reference for the report |
| Fire FSM | fire\_fsm.dot | fire\_fsm.png | Graphviz | Current |
| Fire FSM (first version) | fire\_fsm.puml | – | PlantUML | Superseded |
| Network FSM | network\_fsm.dot | network\_fsm.png | Graphviz | Current |
| Power Management FSM | power\_fsm.dot | power\_fsm.png | Graphviz | Current |
| Start-up flowchart | startup\_flowchart.dot | startup\_flowchart.png | Graphviz | Current |
| PIN validation flowchart | pin\_validation\_flowchart.dot | pin\_validation\_flowchart.png | Graphviz | Current |
| AlarmControllerThread control loop | controller\_loop\_flowchart.dot | controller\_loop\_flowchart.png | Graphviz | Current |

## Diagram conventions

Two house styles were used, both defined by the group and both sized for a 4:3 slide.

**State machines (Graphviz).** States are fixed-size circles (width 1.9, Arial 13) with the bold name and the entry action in 9 pt. Arrows carry "EVENT \[guard\]" in Arial 11. An action goes on an arrow only when it belongs to that transition and not to the target state. Ignored events and reply-only events are self-loops. Repeated rules go in one note at the bottom (fill `#FFF6E0`). The initial state is a filled point, with a small diamond when the start state depends on a boot-time check; a final state is a double circle.

**Flowcharts (Graphviz).** Top-down, orthogonal lines, Arial 12. Start and end boxes are dark blue (`#1F3A5F`), process steps light blue, decisions yellow diamonds with bold Yes / No, and errors or rejections red (`#FDE8E8`). Box texts stay within about 8 words per line and 3 lines.

**Techniques used to make the layouts fit:**

- **Yes / No labels as `xlabel`.** With `splines=ortho`, Graphviz drops ordinary edge labels.
- **Two columns for long flows.** The start-up flowchart continues in a second column, joined by a single line, because nine steps in one column were too tall for 4:3.
- **Join points.** Branches that meet again end in a small point, so the merge reads as one bus line.
- **Loop-back through helper points.** Orthogonal routing ignores edge ports, so the control-loop return line runs through a column of tiny points on the left instead of cutting through the chart.
- **Rows forced with `rank=same`.** Used to place decisions beside their branches and to put a note in an empty corner instead of adding a row.

**Why PlantUML was dropped.** The first two FSMs were written in PlantUML. Its layout put the superstate's arrows on one point of its border and stacked labels on top of each other, which the group found unreadable. The later diagrams use your Graphviz style, which gives more control over placement.

## Operational FSM

The Operational FSM went through three PlantUML versions; the group then redrew it in Graphviz, and that version is the style reference for the other diagrams.

| Version | What changed | Why |
| --- | --- | --- |
| 1 – full labels | Every arrow carried "event \[guard\] / action". EXIT\_DELAY, ARMED, ENTRY\_DELAY and INTRUSION\_ALARM were grouped in a "Not disarmed" superstate, so the two disarm transitions and the wrong-PIN loop were drawn once instead of four times. | Without grouping, eight long disarm arrows would not fit a 4:3 slide. |
| 2 – readable layout | Rendered and checked. DISARMED moved to the top, the four states placed in a loop. EVT\_PIN\_WRONG became an internal transition written inside the states. | Version 1 overlapped labels at the superstate border. A self-loop on a superstate also exits and re-enters it, which would wrongly reset it to its first substate. |
| 3 – simple lines | Arrows show only the event names. Shared arrows list two events. The guards became one note beside DISARMED. | The group asked for simpler lines with less information. The full version was kept as operational\_fsm\_detailed.puml for the report. |

**Additions beyond the request, each from the project context:**

- Remote Arm from EXIT\_DELAY also stops the exit timer, because the rules say it cancels a running exit delay.
- Both disarm transitions also stop the timers, and Remote Disarm sends an OK reply, matching the System Events table.
- The wrong-PIN handling also applies in DISARMED, because a keypad PIN is still checked there (for example to silence a fire alarm) and counts toward the lockout.

## Fire FSM

The Fire FSM has three states (NORMAL, FIRE\_ALARM, SILENCED) and deliberately no arrow from FIRE\_ALARM to NORMAL: a fire alarm stays on until a valid keypad PIN silences it.

**Design choices:**

- **Triangle layout.** NORMAL is on top, FIRE\_ALARM and SILENCED side by side below. Three circles in one column would have been a tall, portrait diagram.
- **Actions in the states.** Fire tone, red LED flashing and the FIRE\_ALARM alert are FIRE\_ALARM's entry action, so the reactivation arrow only adds the alert detail "REACTIVATED".
- **Cross-FSM action on the arrow.** "Operational FSM → DISARMED" stays on the FIRE\_ALARM → SILENCED arrow, because it belongs to that transition, not to every entry into SILENCED.
- **Two self-loops.** Smoke is ignored in SILENCED. EVT\_REMOTE\_DISARM in FIRE\_ALARM only produces the REJECTED\_FIRE\_ACTIVE reply.
- **No lockout guard on the PIN arrow.** This follows the convention set in the Operational FSM: during a lockout no PIN events are generated at all.

**What the check found:** the GUI design document (section 9) says silencing returns the system to its previous armed or disarmed state, while the project context says it always leaves the system DISARMED. The diagram follows the project context. A self-loop for smoke detected during FIRE\_ALARM was left out because it was not in the requested transition list. See "Open issues" for the full list.

## Network FSM

The Network FSM has two states, NET\_CONNECTED and NET\_LOST, chosen at boot by a "connected at boot?" diamond.

**Design choices:**

- **Boot decision.** The start state depends on whether the first broker connection succeeds, so a diamond replaces a fixed start state.
- **Reconnect order as an entry action.** NET\_CONNECTED's entry action publishes "online", then the status, then resends PENDING alerts at ≤5/s. "Oldest first" was added because the project context sets that order.
- **Reconnect as a self-loop.** EVT\_RETRY\_TIMER on NET\_LOST carries "/ reconnect" with the 1 s → 60 s backoff and the 5 min retry after authentication errors, because that action belongs to the retry, not to entering NET\_LOST.

**What the check found:** the retry timer and the Wi-Fi detection in the note are not in the agreed architecture.

- The retry timer is NetworkMQTTThread's own job, not an AlarmControllerThread timer or a queued event.
- "Wi-Fi loss ≤2 s" needs NetworkMQTTThread to listen to wpa\_supplicant or netlink, which no thread description includes.
- Broker loss is detected on the device within ≤60 s; the broker publishes the Last Will after about 45 s. These are two different numbers and should not be mixed in the report.

## Power Management FSM

The Power FSM has four states (AC\_POWERED, BATTERY\_MODE, LOW\_BATTERY and the final SHUTDOWN), chosen at boot by a "mains at boot?" diamond.

**Design choices:**

- **2 × 2 grid.** AC\_POWERED and BATTERY\_MODE on top, LOW\_BATTERY and SHUTDOWN below. The note sits in the empty bottom-left corner; on its own row it made the diagram taller than wide.
- **Power alerts on the arrows.** POWER\_RESTORED is on both arrows into AC\_POWERED, not in its entry action, because as an entry action it would also fire on every normal boot with mains. POWER\_LOST is on the arrow out of AC\_POWERED for the same reason.
- **SHUTDOWN as a double circle.** It is the only final state in the four FSMs. Its entry action keeps the required order: SHUTDOWN alert, retained "offline", DISCONNECT 0x00 (which suppresses the Last Will), SQLite sync, power off.

**What the check found:**

- The note promises a POWER\_RESTORED alert at the next boot after a SHUTDOWN, but the "yes" branch has no action for it.
- There is no BATTERY\_MODE → SHUTDOWN path, which a voltage sag under load could need.
- A halted Pi 4 may not restart by itself when mains returns, which is a hardware risk to test.

These are detailed in "Open issues".

## Start-up flowchart

The start-up flowchart loads the PIN exactly once on every path, then launches the threads and reaches normal operation.

**Design choices:**

- **Two columns.** The left column runs from power-on to the PIN decisions; the right column runs from "Launch threads" to normal operation. One column of nine steps was too tall for 4:3.
- **One PIN load per branch.** Each of the three database branches has exactly one PIN-loading box: from the database, from the restored backup, or the default hash from the root-only config. All three meet at a join point before any thread starts.
- **Error style for the default-PIN branch.** It is the only path that raises FAULT (DATABASE) and forces a PIN change.
- **"Default PIN hash" wording.** The box says hash, because the security requirement forbids storing the PIN in plain text, even in the config file.

**What the check found:**

- The default-PIN branch has no working database, so the FAULT alert cannot go through the outbox until a new empty database is created.
- The backup vigil.db.bak has no defined update rule, so a restore could load an old PIN.
- Startup can block on NTP with no internet, which would stop the alarm from starting.
- The MQ-2 warm-up runs in parallel in MQ2SensorThread, not as a blocking step, and fire detection is off while it lasts.

## PIN validation flowchart

The PIN validation flowchart merges the local and remote paths at the lockout check, so one shared counter and one hash comparison serve both.

**Design choices:**

- **Two starts on the top row.** "Local" goes straight down; "Remote" runs sideways through its two remote-only checks (fire alarm active, already disarmed), with their rejection boxes underneath.
- **The required check order.** Fire alarm, then already disarmed, then lockout, then PIN. The first three reply without checking the PIN, as the project context requires.
- **Wrong-PIN branch to the right.** It leads to the "Counter = 5?" decision and the lockout start, keeping the success path in the left column.
- **One shared end node.** Every branch returns to "Return to control loop".
- **Second fire check on the success path.** Only the local path can reach it, because the remote path already rejected a fire alarm.

**What the check found:**

- A correct local PIN while already disarmed logs a DISARMED event that never happened.
- The fire-silence box must also stop an intrusion alarm and its timers, since one valid PIN silences both.
- The error beep collides with an alarm tone on the single speaker channel.
- PBKDF2 hashing inside AlarmControllerThread can block other events if the iteration count is high.

## AlarmControllerThread control loop

The control-loop flowchart shows one pass of AlarmControllerThread: wait on epoll, read one event or timer, dispatch it to one of five branches, apply the result and send commands to the other threads.

**Design choices:**

- **Straight centre column.** Wait, Read, Event type?, Network FSM, join, Apply and Send line up, so the main path reads top to bottom.
- **Five branches in one row,** in the requested order: Operational, Fire, Network, Power, Commands.
- **Loop-back on the left edge.** Drawn through helper points, because orthogonal routing ignored ports and the plain back-edge cut through the middle.
- **A start box.** "AlarmControllerThread start" was added as the entry point, as the flowchart style requires a start node.

**What the check found:**

- Several events touch two FSMs (a PIN during a fire alarm; Remote Arm and Disarm with their fire guard). The branch should read "dispatch to every FSM that handles it, Fire FSM first", not one exclusive branch.
- The retry timer belongs to NetworkMQTTThread, not to this loop.
- get\_events, fault events and single key digits have no branch.
- The epoll wait needs a timeout so the Supervisor heartbeat is sent when nothing happens.
- One eventfd read can carry several events, so the queue must be drained before waiting again.
- The shutdown sequence needs acknowledgments from Storage and Network before power-off.

## Open issues

The checks found 28 issues: 2 contradictions between documents, 4 safety or availability risks, and the rest gaps in the specification. They are listed by diagram; set Status as each is decided.

| # | Issue | Found in | Proposed fix | Status |
| --- | --- | --- | --- | --- |
| 1 | Silencing at the keypad returns to the previous armed state in the GUI document, but always leaves DISARMED in the project context | Fire FSM vs GUI doc §9 | Update the GUI document and the Figma prototype to DISARMED | Open |
| 2 | No self-loop for smoke detected while FIRE\_ALARM is active | Fire FSM | Add `EVT_SMOKE_DETECTED / ignored` to the FIRE\_ALARM loop | Open |
| 3 | "Wi-Fi loss ≤2 s" needs wpa\_supplicant or netlink monitoring that no thread has | Network FSM note | Add it to NetworkMQTTThread, or remove the claim | Open |
| 4 | EVT\_RETRY\_TIMER is not an AlarmControllerThread timer or a queued event | Network FSM, control loop | Caption the loop as internal to NetworkMQTTThread; remove it from the control loop | Open |
| 5 | "RGB LED normal" and "battery colour" each depend on the other FSM | Network and Power FSMs | Document the combined LED logic (green, yellow, blue, magenta) | Open |
| 6 | The note promises POWER\_RESTORED after a SHUTDOWN, but the boot branch has no action | Power FSM | `yes [last event = SHUTDOWN] / POWER_RESTORED alert` | Open |
| 7 | A halted Pi 4 may not restart when mains returns (HAT keeps 5 V on) | Power FSM | Test on the board; add to the points to verify | Open |
| 8 | No BATTERY\_MODE → SHUTDOWN path for a voltage sag under load | Power FSM | Add `EVT_BATTERY_CRITICAL [≤6.4 V]` from BATTERY\_MODE | Open |
| 9 | Booting on battery sends no POWER\_LOST alert | Power FSM | Decide; optionally add the alert on the "no" branch | Open |
| 10 | No event or status publish for the 10 % battery steps the app shows | Power FSM vs GUI doc | Define a status publish per 10 % step in PowerMonitorThread | Open |
| 11 | The default-PIN branch has no working database for the outbox | Start-up | Create a new empty database in that branch | Open |
| 12 | No rule for when vigil.db.bak is written; a restore can load an old PIN | Start-up | Copy the database after every PIN change; alert on restore | Open |
| 13 | Startup can block on NTP when there is no internet | Start-up | chrony with a time limit; start the application anyway | Open |
| 14 | MQ-2 warm-up drawn as a blocking step; fire detection is off during it | Start-up | Mark it as parallel; state the warm-up time in the report | Open |
| 15 | No branch for PWM driver or GPIO request failures | Start-up | Add a decision, or a sentence on degraded start with FAULT (PWM\_DRIVER) | Open |
| 16 | The watchdog is armed only when SupervisorThread starts, last | Start-up | Enable bcm2835\_wdt from boot | Open |
| 17 | The PIN lockout counter resets on reboot | Start-up, PIN flowchart | Persist counter and lockout end time, or accept in the report | Open |
| 18 | A correct local PIN while already disarmed logs a false DISARMED event | PIN flowchart | Add a "nothing active?" check on the local path | Open |
| 19 | The fire-silence box does not stop an intrusion alarm or its timers | PIN flowchart | "Silence all alarms; stop timers; start 10 min silence" | Open |
| 20 | Error beep and LED blink collide with an alarm tone on one PWM channel | PIN flowchart | Define a priority rule in OutputThread; the alarm keeps sounding | Open |
| 21 | PBKDF2 hashing can block AlarmControllerThread | PIN flowchart, control loop | Choose an iteration count in the tens of ms; constant-time compare | Open |
| 22 | Analysis slides 17 and 26 lack the lockout and ALREADY\_DISARMED | PIN flowchart | Update the analysis, or state that the design extends it | Open |
| 23 | Exclusive dispatch, but some events touch two FSMs | Control loop | Dispatch to every FSM that handles the event, Fire FSM first | Open |
| 24 | get\_events, fault events and key digits have no branch | Control loop | Add them to the Commands and Operational branches | Open |
| 25 | epoll without a timeout sends no heartbeat when the house is quiet | Control loop | 1 s timeout and a heartbeat on every pass | Open |
| 26 | One eventfd read can carry several events | Control loop | Drain the queue before waiting, or use EFD\_SEMAPHORE | Open |
| 27 | Shutdown is sent fire-and-forget before power-off | Control loop, Power FSM | Wait for Storage and Network acknowledgments | Open |
| 28 | change\_pin: lockout, counter and in-memory update rules undefined | Control loop | Decide all three; update memory only after Storage confirms | Open |

## Next actions

The two document contradictions and the hardware restart test come first, because they change what the slides say or what the board must do.

- [ ] Align the GUI document and the Figma prototype on "silence → DISARMED" (issue 1)
- [ ] Test whether the Pi restarts by itself when mains returns after a low-battery shutdown (issue 7)
- [ ] Decide the dispatch order and write it into the control-loop slide: Fire FSM first, every FSM that handles the event (issue 23)
- [ ] Add the missing arrows to the Power FSM: BATTERY\_MODE → SHUTDOWN and the POWER\_RESTORED action at boot (issues 6 and 8)
- [ ] Fix the PIN flowchart's fire-silence box and the local "already disarmed" case (issues 18 and 19)
- [ ] Define the backup rule for vigil.db.bak and the empty-database step (issues 11 and 12)
- [ ] Set the NTP timeout and the watchdog-at-boot in the Buildroot configuration (issues 13 and 16)
- [ ] Choose the PBKDF2 iteration count and measure the hash time on the Pi 4 (issue 21)
- [ ] Update analysis slides 17 and 26, or add a note that the design extends them (issue 22)
- [ ] Add this document to the project files
