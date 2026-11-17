> **ARCHIVED — superseded on 2026-11-09 by `VIGILSYSTEM_MASTER_CONTEXT.md` v1.0. Kept for history only. Do not implement from this document.**
>
> Original location: Claude Project: `VigilSystem_Hardware_Context.md` · Document date: 31 Oct 2026 · Superseded by: `docs/hardware/HARDWARE_CONTEXT.md` v2.0.
> The text below this box is an unchanged copy of the original.

---

# VigilSystem — Hardware Context (Design Phase)

**Group 8** · David Cardoso (PG63873) · Ana Sofia Cunha (A107017)
MEEIC — Universidade do Minho · Embedded Systems (SEA) / Real-Time Systems (STR)

This document records the final hardware decisions of VigilSystem after the design-phase review. It replaces the hardware sections of earlier project context documents.

---

## 1. Decisions taken in this review

| Topic | Decision |
|---|---|
| Intrusion sensor | An **infrared reflective proximity sensor** (not a true PIR) is used to detect door/window opening. In the presentation it keeps the name **"PIR"** / "HC-SR501"; the exact model is still to be confirmed. |
| Sound output | A **FoneStar EP-36 miniature speaker (8 Ω, 0.3 W)** is used. In the presentation it keeps the name **"Buzzer Module"**. |
| Speaker driver | NPN **2N2222A** (or BC337), 470 Ω base resistor, 33 Ω / 0.5 W series resistor, 1N4148 flyback diode, 10 kΩ base pull-down. |
| LED resistors | Red LED 220 Ω (≈ 6 mA); RGB LED 390 Ω / 220 Ω / 220 Ω (≈ 8 mA per channel). |
| MQ-2 level shifting | 10 kΩ / 20 kΩ divider (kept; **not** 10 k / 15 k). Optional Schottky alternative. |
| RTC | **Not used.** Time comes from NTP (chrony) plus a saved-date script. |
| MicroSD | Mentioned only as part of the Raspberry Pi specification. |
| Mains-loss detection | Not described in the presentation, but implemented in software from the INA219 readings (see §4.1). |
| INA219 | Measures voltage, current and power only; battery level is **estimated in software**. |

---

## 2. Component overview

| # | Component | Role | Requirement served |
|---|---|---|---|
| 1 | Raspberry Pi 4 Model B | Central processing unit | Technical constraint |
| 2 | MicroSD card (16/32 GB) | Buildroot image + SQLite database | Event Logging, Pending Notifications |
| 3 | UPS HAT (Botnroll Ref. 4106) | Backup power, power monitoring (INA219) | Power Resilience, Availability |
| 4 | 2× 18650 Li-ion (Botnroll Ref. BAT03005) | UPS battery pack (2S) | Power Resilience |
| 5 | AC/DC adapter 8.4 V DC | Mains supply and battery charging | Power Resilience |
| 6 | Keypad 3×4 (Botnroll Ref. 434) | Local PIN entry | Flexible Control |
| 7 | Push-button (Ref. 716 PCB / Ref. 1188 panel) | Local arming | Flexible Control |
| 8 | "PIR" sensor ×2 (IR reflective proximity) | Door/window opening detection | Dual Detection |
| 9 | MQ-2 gas/smoke module (Botnroll Ref. 939) | Fire/gas detection, 24/7 | Dual Detection |
| 10 | "Buzzer Module" (FoneStar EP-36 speaker) + NPN driver | Distinct alarm tones | Distinct Alarms |
| 11 | Red LED 5 mm diffused (Ref. 2629) | Armed / alarm / error indication | Visual Status |
| 12 | RGB LED, common anode (CEL03119, piranha) | Power source + network status | Visual Status, Connectivity Monitoring |

---

## 3. Raspberry Pi 4 Model B

- Broadcom BCM2711, quad-core Cortex-A72 (ARMv8, 64-bit), 1.5–1.8 GHz.
- GPIO logic **3.3 V, not 5 V tolerant**; default drive 8 mA per pin (configurable 2–16 mA); aggregate GPIO current kept far below ≈ 50 mA.
- Internal pull-up/pull-down ≈ 50 kΩ.
- Hardware PWM: 2 channels — PWM0 (GPIO12/18), PWM1 (GPIO13/19).
- Wi-Fi 802.11b/g/n/ac (CYW43455, `brcmfmac`); Gigabit Ethernet.
- Supplied with 5 V **through the GPIO header by the UPS HAT**. The Pi must never be powered from USB-C and from the HAT at the same time.
- No hardware real-time clock.
- GPIO0/1 reserved (HAT EEPROM); GPIO14/15 kept free for the serial debug console.

