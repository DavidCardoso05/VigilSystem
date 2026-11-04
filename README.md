# VigilSystem

Home intrusion and fire alarm on a Raspberry Pi 4, with remote control from an Android app over MQTT 5/TLS (EMQX Serverless).
Group 8 · ESRG Embedded Systems · David Cardoso (cloud + app) · Ana Sofia Cunha

| Folder       | What goes there |
|--------------|-----------------|
| `pi/`        | C++17 application for the Pi (CMake). `include/` headers, `src/` sources, `tests/`, `config/broker.conf.example` |
| `driver/`    | Linux kernel driver for the speaker and red LED (`/dev/vigil_pwm`) |
| `cloud/`     | Broker setup: `emqx/` (ACL rules, setup notes), `mosquitto/` (local test broker: conf, ACL, cert script), `samples/` (JSON messages for mosquitto_pub) |
| `buildroot/` | Defconfig fragment (`configs/`) and root filesystem overlay (`overlay/`) |
| `app/`       | Flutter app (Android) |
| `docs/`      | Design presentation, broker-selection document, protocol notes |
| `secrets/`   | Passwords and certificates. **Ignored by git, never commit it** |

Design sources, in order of priority: `docs/design/G8_VigilSystem_D_Presentation.pdf`, then the broker-selection `.docx`.
Today's plan: [NEXT_3_HOURS.md](NEXT_3_HOURS.md).
