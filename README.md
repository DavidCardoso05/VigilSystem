# VigilSystem

Home intrusion and fire alarm on a Raspberry Pi 4, with remote control from an Android app over MQTT 5/TLS (EMQX Serverless).
Group 8 · ESRG Embedded Systems · David Cardoso · Ana Sofia Cunha

## Start here

1. [`VIGILSYSTEM_MASTER_CONTEXT.md`](VIGILSYSTEM_MASTER_CONTEXT.md) — the single source of truth for requirements, architecture and decisions.
2. [`VIGILSYSTEM_GUIDELINES.md`](VIGILSYSTEM_GUIDELINES.md) — working rules, SOLID review, draw.io rules and the documentation maintenance procedure.

If any other file disagrees with the Master Context, the Master Context wins. Never implement from `docs/archive/`.

## Folders

| Folder       | What goes there |
|--------------|-----------------|
| `pi/`        | C++17 application for the Pi (CMake). `include/` headers, `src/` sources, `tests/`, `config/broker.conf.example` |
| `driver/`    | Linux kernel driver for the speaker and red LED (`/dev/vigil_pwm`) |
| `cloud/`     | Broker setup: `emqx/` (ACL rules, setup notes), `mosquitto/` (local test broker: conf, ACL, cert script), `samples/` (JSON messages for mosquitto_pub) |
| `buildroot/` | Defconfig fragment (`configs/`) and root filesystem overlay (`overlay/`) |
| `app/`       | Flutter app (Android) |
| `staging/`   | Laptop prototype of a superseded architecture, kept until it is refactored (Master Context §J.1) |
| `docs/`      | Implementation Guide, hardware specification (`hardware/`), deck change plan (`presentation/`), design PDF and broker document (`design/`), superseded documents (`archive/`) |
| `secrets/`   | Passwords and certificates. **Ignored by git, never commit it** |