**MicroSD card:** 16/32 GB, Class 10 / A1, high endurance preferred (frequent SQLite writes). Holds the Buildroot image (FAT32 boot + ext4 rootfs) and the local database (`PIN_CONFIG`, `EVENT_LOG`, `PENDING_NOTIFICATION`).

---

## 4. Power: UPS HAT, batteries and adapter

### UPS HAT (Botnroll Ref. 4106)

| Parameter | Value |
|---|---|
| Compatibility | Raspberry Pi 4B / 3B+ / 3B (40-pin) |
| Batteries | 2× 18650 Li-ion in series (2S: 6.0–8.4 V), not included |
| Output | 5 V regulated, up to 2.5 A continuous, via the GPIO header |
| Extra output | 1× USB-A 5 V |
| Monitoring | I²C, INA219 at address `0x42`: bus voltage, current, power |
| Protections | Over-charge, over-discharge, over-current, short-circuit, reverse polarity; balanced charging |

### Batteries (Botnroll Ref. BAT03005, MR18650)

- Li-ion 18650, 2 cells in series (2S).
- Nominal 3.6–3.7 V per cell (7.2–7.4 V pack); full charge 4.2 V per cell (8.4 V pack).
- Capacity 2600 mAh; optimal continuous discharge 1.3 A (0.5C), peak 7.8 A (3C).
- Use two identical cells; switch the HAT **OFF** before inserting them.

### AC/DC adapter

Wall charger, 230 V AC → 8.4 V DC. Powers the system and charges the batteries.

### 4.1 Software notes on power (not in the slides)

- The HAT has **no dedicated "mains lost" signal**. `PowerMonitorThread` derives `Power Grid Failure` / `Power Restoration` from the INA219 current sign (discharging = mains lost), with hysteresis and debounce.
  - Proposed: poll every 1 s; current < −100 mA for 3 samples → mains lost; > +50 mA for 3 samples → restored. Thresholds to validate on the board.
- Battery level is estimated from the pack voltage (vendor mapping 6.0 V → 0 %, 8.4 V → 100 %, approximate for Li-ion).
- Low-battery safe shutdown at ≈ 6.4 V (3.2 V per cell) to protect the SD card.
- Access through `/dev/i2c-1` (`i2c-dev`), `dtparam=i2c_arm=on`.

---

## 5. Keypad 3×4 (Botnroll Ref. 434)

- 12 keys (0–9, `*`, `#`), passive membrane matrix, 7 pins (4 rows + 3 columns).
- 3.3 V GPIO level, no level shifting.
- Rows: inputs with internal pull-up (idle HIGH). Columns: driven LOW one at a time.
- Scan every ≈ 10–20 ms; debounce with 2 consecutive stable reads.
- Optional 330 Ω series resistor per line.

---

## 6. Push-button (arm button)

- Momentary SPST-NO; Ref. 716 (6×6 mm, breadboard) for prototyping, Ref. 1188 (Ø12 mm panel) for the enclosure.
- Between GPIO6 and GND; internal pull-up; **active-low**.
- Falling-edge detection, software debounce ≈ 20–50 ms.
- Optional 330 Ω–1 kΩ series resistor for protection.

---

## 7. Intrusion sensor ("PIR" in the presentation)

**Actual part:** infrared reflective (diffuse) proximity sensor — square red/black housing with cable. It detects an object directly in front of its narrow beam, so it is mounted on the frame pointing at the door/window leaf:

- Door/window closed → leaf reflects the beam → output **LOW**.
- Door/window open → no reflection → output **HIGH** → intrusion event (entry delay starts).

| Parameter | Value |
|---|---|
| Supply | 5 V |
| Output | Digital, **active-low** (LOW when an object is in range); likely NPN open-collector, 100 mA |
| Consumption | < 25 mA |
| Response time | < 2 ms |
| Beam angle | ≤ 15° |
| Quantity | 2 (GPIO16, GPIO20) |

**Naming decision:** the presentation keeps "PIR" / "HC-SR501". The exact model of the sensor is still to be confirmed.

**To verify before wiring:** measure the output with nothing connected.
- Floating / near 0 V → open-collector → use the internal pull-up to 3.3 V (as configured in the GPIO table).
- Near 5 V → internal pull-up to 5 V → needs a 5 V → 3.3 V divider, like the MQ-2.

