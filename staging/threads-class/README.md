# Staging — Threads Implementation Class (3 h plan)

Everything from *VigilSystem – Threads Implementation Class Plan (3h)*, written and tested here
before it goes into the real folders. The layout mirrors the repository: `pi/` → `pi/`,
`cloud/` → `cloud/`. Nothing here is committed yet.

## Status

| Plan item | Done | How it was checked |
|-----------|------|--------------------|
| `core/` contract: `EventType`, `Event`, `PiMutex`, `EventQueue`, `CommandQueue<T>`, `Commands` | yes | unit tests (FIFO, eventfd, 4 producers × 2000 events, close/popFor) |
| `WorkerThread` (explicit sched attrs, effective policy logged, heartbeat) | yes | unit tests |
| **David** — `NetworkMQTTThread` + `MqttClient` (TLS, MQTT 5, Last Will, connect sequence) | yes | demo checks 1, 5, 6, 7 on the local Mosquitto |
| Reconnect backoff 1 → 60 s, 0x86/0x87 every 5 min, logged once | yes | unit tests + wrong-password run (0x87 logged once) |
| `Protocol::parseCmd` → `EVT_REMOTE_*`, `get_events` → empty list | yes | unit tests + demo check 3 |
| Stub `AlarmControllerThread` (epoll, drain, print, `[PIN hidden]`) | yes | unit tests + demo |
| **Ana** — `LocalInputThread` (15 ms scan, debounce, PIN buffer) | yes | unit tests on the simulated keypad + demo check 2 |
| `MQ2SensorThread` (100 ms, 5 reads, warm-up, `isSmokePresent()`) | yes | unit tests + demo check 4 |
| `IRSensorsThread` (edges, 200 ms guard, faults) | yes | unit tests + demo check 4 |
| Keypad jitter measurement | yes | logged every 60 s and at stop |
| Stretch: `PowerMonitorThread` with a fake INA219 | yes | unit tests |
| Real hardware: `GpioLines` (libgpiod v1), `Ina219` | written | **not compiled**: the laptop has no libgpiod v1; first cross-build will tell |
| "If you finish early": outbox drain, OutputThread | no | next session |

69 unit tests pass. The 8 demo checks pass on the laptop (results below).

## Run it

From the repository root:

```bash
# build + unit tests (GoogleTest is downloaded once if libgtest-dev is missing)
cmake -S staging/threads-class/pi -B staging/threads-class/pi/build -G Ninja -DVIGIL_SIM=ON -DVIGIL_BUILD_TESTS=ON
cmake --build staging/threads-class/pi/build && ctest --test-dir staging/threads-class/pi/build --output-on-failure

# interactive: broker in one terminal, vigil in another (type 'help' in vigil)
cloud/mosquitto/run.sh
staging/threads-class/pi/build/vigil -c staging/threads-class/pi/config/broker.conf -C staging/threads-class/pi/config/vigil.sim.conf
staging/threads-class/cloud/send-cmd.sh staging/threads-class/cloud/samples/cmd-arm.json

# all 8 demo checks in one go (~80 s); do not run the broker yourself for this one
staging/threads-class/cloud/class-demo.sh /tmp/vigil-demo
```

`pi/config/broker.conf` is a copy of `broker.local.conf.example` and is git-ignored.
Console commands: `key 1234#`, `button`, `door 1 open|close`, `smoke on|off`, `mains on|off`,
`volts 6.3`, `fail 16 on|off`, `quit`.

## Demo results (laptop, 5 Nov 2026)

| # | Expected | Seen |
|---|----------|------|
| 1 | `EVT_NET_RESTORED`, `online` retained | yes |
| 2 | 4 × `EVT_KEY_DIGIT`, `EVT_PIN_SUBMIT [PIN hidden]` | yes, 50–190 µs from the event to the controller |
| 3 | `EVT_REMOTE_ARM` with its request_id | yes |
| 4 | smoke + door 1 + disarm all arrive | yes, all three, none lost |
| 5 | `EVT_NET_DISCONNECTED` once, keypad still works | yes; reconnect attempts at 1, 2, 4 s |
| 6 | `EVT_NET_RESTORED`, `online` again | yes, at the next backoff step |
| 7 | Last Will `{"state":"offline"}` | yes, but **at once**, not after ≈ 45 s (see notes) |
| 8 | FF 80 / 70 / 50 / TS | all TS on the laptop: needs root (see notes) |

Extra checks: `get_events` gets `{"events":[],…,"result":"OK"}`; unknown type and a cmd without
Message Expiry are ignored with a syslog line and no reply; Ctrl+C leaves a retained `offline`.

## Things to know before class

