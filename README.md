# VigilSystem

Home intrusion and fire alarm on a Raspberry Pi 4, with remote control from an Android app over MQTT 5/TLS (EMQX Serverless).
Group 8 · ESRG Embedded Systems · David Cardoso · Ana Sofia Cunha

## Start here

1. [`VIGILSYSTEM_MASTER_CONTEXT.md`](VIGILSYSTEM_MASTER_CONTEXT.md) — the single source of truth for requirements, architecture and decisions.
2. [`VIGILSYSTEM_GUIDELINES.md`](VIGILSYSTEM_GUIDELINES.md) — working rules, SOLID review, draw.io rules and the documentation maintenance procedure.

Claude Code reads a local `CLAUDE.md` (not in the repository; ask David for a copy).

If any other file disagrees with the Master Context, the Master Context wins. Never implement from `docs/archive/`.

## Folders

| Folder       | What goes there |
|--------------|-----------------|
| `pi/`        | C++17 application for the Pi (CMake). One folder per package (Master Context §E.3) in `include/vigil/<package>/` (headers) and `src/<package>/` (sources): `core`, `logic`, `devices`, `hw` (+ `hw/sim`), `outputs`, `storage`, `net`, `threads`; `tests/`; `config/` |
| `driver/`    | Linux kernel driver for the speaker and red LED (`/dev/vigil_pwm`) |
| `cloud/`     | Broker setup: `emqx/` (ACL rules, setup notes), `mosquitto/` (local test broker: conf, ACL, cert script), `samples/` (JSON messages for mosquitto_pub) |
| `buildroot/` | Defconfig fragment (`configs/`) and root filesystem overlay (`overlay/`) |
| `app/`       | Flutter app (Android) |
| `staging/`   | Not committed. Only the broker code of the 5 Nov laptop prototype (`net/` classes, their tests, `send-cmd.sh` and command samples), kept as reference for `pi/src/net/` (Master Context §J.1) |
| `docs/`      | Implementation Guide, hardware specification (`hardware/`), deck change plan (`presentation/`), current design deck PDF (82 slides) and broker document (`design/`), superseded documents (`archive/`) |
| `secrets/`   | Passwords and certificates. **Ignored by git, never commit it** |
