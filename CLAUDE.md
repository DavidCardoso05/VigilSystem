# VigilSystem — instructions for Claude

Home intrusion + fire alarm on a Raspberry Pi 4 (Buildroot, C++17, pthreads, one custom PWM driver) with an Android app over MQTT 5/TLS (EMQX Serverless). Group 8 · David Cardoso · Ana Sofia Cunha.

**Before writing any code, read [`docs/IMPLEMENTATION_GUIDE.md`](docs/IMPLEMENTATION_GUIDE.md)** — at least §0 (the 25 key rules) and §5–§6 (architecture and behaviour), then the section for the component you are working on. It is the single source of truth and overrides older documents.

## Rules

- The design is decided: do not redesign threads, the driver, topics, messages, the database or the state machines. If something looks wrong, say so and cite the rule.
- Keep the names from the UML diagrams exactly (`IRSensorsThread`, `NetworkMQTTThread`, `AlarmControllerThread`, …). Never use the obsolete names listed in guide §2.3 (PIR thread, `CommunicationThread`, the four custom drivers, red LED on GPIO26).
- When the design is silent, use the default in guide §18 and say that you did; if there is none, ask.
- Never commit secrets: `secrets/`, `*.pem`, `*.key`, `broker.conf`, `app/assets/config/broker.json`, `wpa_supplicant.conf`. Check `git status` before committing.
- Never log or publish a PIN, password or `cmd` payload.
- Every logic change comes with unit tests (`AlarmLogic` runs on the laptop, guide §16.2).
- Ownership: David = cloud, MQTT contract, `NetworkMQTTThread`, Flutter app; Ana = hardware, `vigil_pwm` driver, hardware threads. Mention it when a change crosses into the other area.

## Where things are

| Folder | Content | Guide |
|--------|---------|-------|
| `pi/` | C++17 application (CMake) | §5–§7, §9–§11 |
| `driver/` | `vigil_pwm` kernel module + device-tree overlay | §8 |
| `buildroot/` | defconfig, fragment, rootfs overlay, (BR2_EXTERNAL) | §12 |
| `cloud/` | EMQX ACL, local Mosquitto test broker, test script, samples | §10 |
| `app/` | Flutter app (Android) | §13 |
| `docs/design/` | design deck (PDF) and broker-selection document | — |

## Common commands

```bash
# laptop: unit tests + simulation build
cmake -S pi -B pi/build -G Ninja -DVIGIL_SIM=ON -DVIGIL_BUILD_TESTS=ON && cmake --build pi/build && ctest --test-dir pi/build --output-on-failure
# local broker + broker checks
cloud/mosquitto/run.sh            # then: cloud/test-broker.sh secrets/local.env
# cross-compile for the Pi
cmake -S pi -B pi/build-rpi -G Ninja -DCMAKE_TOOLCHAIN_FILE=$HOME/buildroot/buildroot-2026.08/output/host/share/buildroot/toolchainfile.cmake -DVIGIL_TARGET=ON
```

If a decision changes, update `docs/IMPLEMENTATION_GUIDE.md` in the same change.
