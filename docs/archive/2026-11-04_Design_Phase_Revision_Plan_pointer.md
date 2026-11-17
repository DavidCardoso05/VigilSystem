> **ARCHIVED — superseded on 2026-11-09 by `VIGILSYSTEM_MASTER_CONTEXT.md` v1.0. Kept for history only. Do not implement from this document.**
>
> Original location: Claude Project: `claude/VigilSystem – Design Phase Revision Plan (pointer).md` · Document date: 4 Nov 2026 · Superseded by: Master Context §G.3, §L.
> The text below this box is an unchanged copy of the original.

---

# VigilSystem — Design Phase Revision Plan (4 Nov 2026)

Full review (slide-by-slide, 71-slide deck): https://claude.ai/artifact/8uaLyAfnLAStgT8QX7PjCo

## Critical items
- Slide 45 priorities are wrong: LocalInput/PowerMonitor must be SCHED_FIFO 50; Storage/Network/Supervisor SCHED_OTHER.
- Professor (via G9) explicitly expects: physical deployment slide, design-pattern/SOLID map, readable class diagram (+ simplified by package), thread role table, one flowchart per thread, "SQLite is a library linked into the process".
- Control-loop flowchart (51) must drain the queue and dispatch to every FSM, Fire first.
- Driver needs architecture diagram + write()/hrtimer/probe flowcharts; remove -EBUSY.
- Schematic legend colours wrong; RGB anode on 5 V with 3.3 V GPIO sinks.
- Slide 20 wrong-PIN LED override contradicts Guide BR-PIN-7.

## Open decisions (recommendation, status)
- D1 Protoboard interface board + enclosure; IR on frame, MQ-2 high on wall/ceiling (new)
- D2 NPN low-side switch per RGB cathode, active-high (new; G10 precedent)
- D3 IR output type: measure; divider if 5 V push-pull
- D4 Wrong-PIN blink dropped during alarm flashing (Guide)
- D5 Storage does all inserts; Network uses getPendingAlerts/markSent/getEventsSince (Guide) — do not write "Storage is the only accessor"
- D6 Wi-Fi loss via netlink, not wpa_supplicant (Guide)
- D7 Network FSM always starts NET_LOST (Guide)
- D8 9th keypad digit ignored + ERROR_BEEP (new)
- D9 Arm button outside DISARMED ignored + ERROR_BEEP (default G12)
- D10 Door open at arming: accepted limitation (default G9)
- D11 RGB fault blink latched until reboot (new)
- D12 MQ-2 recovery: FAULT once, re-request line every 10 s (new)
- D13 INA219 failure → new FaultComponent UPS (new; default was syslog only)
- D14/D15 MQ-2 warm-up and PBKDF2 iterations: measure
- D16 libgpiod v1.6 (Guide)