Limitations: needs alignment; dark or glossy surfaces and strong sunlight can affect it; draws power continuously (relevant on battery).

---

## 8. MQ-2 gas/smoke sensor (Botnroll Ref. 939)

| Parameter | Value |
|---|---|
| Principle | Heated SnO₂ metal-oxide sensor |
| Detects | LPG, butane, propane, methane, hydrogen, alcohol, smoke |
| Range | ≈ 300–10 000 ppm combustible gases (typ.) |
| Supply | 5 V (4.5–5.5 V) |
| Current | ≈ 120–150 mA (heater) |
| Outputs | AO (not used); DO from LM393 comparator, 5 V TTL, **active-low** (LOW = gas detected) |
| Operating range | −20 to 50 °C |

**Level shifting (mandatory):** DO → 10 kΩ → GPIO21, with 20 kΩ from GPIO21 to GND.

- The 4-pin (Flying-Fish style) module has an on-board pull-up (typically 10 kΩ) on DO, in series with the divider: the HIGH level is ≈ 5 × 20 / (10 + 10 + 20) ≈ **2.5 V** (≈ 3.3 V if there is no on-board pull-up). Both are valid HIGH levels.
- Do **not** use 10 kΩ / 15 kΩ: with the on-board pull-up, HIGH drops to ≈ 2.14 V, too close to the Pi's input threshold.
- **Measure** GPIO21 with the divider connected (no gas): ≥ ≈ 2.5 V is fine.
- Optional alternative: Schottky diode (1N5817 / 1N5819 / BAT85 / BAT43), cathode to DO, anode to GPIO21, with the Pi's internal pull-up. A 1N4148 is **not** suitable (LOW ≈ 0.8 V).

Software notes: inhibit alarms during warm-up; require N consecutive positive reads; also reacts to alcohol vapours. The MQ-2 is not a certified smoke detector (EN 14604) — educational prototype.

---

## 9. "Buzzer Module" (FoneStar EP-36 speaker)

| Parameter | Value |
|---|---|
| Type | Miniature dynamic speaker, Ø36 × 4 mm, alnico magnet, mylar cone |
| Impedance | 8 Ω |
| Rated power | 0.3 W |
| Control | PWM on GPIO18 (PWM0); tone set by the PWM frequency |

**Naming decision:** the presentation calls it "Buzzer Module".

### 9.1 Driver circuit

- GPIO18 → **470 Ω** → base of **2N2222A** (or BC337); **10 kΩ** base-to-GND pull-down (silent during boot).
- Emitter → GND.
- +5 V → speaker → **33 Ω / 0.5 W** → collector.
- **1N4148** flyback diode across the speaker (required — inductive voice coil).

Calculations:
- Peak current: 5 / (33 + 8) ≈ 122 mA (BC547, 100 mA max, is not suitable).
- Speaker power at 50 % duty: ≈ 0.06 W (rating 0.3 W). Resistor ≈ 0.25 W → 0.5 W part.
- Base current: (3.3 − 0.8) / 470 ≈ 5.3 mA (below the 8 mA GPIO drive).
- Louder option: 22 Ω (≈ 0.11 W in the speaker), resistor rated 1 W.

**Check the transistor pinout** of the exact part bought: PN2222A = E-B-C; P2N2222A (onsemi) = C-B-E; BC337 = C-B-E; metal-can 2N2222A differs.

**Never leave GPIO18 stuck HIGH** — constant DC heats the coil and resistor. "Stop tone" must leave the pin LOW. Optional: 100 µF in series with the speaker to block DC.

### 9.2 Tone plan (to tune by ear)

| Event | Pattern (proposal) |
|---|---|
| Key press | 50 ms at 2 kHz |
| Error (wrong PIN) | 2 × 150 ms at ≈ 600–800 Hz (low tones are weak on a 36 mm speaker) |
| Entry/exit warning | 100 ms at 1 kHz, every 1 s |
| Intrusion alarm | Alternating 800/1200 Hz, 250 ms each |
| Fire alarm | Alternating 2 kHz/3 kHz, 150 ms each |

---

## 10. Red LED (Ref. 2629)

- 5 mm diffused red, V_F 1.8–2.4 V.
- GPIO13 (PWM1) → **220 Ω** → anode; cathode → GND.
- Operating current ≈ 6 mA (4.1–6.8 mA across V_F tolerance); 20 mA is only the LED's maximum rating.
- States: OFF = disarmed; steady ON = armed; flashing = alarm; blinking = wrong PIN.

