# VigilSystem — instructions for Claude

Home intrusion and fire alarm on a Raspberry Pi 4 (Buildroot, C++17, pthreads, one custom PWM driver) with an Android app over MQTT 5/TLS through EMQX Serverless. Group 8 · David Cardoso · Ana Sofia Cunha · MEEIC, Universidade do Minho (SEA + STR).

> Version 1.0 (2026-11-09), approved by David. Replaces the `CLAUDE.md` of 4 Nov, which pointed to an obsolete design.

## Read first

1. **`VIGILSYSTEM_MASTER_CONTEXT.md`** — the single source of truth. Read §0 at the start of every task; read §C, §E, §G and §N before proposing any design change.
2. **`VIGILSYSTEM_GUIDELINES.md`** — read §2 before changing any file.
3. `docs/` — detailed specifications. They derive from the Master Context; if they disagree with it, the Master Context wins: report the difference.

Never implement from `docs/archive/` or from documents listed as obsolete in Master Context §N.3, even if they look recent.

## Non-negotiable rules

- **No file is created, modified, deleted, renamed or restructured without David's explicit approval** (code, docs, diagrams, configuration, tests, commits). Approving a decision is not authorization to execute it.
- Before any change: problem, alternatives (including keeping the current solution), trade-offs, consequences, recommendation, then ask.
- Architectural proposals include a SOLID Impact Analysis. Settled decisions (Master Context §G) are reopened only for a new problem and with authorization.
- Distinguish facts, assumptions and proposals; never invent; report problems instead of silently changing the design.
- Talk to David in European Portuguese (technical terms in English); write documentation and code in English.
- Never commit secrets (`secrets/`, `*.pem`, `*.key`, `broker.conf`, `wpa_supplicant.conf`, `app/assets/config/broker.json`); no `git commit` unless asked; never log a PIN, password or `cmd` payload.

## The approved design in brief (details: Master Context)

- **5 threads + `main`:** `LocalInputThread` (15 ms, FIFO 50), `SafetySensorsThread` (100 ms, FIFO 80), `PowerMonitorThread` (1 s, FIFO 50), `AlarmControllerThread` (waits until the next deadline, FIFO 80), `CommThread` (SCHED_OTHER). `main` feeds the watchdog from heartbeats.
- **2 queues:** EventQueue (all → controller), CommQueue (controller + Paho → Comm). Rule: merge threads, never responsibilities.
- **Packages:** `threads`, `logic` (pure, time injected), `devices`, `hw` (interfaces + real/simulated), `outputs`, `storage`, `net`, `core`.
- **Driver:** `/dev/vigil_pwm` plays step sequences (≤ 8 steps) on PWM0/GPIO18 (speaker) and PWM1/GPIO13 (red LED).
- **Database:** 2 tables — `EVENT_LOG` (with outbox columns `status`, `sent_at`) and `PIN_CONFIG`.
- **Never use (obsolete):** 9 threads, OutputThread, StorageThread, NetworkMQTTThread, SupervisorThread, IRSensorsThread, MQ2SensorThread, epoll/eventfd/timerfd, `IEventSink`, `PENDING_NOTIFICATION`, `get_events`, `LinkMonitor`, battery %, power-off on low battery, four custom drivers, PIR sensor, red LED on GPIO26.

## Where things are

| Folder | Content | Status (9 Nov 2026) |
|---|---|---|
| `pi/` | C++17 application (CMake) | Empty |
| `staging/threads-class/` | Laptop prototype of the **superseded** architecture (5 Nov), not committed | To refactor (Master Context §J.1) |
| `driver/` | `vigil_pwm` kernel module + overlay | Empty |
| `buildroot/` | Defconfig, fragment, rootfs overlay | Configured |
| `cloud/` | EMQX ACL rules, local Mosquitto test broker, `test-broker.sh` | Working on the laptop |
| `app/` | Flutter app (Android) | Empty |
| `docs/` | Implementation Guide, hardware specification, deck change plan, design PDF, archive | See Master Context Appendix A |

## Common commands

```bash
# laptop: unit tests + simulation build
cmake -S pi -B pi/build -G Ninja -DVIGIL_SIM=ON -DVIGIL_BUILD_TESTS=ON && cmake --build pi/build && ctest --test-dir pi/build --output-on-failure
# local broker + broker checks
cloud/mosquitto/run.sh            # then: cloud/test-broker.sh secrets/local.env
# cross-compile for the Pi
cmake -S pi -B pi/build-rpi -G Ninja -DCMAKE_TOOLCHAIN_FILE=$HOME/buildroot/buildroot-2026.08/output/host/share/buildroot/toolchainfile.cmake -DVIGIL_TARGET=ON
```

## When something is decided

Propose the documentation changes (Guidelines §5.2) and wait for approval. Do not edit the Master Context or any other document on your own.