- **Check 7 on a laptop:** `kill -9` closes the TCP socket, so Mosquitto sees the drop at once and
  publishes the Will immediately. The ≈ 45 s (1.5 × keep-alive) only shows when the link dies
  silently: unplug the Pi's power or Wi-Fi, or drop the traffic
  (`sudo iptables -A OUTPUT -p tcp --dport 8883 -j DROP`, remove it with `-D` afterwards).
- **Check 8 needs root:** without `CAP_SYS_NICE`, `WorkerThread::start()` logs a warning and falls
  back to `SCHED_OTHER` so the laptop demo still runs. Use `sudo` (or run on the Pi as root) to see
  FF. Thread names drop the `Thread` suffix (15-char limit), so `grep vigil` from the plan does not
  match them; use `ps -L -o tid,cls,rtprio,comm -p "$(pidof vigil)"`.
- **Jitter:** the laptop number (max ≈ 1.8 ms as `SCHED_OTHER`) is not for the report. Measure on
  the Pi as root, with the `performance` governor (RT-9).
- **Buildroot needs `BR2_PACKAGE_NLOHMANN_JSON=y`** in `buildroot/configs/vigil_fragment.config`
  (`Protocol` uses it). It is not there yet.

## Choices I made where the guide is silent

Guide §18 has no default for these. Please confirm or change them; once agreed they should go
into `docs/IMPLEMENTATION_GUIDE.md` (§18.2):

1. **Normal stop (SIGTERM/Ctrl+C):** publish `{"state":"offline"}` retained, wait ≤ 2 s for the
   PUBACK, then DISCONNECT 0x00. Without this the app keeps showing a stale `online`. I first tried
   DISCONNECT 0x04 ("send the Will"), but Paho C++ 1.2 does not pass the reason code to the broker.
2. **MQ-2 at the end of warm-up:** the debounce counters restart, so smoke already present needs 5
   fresh LOW reads (0.5 s) and then raises one `EVT_SMOKE_DETECTED`. During warm-up: no events.
3. **Keypad/button request failure:** syslog only, no `EVT_FAULT` (there is no `FaultComponent` for
   the keypad, same as G8 for the UPS).
4. **`NetCmd` extra fields** (`code`, `token`, `ok`, `topic`, `retained`, `hasExpiry`) carry what
   the Paho callbacks report. A failed connect is posted as `ConnectionLost` with the CONNACK reason
   in `code`, so there is no new `Kind`.
5. **Laptop without root:** `SCHED_FIFO` falls back to `SCHED_OTHER` with a warning (above).
6. **The stub controller publishes no status**, so after CONNACK only `online` goes out ("no status
   snapshot yet" in syslog). The real `AlarmControllerThread` will send it at start-up.

## Bug found while testing (fixed)

With a wrong password, Paho 1.3 puts the broker's reason (0x87) in the token's return code, not
in its reason-code field. The first version therefore used the normal backoff instead of the
5-minute retry. `MqttClient` now checks both fields.

## Ownership

The plan splits the work, and this folder has both halves: David's (`NetworkMQTTThread`,
`MqttClient`, `Protocol`, controller stub, samples) and **Ana's** (`LocalInputThread`,
`MQ2SensorThread`, `IRSensorsThread`, `PowerMonitorThread`, `IGpio`/`GpioLines`/`Ina219`, the
simulated board). Ana should review her half, mainly `GpioLines.cpp`, which no one has compiled yet.

## Move it into place

```bash
cd ~/VigilSystem
rm -rf staging/threads-class/pi/build
cp -r staging/threads-class/pi/. pi/
cp staging/threads-class/cloud/send-cmd.sh staging/threads-class/cloud/class-demo.sh cloud/
cp staging/threads-class/cloud/samples/*.json cloud/samples/
rm -f pi/{config,include,src,tests}/.gitkeep cloud/samples/.gitkeep
git status    # broker.conf must NOT appear (git-ignored)
```

After the move the commands in `CLAUDE.md` work as they are
(`cmake -S pi -B pi/build -G Ninja -DVIGIL_SIM=ON -DVIGIL_BUILD_TESTS=ON …`).

## Next session

- David: outbox drain (token bucket 5/s, in-flight map token → event_id, SENT on PUBACK 0x00/0x10),
  `StorageThread`, `LinkMonitor` (netlink, Wi-Fi loss ≤ 2 s), low-battery shutdown sequence (§6.9).
- Ana: `OutputThread` with a fake `PwmDevice`, the `vigil_pwm` driver, first cross-build to fix
  `GpioLines`.
- Both: `AlarmLogic` (§6) replaces the stub controller, with the §16.2 tests.