---

## 11. RGB LED (CEL03119, piranha, common anode)

| Parameter | Value |
|---|---|
| Common terminal | Common anode |
| V_F | Red 1.8–2.4 V; Green 2.8–3.6 V; Blue 2.8–3.6 V |
| Rated current | 20 mA per channel (maximum) |
| Operating current | ≈ 8 mA per channel |

**Interface:** common anode → **+5 V** (UPS-backed rail); each cathode → resistor → GPIO; **GPIO LOW = channel on** (GPIO sinks the current). Driving from 5 V is needed because the green/blue V_F is close to 3.3 V.

Resistors: **390 Ω (R), 220 Ω (G), 220 Ω (B)**.

| Colour | Power source | Network (broker) |
|---|---|---|
| Green | Mains | OK |
| Yellow (R+G) | Mains | Lost |
| Blue | Battery | OK |
| Magenta (R+B) | Battery | Lost |

Notes: pure red is reserved for the armed/alarm LED. With the GPIO HIGH (3.3 V), only 1.7 V is across the red die (V_F min 1.8 V) — if it glows faintly, add a 1N4148 in series with the red channel. GPIO19/26 default to pull-down at boot, so green/blue may glow very faintly until the software drives them HIGH.

---

## 12. GPIO allocation (final)

| GPIO | Header pin | Description | Mode |
|---|---|---|---|
| 17 | 11 | Keypad row 1 | Input, pull-up |
| 27 | 13 | Keypad row 2 | Input, pull-up |
| 22 | 15 | Keypad row 3 | Input, pull-up |
| 23 | 16 | Keypad row 4 | Input, pull-up |
| 24 | 18 | Keypad column 1 | Output (scan) |
| 25 | 22 | Keypad column 2 | Output (scan) |
| 5 | 29 | Keypad column 3 | Output (scan) |
| 6 | 31 | Arm button | Input, pull-up, active-low |
| 16 | 36 | "PIR" sensor 1 (door/window) | Input, pull-up (to confirm), active-low |
| 20 | 38 | "PIR" sensor 2 (door/window) | Input, pull-up (to confirm), active-low |
| 21 | 40 | MQ-2 DO (via 10 k/20 k divider) | Input, active-low |
| 13 | 33 | Red LED (PWM1) | PWM output (ALT0) |
| 4 | 7 | RGB LED red channel | Output, active-low (sink) |
| 26 | 37 | RGB LED green channel | Output, active-low (sink) |
| 19 | 35 | RGB LED blue channel | Output, active-low (sink) |
| 18 | 12 | Buzzer Module (PWM0) | PWM output (ALT5) |
| 2 | 3 | I²C-1 SDA — UPS HAT (`0x42`) | I²C (ALT0) |
| 3 | 5 | I²C-1 SCL — UPS HAT | I²C (ALT0) |

Power: 5 V on pins 2/4 (from the UPS HAT), GND on pin 6. Free for expansion: GPIO7–GPIO12. GPIO14/15 kept for the serial console.

**PWM overlay:** the default `dtoverlay=pwm-2chan` maps PWM1 to GPIO19 (the RGB blue channel). Use:

```
dtoverlay=pwm-2chan,pin=18,func=2,pin2=13,func2=4
```

or have the custom PWM driver configure the pin multiplexing itself.

---

## 13. Power budget (5 V rail, UPS HAT limit 2.5 A)

| Consumer | Typical | Maximum |
|---|---|---|
| Raspberry Pi 4B (Wi-Fi, no USB devices) | ≈ 0.60 A | ≈ 1.30 A |
| MQ-2 heater | ≈ 0.12 A | ≈ 0.15 A |
| PIR sensors (×2) | ≈ 0.04 A | ≈ 0.05 A |
| LEDs (red + RGB) | ≈ 0.014 A | ≈ 0.022 A |
| Buzzer (only while sounding) | — | ≈ 0.12 A |
| **Total** | **≈ 0.77 A** | **≈ 1.64 A** |
| **UPS HAT limit** | | **2.5 A continuous** |

Maximum load stays below the UPS HAT limit, with ≈ 0.86 A of margin.

Estimated autonomy: 2 × 18650 cells ≈ 19.2 Wh (≈ 16 Wh usable at 85 % efficiency) → ≈ 4 h at typical load, ≈ 2 h at maximum load (to be measured during testing).

---

## 14. Timekeeping (no RTC)

