# VigilSystem — Hardware Specification

| Field | Value |
|---|---|
| Document | `docs/hardware/HARDWARE_CONTEXT.md` |
| Version | **2.0** — 2026-11-09 |
| Replaces | `VigilSystem_Hardware_Context.md` (31 Oct 2026), archived in `docs/archive/` |
| Derived from | `VIGILSYSTEM_MASTER_CONTEXT.md` v1.0, §D (DEC-001, DEC-005a/b, DEC-007, DEC-010, DEC-012) |
| Authority | The Master Context wins if this document disagrees with it. Report the difference; do not resolve it silently. |

Group 8 · David Cardoso (PG63873) · Ana Sofia Cunha (A107017) · MEEIC, Universidade do Minho · SEA + STR

**Validation status:** nothing has been assembled or measured yet. Every connection below is approved design intent, not verified hardware (§17).

---

## 1. What changed from version 1 (31 Oct)

| Topic | Version 1 | Version 2 | Decision |
|---|---|---|---|
| Intrusion sensor name | Called "PIR" / "HC-SR501" in the slides | **IR reflective sensor** (IR1, IR2). It detects the **movement of a monitored door or window**. "PIR" is no longer used anywhere. | DEC-010 (A') |
| IR interface | Internal pull-up or divider, "to verify" | **Schottky diode + external 10 kΩ pull-up to 3.3 V**, GPIO bias disabled. Works for open-collector and 5 V push-pull outputs | DEC-007 (7.2 b) |
| RGB LED drive | Anode at 5 V, cathodes sunk directly by 3.3 V GPIOs, active-low | **One NPN low-side switch per cathode**; GPIO HIGH = on; no 5 V node touches a GPIO | DEC-007 (7.1 b) |
| Physical build | "Breadboard / protoboard" | Breadboard for bring-up; protoboard interface board in a **commercial plastic box bought in a local shop** | DEC-007 (7.3 b) |
| Battery | Level estimated as a percentage; shutdown at ≈ 6.4 V | **No battery percentage.** LOW_BATTERY at ≤ 6.7 V; **safe state** at ≤ 6.4 V (no power-off) | DEC-001, DEC-005a/b, DEC-012 |
| `config.txt` IR bias | `gpio=16,20=ip,pu` | `gpio=16,20=ip,pn` (the external pull-up replaces the internal one) | Consequence of DEC-007 |
| Purchases | Most parts listed as "purchased" | Only the Pi, microSD, breadboard, red LED, RGB LED, speaker and the two IR sensors are owned; the rest is ordered in the week of 9 Nov | Status update (9 Nov) |

The speaker driver, LED resistors, MQ-2 divider, GPIO numbers and "no RTC" decisions of version 1 are unchanged.

---

## 2. Components and availability (9 Nov 2026)

| # | Component | Reference | Role | Availability |
|---|---|---|---|---|
| 1 | Raspberry Pi 4 Model B | — | Central processing unit | **Owned** |
| 2 | microSD 16/32 GB, Class 10 / A1 | — | Buildroot image + SQLite database | **Owned** |
| 3 | Breadboard + jumpers | — | Bring-up | **Owned** |
| 4 | Red LED 5 mm diffused | Botnroll 2629 | Armed / alarm indicator | **Owned** |
| 5 | RGB LED, common anode, piranha | CEL03119 **[part number unverified]** | Power source + broker status | **Owned** |
| 6 | Speaker FoneStar EP-36, 8 Ω, 0.3 W | — | Distinct tones | **Owned** |
| 7 | IR reflective proximity sensor ×2 | Model to confirm | Door/window movement (IR1, IR2) | **Owned** |
| 8 | UPS HAT with 8.4 V charger | Botnroll 4106 | Backup power; INA219 at `0x42` | To order |
| 9 | 2× Li-ion 18650, 2600 mAh (2S) | Botnroll BAT03005 | UPS battery | To order |
| 10 | Keypad 3×4 | Botnroll 434 | PIN entry | To order |
| 11 | Push-button (breadboard + panel) | Botnroll 716 + 1188 | Local arming | To order |
| 12 | MQ-2 gas sensor brick | Botnroll 939 | Smoke/gas, 24/7 | To order |
| 13 | Passives, transistors, protoboard | §16 | Interface circuits | To order |
| 14 | Commercial plastic enclosure | Local shop | Housing | To buy (check dimensions) |

The purchases are on the critical path of every hardware test.

---

## 3. Raspberry Pi 4 Model B

- Broadcom BCM2711, quad-core Cortex-A72 (ARMv8, 64-bit), 1.5–1.8 GHz.
- GPIO logic **3.3 V, not 5 V tolerant**; default drive 8 mA per pin; internal pull-up/pull-down ≈ 50 kΩ.
- Hardware PWM: PWM0 on GPIO18 (speaker), PWM1 on GPIO13 (red LED), both owned by the custom driver `vigil_pwm`. Analogue audio stays disabled because it uses the PWM block.
- Wi-Fi 802.11b/g/n/ac (CYW43455, `brcmfmac`).
- Supplied with 5 V **through the GPIO header by the UPS HAT**. Never power it from USB-C and the HAT at the same time.
- No hardware real-time clock (§14).
- Reserved: GPIO0/1 (HAT EEPROM), GPIO14/15 (serial console).

**microSD:** 16/32 GB, Class 10 / A1, high endurance preferred. Holds the Buildroot image (FAT32 boot + ext4 rootfs) and the SQLite database with two tables, `EVENT_LOG` and `PIN_CONFIG` (Master Context §E.5).

---

## 4. Power: UPS HAT, batteries and adapter

### 4.1 Ratings

| Item | Value |
|---|---|
| UPS HAT (Botnroll 4106) | 5 V regulated to the Pi through the header, up to 2.5 A continuous; 1× USB-A 5 V; protections against over-charge, over-discharge, over-current, short circuit and reverse polarity; balanced charging |
| Batteries (BAT03005, MR18650) | 2 cells in series (2S): 6.0 V empty, 7.2–7.4 V nominal, 8.4 V full; 2600 mAh; optimal continuous discharge 1.3 A (0.5C), peak 7.8 A (3C). Use two identical cells; switch the HAT **off** before inserting them |
| Adapter | 230 V AC → 8.4 V DC; powers the system and charges the batteries |

### 4.2 INA219 power monitor

- Access through `/dev/i2c-1` (`i2c-dev`), address `0x42`, big-endian registers; `dtparam=i2c_arm=on`.
- Calibration `0x05 = 4096`; configuration `0x00 = 0x3EEF`.
- Bus voltage = `(reg 0x02 >> 3) × 4 mV`; current = signed `reg 0x04 × 0.1 mA`.
- It measures voltage, current and power only.

### 4.3 How the software uses it

- **Mains detection** (the HAT has no "mains lost" signal): current < −100 mA for 3 consecutive 1 s samples → mains lost; > +50 mA for 3 samples → mains back; near 0 mA keeps the previous state. **[Unverified: the sign of the current and the thresholds are checked on the board.]**
- **Initial power state:** the first 3 samples decide silently; mains is assumed until then (DEC-005b).
- **LOW_BATTERY:** ≤ 6.7 V for 3 samples, only on battery, once per outage (DEC-005a). No battery percentage is computed, stored, published or shown (DEC-001).
- **Safe state:** ≤ 6.4 V for 3 samples → SHUTDOWN alert, database closed and synced, the system keeps running until the HAT cuts its output; there is **no power-off** (DEC-005b, 5b.3). If mains returns while in the safe state, the application performs a controlled reboot (DEC-012, 12.2).
- **Condition of the safe-state decision:** the HAT must cut its output at full discharge and restore it when the charger returns. This is tested first when the HAT arrives (§17, check 2); if it fails, the decision is reopened.

---

## 5. Keypad 3×4 (Botnroll 434)

- 12 keys (`0–9`, `*`, `#`), passive membrane matrix, 7 pins (4 rows + 3 columns), 3.3 V, no level shifting.
- Rows: inputs with internal pull-up (idle HIGH). Columns: open-drain outputs, one driven LOW at a time (released = high-Z), so two pressed keys cannot short a HIGH column to a LOW one.
- Scan every 15 ms; a key is accepted after 2 stable scans.
- Key map `[row][col]`: `1 2 3 / 4 5 6 / 7 8 9 / * 0 #`.
- Verify the connector pin order with a continuity tester before wiring.

---

## 6. Arm button

- Momentary SPST-NO: Ref. 716 (6×6 mm) on the breadboard, Ref. 1188 (Ø12 mm) on the enclosure panel.
- Between GPIO6 and GND; internal pull-up; **active-low** (LOW = pressed).
- Debounce: 2 stable reads at 15 ms (same `DebouncedInput` class as the sensors).

---

## 7. IR reflective sensors (door/window movement)

**Function (DEC-010):** each sensor is mounted on the frame of a door or window, with its narrow beam pointing at the leaf. It detects the **movement of the monitored door or window**:

- closed → the leaf reflects the beam → output LOW;
- opened → no reflection → output HIGH → intrusion event (the entry delay starts when the system is armed).

| Parameter | Value |
|---|---|
| Supply | 5 V |
| Output | Digital, LOW when an object is in range; output stage (open-collector or 5 V push-pull) to be measured |
| Consumption | < 25 mA each |
| Response time | < 2 ms |
| Beam angle | ≤ 15° |
| Quantity | 2 — IR1 on GPIO16, IR2 on GPIO20 |

**Interface (DEC-007, 7.2 b):**

```
sensor OUT ──|◄── GPIO16 / GPIO20          Schottky: cathode to OUT, anode to GPIO
                 │                         (BAT85 preferred for low leakage; 1N5817 acceptable)
                10 kΩ
                 │
               3.3 V                       GPIO bias disabled (external pull-up)
```

- Open-collector or push-pull LOW → the diode conducts → GPIO ≈ 0.3–0.5 V (below V_IL max 0.8 V).
- Output HIGH (even 5 V) → the diode blocks → the 10 kΩ pulls the GPIO to 3.3 V. 5 V never reaches the GPIO.

**Limitations:** needs alignment; dark or glossy surfaces and strong sunlight can affect it; it draws power continuously (relevant on battery). Only the two instrumented doors/windows are protected; a door already open when the system arms is detected only after it closes and opens again.

---

## 8. MQ-2 gas/smoke sensor (Botnroll 939)

| Parameter | Value |
|---|---|
| Principle | Heated SnO₂ metal-oxide sensor |
| Detects | LPG, butane, propane, methane, hydrogen, alcohol, smoke |
| Supply | 5 V (4.5–5.5 V); heater ≈ 120–150 mA |
| Outputs | AO not used; DO from an LM393 comparator, 5 V TTL, **active-low** (LOW = gas detected) |
| Warm-up | 60 s default, to be measured; no smoke events during the warm-up |

**Level shifting (mandatory):** DO → 10 kΩ → GPIO21, with 20 kΩ from GPIO21 to GND; GPIO bias **disabled** (the internal ≈ 50 kΩ pull-down would lower the HIGH level).

- With the module's on-board pull-up (typically 10 kΩ) in series, HIGH ≈ 5 × 20 / (10 + 10 + 20) ≈ 2.5 V (≈ 3.3 V without it). Both are valid HIGH levels.
- Do **not** use 10 kΩ / 15 kΩ: HIGH would drop to ≈ 2.1 V.
- Measure GPIO21 with the divider connected and no gas: ≥ ≈ 2.5 V is fine.

The MQ-2 also reacts to alcohol vapours and is not a certified smoke detector (EN 14604); this is an educational prototype.

---

## 9. Speaker (FoneStar EP-36)

| Parameter | Value |
|---|---|
| Type | Miniature dynamic speaker, Ø36 × 4 mm |
| Impedance / rated power | 8 Ω / 0.3 W |
| Control | PWM0 on GPIO18 through `/dev/vigil_pwm`; the driver plays step sequences (≤ 8 steps; 0 steps = stop) |

### 9.1 Driver circuit

- GPIO18 → **470 Ω** → base of **BC337 or 2N2222A**; **10 kΩ** base-to-GND (silent during boot).
- Emitter → GND; +5 V → speaker → **33 Ω / 0.5 W** → collector.
- **1N4148** flyback diode across the speaker (required: inductive voice coil).

Calculations: peak current 5 / (33 + 8) ≈ 122 mA (a BC547, 100 mA max, is not suitable); speaker power at 50 % duty ≈ 0.06 W; base current (3.3 − 0.8) / 470 ≈ 5.3 mA.

- Check the transistor pinout of the exact part bought (PN2222A = E-B-C; P2N2222A and BC337 = C-B-E; the metal-can 2N2222A differs).
- **Duty ≤ 50 %; GPIO18 LOW when silent.** A pin stuck HIGH heats the coil and the resistor.

### 9.2 Tone plan (proposal, tuned by ear)

| Event | Pattern |
|---|---|
| Key press | 50 ms at 2 kHz |
| Error (wrong PIN) | 2 × 150 ms at ≈ 600–800 Hz |
| Entry warning | 100 ms at 1 kHz every 1 s |
| Intrusion alarm | Alternating 800 / 1200 Hz, 250 ms each |
| Fire alarm | Alternating 2 / 3 kHz, 150 ms each |

Low tones are weak on a 36 mm speaker.

---

## 10. Red LED (Botnroll 2629)

- 5 mm diffused red, V_F 1.8–2.4 V.
- GPIO13 (PWM1, through `/dev/vigil_pwm`) → **220 Ω** → anode; cathode → GND; ≈ 6 mA.
- States: off = disarmed; on = exit delay, armed, entry delay; flashing = any alarm; short blink = wrong PIN (dropped while flashing).

---

## 11. RGB LED (common anode, piranha)

| Parameter | Value |
|---|---|
| Common terminal | Common anode at **+5 V** (UPS-backed rail) |
| V_F | Red 1.8–2.4 V; green and blue 2.8–3.6 V |
| Operating current | ≈ 7–8 mA per channel (20 mA maximum) |

**Interface (DEC-007, 7.1 b):** each cathode → channel resistor (**390 Ω R, 220 Ω G, 220 Ω B**) → collector of an NPN (BC337 or 2N2222A), emitter to GND. GPIO → **1 kΩ** → base, **10 kΩ** base-to-GND. Base current ≈ 2.6 mA per GPIO. **GPIO HIGH = channel on.** No 5 V node is connected to a GPIO, and the LED stays off while the pins float at boot.

| Colour | Power source | Broker connection |
|---|---|---|
| Green | Mains | Connected |
| Yellow (R+G) | Mains | Lost |
| Blue | Battery | Connected |
| Magenta (R+B) | Battery | Lost |

Pure red is reserved for the red LED. There is no fault blink (DEC-001).

---

## 12. GPIO allocation

| BCM | Header pin | Function | Configuration | Active level |
|---|---|---|---|---|
| 17, 27, 22, 23 | 11, 13, 15, 16 | Keypad rows 1–4 | Input, internal pull-up | LOW = key in the scanned column |
| 24, 25, 5 | 18, 22, 29 | Keypad columns 1–3 | Output, open-drain, scanned | Driven LOW when scanned |
| 6 | 31 | Arm button | Input, internal pull-up | LOW = pressed |
| 16 | 36 | IR1 (door/window) | Input, bias disabled; external 10 kΩ pull-up + Schottky | LOW = closed, HIGH = opened |
| 20 | 38 | IR2 (door/window) | Same as IR1 | Same |
| 21 | 40 | MQ-2 DO via 10 kΩ / 20 kΩ divider | Input, bias disabled | LOW = smoke/gas |
| 13 | 33 | Red LED, PWM1 (ALT0) | Owned by `vigil_pwm` | HIGH = on |
| 18 | 12 | Speaker, PWM0 (ALT5) | Owned by `vigil_pwm` | LOW when silent |
| 4, 26, 19 | 7, 37, 35 | RGB red, green, blue → NPN base | Output | **HIGH = on** |
| 2, 3 | 3, 5 | I²C-1 SDA / SCL → INA219 `0x42` | I²C | — |

Power: 5 V on header pins 2/4 (from the UPS HAT), GND on pin 6. Free for expansion: GPIO7–12.

**Obsolete pin maps — never use:** red LED on GPIO26; RGB on GPIO4/13/19; "PIR" on GPIO16/20 (old Project instructions).

---

## 13. Boot-safe levels (`config.txt`)

The custom overlay `vigil-pwm` claims GPIO18 and GPIO13; never load `pwm-2chan` as well. Keep analogue audio off.

```
dtparam=i2c_arm=on
dtoverlay=vigil-pwm
# Safe levels before Linux starts (firmware gpio= syntax: ip/op, pu/pd/pn, dh/dl)
# keypad rows and arm button: input + pull-up
gpio=17,22,23,27=ip,pu
gpio=6=ip,pu
# IR sensors: input, no pull (external 10 kOhm pull-up + Schottky, DEC-007)
gpio=16,20=ip,pn
# MQ-2 DO through the divider: no pull
gpio=21=ip,pn
# RGB channels drive NPN low-side switches: LOW = transistor off = LED off
gpio=4,19,26=op,dl
# red LED and speaker LOW until the PWM driver takes the pins
gpio=13,18=op,dl
```

The application also sets the bias through libgpiod; these lines are the safety net before Linux starts.

---

## 14. Power budget and timekeeping

| Consumer (5 V rail) | Typical | Maximum |
|---|---|---|
| Raspberry Pi 4B (Wi-Fi, no USB devices) | ≈ 0.60 A | ≈ 1.30 A |
| MQ-2 heater | ≈ 0.12 A | ≈ 0.15 A |
| IR sensors (×2) | ≈ 0.04 A | ≈ 0.05 A |
| LEDs (red + RGB) | ≈ 0.014 A | ≈ 0.022 A |
| Speaker (only while sounding) | — | ≈ 0.12 A |
| **Total** | **≈ 0.77 A** | **≈ 1.64 A** |
| UPS HAT limit | | 2.5 A continuous |

Theoretical autonomy: 2 × 18650 ≈ 19.2 Wh (≈ 16 Wh usable) → ≈ 4 h typical, ≈ 2 h at maximum load. Acceptance target ≥ 3 h, to be measured.

**No RTC:** time comes from NTP (chrony) at boot plus a script that saves the date periodically and restores it at boot. NTP must sync before the TLS handshake. After a boot without internet, timestamps are approximate until NTP syncs; event order never depends on the clock (it uses `EVENT_LOG.id`).

---

## 15. Physical assembly (DEC-007, 7.3 b)

1. **Bring-up** on the breadboard, one circuit at a time, to test each peripheral and the driver.
2. **Demonstration build:** an interface board on protoboard (speaker driver, 3 RGB NPNs, MQ-2 divider, IR diodes and pull-ups, screw terminals for the sensor cables) inside a **commercial plastic box bought in a local shop**, with cut-outs for the keypad, panel button, LEDs and speaker, and ventilation slots for the Pi.
3. **Installation:** IR sensors on the door/window frames, beam on the leaf; MQ-2 high on a wall or the ceiling; cables to the box.

Not chosen: a custom PCB (no time and no benefit for a few transistors and resistors).

---

## 16. Parts still to buy

| Item | Quantity |
|---|---|
| UPS HAT (Ref. 4106, with 8.4 V charger) | 1 |
| Li-ion 18650 2600 mAh (Ref. BAT03005) | 2 |
| Keypad 3×4 (Ref. 434) | 1 |
| Push-button Ref. 716 + Ref. 1188 | 1 + 1 |
| MQ-2 gas sensor brick (Ref. 939) | 1 |
| NPN BC337 or 2N2222A | 4 (+2 spare) |
| 1N4148 | 1 (+1) |
| Schottky BAT85 (1N5817 acceptable) | 2 (+1) |
| Resistors 220 Ω ×3, 390 Ω ×1, 470 Ω ×1, 1 kΩ ×3, 10 kΩ ×7, 20 kΩ ×1, 33 Ω 0.5 W ×1 | — |
| Protoboard, pin headers, screw terminals, Dupont jumpers, multi-core cable | 1 set |
| Pi heatsinks (the HAT covers the board) | 1 set |
| Commercial plastic enclosure (local shop) | 1 |

**Rejected:** self-flashing red LED (cannot be held steady); common-cathode RGB LED (needs high-side switching); ADS1115 ADC (MQ-2 DO is used); BC547 for the speaker (100 mA max); RTC DS3231 (NTP + saved-date script).

---

## 17. Verification checklist

| # | Check | Purpose | Status |
|---|---|---|---|
| 1 | `i2cdetect -y 1` shows `0x42`; sign of the INA219 current with the charger unplugged | Mains detection | Not done (HAT not bought) |
| 2 | HAT cuts its output at full discharge and restores it when the charger returns | Condition of the safe-state decision (DEC-005b, 5b.3) | Not done |
| 3 | IR output type, real LOW level at the GPIO, detection distance to the leaf | IR interface margin (DEC-007) | **Possible now** (IR owned) |
| 4 | GPIO21 HIGH level with the divider (≥ ≈ 2.5 V); MQ-2 threshold with a controlled smoke source | MQ-2 interface | Not done |
| 5 | Transistor pinout of the parts bought; speaker volume with 33 Ω; GPIO18 LOW when silent | Speaker driver | Not done |
| 6 | RGB fully off with GPIO LOW and during boot | RGB interface (DEC-007) | Not done |
| 7 | PWM on GPIO18 and GPIO13 with the custom overlay, GPIO19 untouched | Driver | Not done |
| 8 | Measured autonomy; time from 6.7 V to 6.4 V | LOW_BATTERY margin (non-linear Li-ion discharge) | Not done |
| 9 | Enclosure fits the Pi with the HAT and the interface board | Assembly | Not done |
| 10 | 18650 cells fit the holder; HAT switch off when inserting | Safety | Not done |

---

## 18. Open points

- Exact IR sensor model and output stage (check 3).
- MQ-2 DO threshold (on-board trimmer) and warm-up time.
- INA219 current sign and mains-detection thresholds.
- Tone frequencies, tuned by ear.
- RGB LED part number (CEL03119 is unverified).
- Enclosure dimensions.