- The Pi 4 has no battery-backed clock. Time is set by **NTP (chrony)** at boot; while powered (including on UPS battery) the kernel keeps time with small drift, corrected by chrony.
- A script saves the date periodically and restores it at boot.
- Accepted limitation: after a full battery depletion, if the Pi boots without internet, timestamps are inaccurate until NTP syncs. Event order is still correct because `EVENT_LOG` is ordered by its auto-increment `id`.
- NTP must sync before the TLS handshake, otherwise the broker certificate appears invalid.

---

## 15. Custom circuits (breadboard / protoboard)

| Circuit | Components |
|---|---|
| Arm button | Internal pull-up (optional 330 Ω series) |
| Keypad line protection (optional) | 330 Ω × 7 |
| "PIR" sensor outputs | Internal pull-up if open-collector; divider if 5 V output (to verify) |
| MQ-2 level shifting | 10 kΩ + 20 kΩ divider (or Schottky + internal pull-up) |
| Red LED | 220 Ω |
| RGB LED (5 V, common anode) | 390 Ω (R) + 220 Ω (G) + 220 Ω (B) |
| Speaker driver | 2N2222A (or BC337) + 470 Ω base + 10 kΩ pull-down + 33 Ω / 0.5 W series + 1N4148 flyback |

---

## 16. Bill of materials

### Purchased / available

| Item | Qty |
|---|---|
| Raspberry Pi 4 Model B | 1 |
| UPS HAT (Ref. 4106, incl. 8.4 V charger) | 1 |
| 18650 Li-ion 2600 mAh (Ref. BAT03005) | 2 |
| Keypad 3×4 (Ref. 434) | 1 |
| Push-button PCB (Ref. 716) / panel (Ref. 1188) | 1 + 1 |
| MQ-2 gas sensor brick (Ref. 939) | 1 |
| IR proximity sensor ("PIR") | 2 |
| FoneStar EP-36 speaker | 1 |
| Red LED 5 mm (Ref. 2629) | 1 |
| RGB LED common anode (CEL03119) | 1 |

### Still to purchase

| Item | Qty | Notes |
|---|---|---|
| MicroSD card 16/32 GB | 1 | Class 10 / A1, high endurance |
| NPN 2N2222A or BC337 | 2–3 | Spares; check pinout |
| Diode 1N4148 | 1–2 | Speaker flyback (+ optional RGB red) |
| Resistors 220 Ω, 330 Ω, 390 Ω, 470 Ω, 10 kΩ, 20 kΩ | assorted | |
| Resistor 33 Ω, 0.5 W | 1 | Speaker series resistor |
| Schottky 1N5817/1N5819 (optional) | 1 | MQ-2 alternative interface |
| Breadboard + Dupont jumpers | 1 set | |
| Heatsinks for the Pi | 1 set | The HAT covers the board |

### Rejected

| Item | Reason |
|---|---|
| Self-flashing red LED | Cannot be held steady; V_min > 3.3 V |
| RGB LED common cathode (CEL03030) | Needs high-side switching (PNP + NPN per channel) |
| ADS1115 ADC | Not needed — MQ-2 DO is used |
| BC547 for the speaker | 100 mA max, below the ≈ 122 mA peak |
| RTC DS3231 | Decided not to use; NTP + saved-date script |

---

## 17. Points to verify on delivery

1. UPS HAT answers at `0x42` (`i2cdetect -y 1`), and the **sign** of the INA219 current when the charger is unplugged.
2. "PIR" sensor: exact model, output type (open-collector vs 5 V), detection distance against the door/window.
3. MQ-2: voltage on GPIO21 with the divider connected (≥ ≈ 2.5 V HIGH).
4. Transistor pinout of the purchased part.
5. RGB LED red channel fully off at GPIO HIGH; piranha leads seat on the breadboard.
6. Both PWM channels on GPIO18 and GPIO13 with the overlay above, without affecting GPIO19.
7. Speaker volume with 33 Ω; GPIO18 left LOW when silent.
8. 18650 cells fit the holder; HAT switch OFF when inserting.
9. Measured battery autonomy.

## 18. Open points

- MQ-2 DO threshold (on-board trimmer, against a controlled smoke source).
- INA219 thresholds and debounce for mains-loss detection.
- Tone frequencies, tuned by ear.
- Driver strategy for the software design: which devices use custom kernel modules (at least the PWM driver for the speaker and red LED) and which use mainline interfaces (`libgpiod`, `i2c-dev`).
