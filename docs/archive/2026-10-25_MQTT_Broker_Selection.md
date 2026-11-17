> **ARCHIVED — superseded on 2026-11-09 by `VIGILSYSTEM_MASTER_CONTEXT.md` v1.0. Kept for history only. Do not implement from this document.**
>
> Original location: Claude Project: `VigilSystem – MQTT Broker Selection EMQX Serverless and Mosquitto Test Broker.md` · Document date: 25 Oct 2026 · Superseded by: Master Context §I.1–§I.2 (the broker choice itself is still valid).
> The text below this box is an unchanged copy of the original.

---

# VigilSystem – MQTT Broker Selection: EMQX Serverless and Mosquitto Test Broker

Oct 25, 2026 · @David

## Purpose and context

VigilSystem uses **EMQX Serverless** as its cloud MQTT broker and a **local Mosquitto broker** only as a development and test tool. The broker is the system's only cloud component: it relays alerts and status from the Raspberry Pi to the mobile app, and commands from the app back to the Pi.

The choice is driven by these analysis-phase requirements:

- **Security:** remote communication must be authenticated and encrypted (MQTT over TLS). The PIN is validated only on the device.
- **Instant alerts:** intrusion, fire/gas and power events must reach the app in real time.
- **Connectivity monitoring:** the device monitors its connection to the cloud broker, not only the Wi-Fi link.
- **Pending notifications:** events generated while offline are delivered in chronological order after reconnection.
- **Budget:** limited student budget and no recurring subscription, consistent with the project's motivation against subscription-based alarms.

## Candidates evaluated

Four brokers were compared: two managed brokers with free tiers, one full IoT cloud platform and one self-hosted open-source broker. All four speak standard MQTT over TLS, so the device-side Paho client is the same for every option.

| Criterion | [HiveMQ Cloud Serverless](https://docs.hivemq.com/hivemq-platform/connect/access-management-options-per-cloud-plan.html) | [EMQX Serverless](https://docs.emqx.com/en/cloud/latest/create/restriction.html) | [AWS IoT Core](https://aws.amazon.com/iot-core/pricing/) | [Mosquitto (self-hosted)](https://www.mankier.com/5/mosquitto.conf) |
| --- | --- | --- | --- | --- |
| Type | Managed, shared cluster | Managed, shared cluster | Managed IoT platform inside AWS | Open-source broker on our own server |
| TLS | MQTT over TLS and WebSockets | TLS 1.2 and 1.3; provider certificate only (no custom TLS) | TLS on port 8883 | Fully configurable; we manage certificates |
| Authentication | Username/password only (no X.509, no JWT) | Username/password and X.509 client certificates | X.509 natively; username/password only through a Lambda custom authorizer | Password file, client certificates, Dynamic Security plugin |
| Topic permissions (ACL) | One permission per credential; no roles, no per-client variables | Allow/deny rules per client ID, username or all users; up to 100 per client; `${username}`/`${clientid}` placeholders | IoT policies per action and topic, with policy variables | ACL file with `%u`/`%c` patterns |
| Offline queue | Stored data and retained messages kept 3 days | 1000 queued messages per client; 2 h session expiry (MQTT 3.x) | Persistent sessions kept 1 h by default | Expiry and queue size configurable |
| MQTT 5 / QoS 2 | Yes / Yes | Yes / Yes | Yes, with documented deviations / No | Yes / Yes |
| Main limits | 100 connections; 10 GB/month | 1000 connections; 1 MB messages; 10 publishes/s per client | 128 KB messages, billed per 5 KB | Server resources only |
| Free tier | Free, no credit card | Every month: 1 M session minutes, 1 GB traffic; spend limit 0 keeps it free | 12 months: 2.25 M connection minutes, 500 k messages; new accounts since 15 Jul 2025 get up to $200 credits instead | Software free; server not |
| First paid step | Starter: $0.34/h + $0.80 per M messages (≈ $250/month) | $2.00 per M session minutes, $0.15/GB | $1.00 per M messages, $0.08 per M connection minutes | Small VPS, a few €/month |
| Uptime SLA | None | Not stated for Serverless | Yes (AWS standard) | Our responsibility |
| Setup effort | Very low | Very low | High (certificates, policies, IAM, app authentication) | Medium to high (server, firewall, certificates, updates) |

HiveMQ's current [pricing page](https://www.hivemq.com/pricing/) lists only the paid Starter and Enterprise managed plans, while its documentation still describes the free Serverless plan. Its availability for new sign-ups should be confirmed before relying on it.

## Why EMQX Serverless

EMQX Serverless is the only free option that supports both least-privilege topic rules and X.509 device certificates, and it costs nothing at VigilSystem's load.

1. **Least-privilege authorization.** Allow/deny rules per username or client ID (up to 100 per client) let each credential do only what its role needs. HiveMQ's free plan allows one permission per credential, which forces both the device and the app to get full access to the topic tree.
2. **Certificate authentication on the free plan.** X.509 client certificates are supported, so the device can move from password to mutual TLS later without changing broker.
3. **Zero cost at our load.** One client connected all month uses 30 × 24 × 60 = 43,200 session minutes. The Pi plus two phones use about 130,000, roughly 13% of the 1 M free quota. Alert and status messages are a few hundred bytes, far below 1 GB/month. With the spend limit at 0, the deployment can never be charged.
4. **The MQTT features the design relies on.** MQTT 5, QoS 1, retained status messages, Last Will and Testament, and persistent sessions are all supported.
5. **No server to maintain.** No credit card is needed, setup takes minutes, and the TLS 1.2/1.3 certificate is managed by the provider.
6. **Cheap growth path.** Beyond the free quota, the same three clients would cost about $0.26/month in session fees.

The alternatives were rejected for these reasons:

- **HiveMQ Cloud Serverless:** password-only authentication, one permission per credential, and uncertain availability of the free plan.
- **AWS IoT Core:** high setup effort (certificates, policies, IAM, mobile-app authentication), a credit card is required, the free period is time-limited, QoS 2 is not supported and the offline queue defaults to 1 hour.
- **Self-hosted Mosquitto:** we would have to operate and secure an internet-facing server. It is kept instead as a local test broker (see below).

## Least-privilege topic permissions

Each credential is limited to the topics and actions its role needs, and a final deny-all rule blocks everything else. EMQX checks [username and client-ID rules first, then all-user rules, and applies the first match](https://docs.emqx.com/en/cloud/latest/deployments/default_authz.html). Serverless allows unmatched requests by default, so the deny-all fallback rule is mandatory.

| Username | Topic | Action | Permission | Purpose |
| --- | --- | --- | --- | --- |
| vigil-device | vigil/home1/status | Publish | Allow | Retained system state (armed, power source, battery, alarm) |
| vigil-device | vigil/home1/alert | Publish | Allow | Intrusion, fire/gas and power alerts |
| vigil-device | vigil/home1/event | Publish | Allow | Event log entries |
| vigil-device | vigil/home1/response | Publish | Allow | Replies to app commands |
| vigil-device | vigil/home1/connection | Publish | Allow | Online message and Last Will (offline) |
| vigil-device | vigil/home1/cmd | Subscribe | Allow | Receive arm, disarm and PIN-change commands |
| vigil-app | vigil/home1/cmd | Publish | Allow | Send commands |
| vigil-app | vigil/home1/status | Subscribe | Allow | Show current state |
| vigil-app | vigil/home1/alert | Subscribe | Allow | Show alerts |
| vigil-app | vigil/home1/event | Subscribe | Allow | Show recent events in the alert list |
| vigil-app | vigil/home1/response | Subscribe | Allow | Receive command results |
| vigil-app | vigil/home1/connection | Subscribe | Allow | Show whether the device is online |
| All users | # | Publish and Subscribe | Deny | Fallback: block everything not allowed above |

With these rules, a leaked app credential can only send commands, and disarming still requires the PIN. It cannot publish fake alerts or status, fake the device's online message, or read other commands carrying PINs. A leaked device credential requires physical access to the SD card inside the home.

On HiveMQ's free plan, each credential holds a single permission. Both clients would need publish and subscribe on the whole `vigil/home1/#` tree, so a leaked app credential could spoof alerts and status and read every command.

## Known limitations and mitigations

None of the Serverless limits blocks VigilSystem, but four of them shape the device and app design.

| Limitation | Impact on VigilSystem | Mitigation in the design |
| --- | --- | --- |
| Max 10 publishes/s per client | A long outage can leave a backlog of pending notifications larger than this rate | CommunicationThread paces the resend (e.g. 5 messages/s) and marks a row SENT only after the broker's PUBACK |
| Offline queue: 1000 messages per client, 2 h session expiry for MQTT 3.x clients | A phone offline for more than 2 h can miss queued alerts | The app reads the retained `status` on reconnect and requests recent events from the device; use MQTT 5 and set the session expiry interval (maximum accepted by Serverless to be confirmed) |
| Unmatched topic requests are allowed by default | A missing rule would silently grant access | Deny-all fallback rule, plus a test case checking that forbidden publishes are rejected |
| No uptime guarantee stated for the shared Serverless cluster | Remote alerts stop during a broker outage | Local alarms never depend on the network; the device buffers alerts in `PENDING_NOTIFICATION` and shows the loss on the RGB LED |
| Spend limit 0 pauses the deployment when the free quota runs out | Remote access would stop until the next month | Estimated use is about 13% of the quota; the console's quota alerts are watched |
| Offline clients with persistent sessions still consume session minutes | Phones use quota while disconnected | Already included in the estimate, which assumes every client 24/7 |
| No custom certificate or domain | The device must trust the provider's certificate chain | Include the CA certificate bundle in the Buildroot image and verify the broker hostname |
| Max 10 subscriptions per client | None: the app needs 5 | None needed |
| Dependence on a third-party cloud service | Conflicts with the project's argument against subscription-based alarms | Free tier, standard MQTT and a broker address in a config file, so self-hosted Mosquitto can replace EMQX without code changes |

## Broker only, no cloud database

The EMQX broker is VigilSystem's only cloud component; there is no cloud database and no backend server. The Raspberry Pi is the source of truth, and the broker only relays messages between the Pi and the app.

Reasons for not adding a cloud database:

- **The device already keeps the data.** `EVENT_LOG` holds the full history and `PENDING_NOTIFICATION` buffers alerts while offline, both in the Pi's SQLite database.
- **Security.** The PIN is validated only on the device. A cloud database would be one more place holding system data that must be secured.
- **Scope and budget.** A cloud database needs a backend, its own authentication and its own tests. Its only extra value would be showing the full history while the Pi is offline, which the prototype app does not need: the full history stays on the Pi.

| Where | What it stores | Purpose |
| --- | --- | --- |
| Raspberry Pi (SQLite) | `EVENT_LOG`, `PENDING_NOTIFICATION`, `PIN_CONFIG` | Source of truth: full history, buffering while offline, PIN validation |
| EMQX broker | Retained `status` and `connection` messages; each phone's offline queue | Relay between Pi and app; short-term delivery |
| Mobile app (key-value storage) | The last \~20 alerts and events, plus the last `event_id` | Shows recent alerts, even with the Pi offline; drives the sync |

No event is permanently lost: anything the broker drops is recovered from `EVENT_LOG` by the sync described below. The one remaining gap is a delay. If the phone's broker session has expired and the Pi is offline when the app opens, the missing events arrive only after the Pi reconnects.

**Open decision: notifications while the app is closed.** The broker only delivers to an app that holds an open MQTT connection. On Android, a foreground service can keep that connection open, which keeps the design broker-only. Real push notifications (Firebase Cloud Messaging) would need a small extra component that forwards `alert` messages, so the design would no longer be broker-only. This is decided together with the app's scope.

## Notifications while the app is closed: Android foreground service

For the prototype, the mobile app keeps its MQTT connection open inside an Android foreground service. Alerts reach the phone even when the app is not on screen, and the design stays broker-only.

**How it works.** The app starts the service on first launch. The service holds the MQTT 5 connection to EMQX (persistent session, QoS 1) and turns each message on `alert` into a high-priority Android notification. Android requires a permanent notification while the service runs; it is shown silently on a low-priority channel (e.g. "VigilSystem – monitoring").

**Why it is the best choice for prototyping:**

- **The architecture does not change.** No extra cloud component, no new credentials and no new ACL rules; the Pi needs no changes.
- **No cost and no extra accounts.** No Firebase project, Google service-account key or Apple developer account is needed.
- **Least work for a two-person, one-semester project.** Everything lives in the app, so the effort goes into the embedded system, which is what the course assesses.
- **Easy to demonstrate.** The demo can trigger an intrusion or smoke alarm with the app closed and show the notification arriving.
- **Proven pattern.** The open-source [ntfy](https://github.com/binwiederhier/ntfy) Android app uses the same approach for its instant-delivery mode, with a small battery cost.
- **Clean upgrade path.** Firebase push can be added later as a wake-up signal only. The app always syncs over MQTT after waking, so nothing else changes.

| Disadvantage | Mitigation |
| --- | --- |
| Android only; iOS does not allow long-lived background connections | The prototype app targets Android; Firebase push is the planned extension for iOS |
| [Android 15](https://developer.android.com/about/versions/15/behavior-changes-15) limits the `dataSync` service type to 6 hours per 24 hours | Declare a service type without this limit (e.g. `specialUse`), to be verified during implementation |
| Battery optimization and some manufacturers' background killing can stop the service | The app asks the user to exempt it from battery optimization; the demo phone is tested over 24 h |
| A permanent notification is always visible | Low-priority, silent channel |
| Keep-alive pings use some battery and data | Moderate keep-alive on the app (e.g. 60 s) |
| The phone uses EMQX session minutes 24/7 | Already included in the quota estimate |
| If the phone itself has no internet, nothing arrives | Same for every option; the `EVENT_LOG` sync delivers the missed events after reconnection |

**Verification.** Test cases check that an alert arrives with the app closed, after a phone reboot, and after more than 24 h of continuous running.

## Mobile app: Flutter, Android first

The mobile app is built with **Flutter** and targets Android for the prototype. Flutter keeps an iOS version possible later, once Firebase push is added.

**Scope: a thin remote control.** The Pi holds all the logic: PIN validation, system state and history. The app only sends commands and shows what it receives, which keeps it small, since it is not the focus of the course.

| Requirement | App feature |
| --- | --- |
| Remote arm and disarm | Arm button; Disarm button with a PIN dialog |
| PIN change via the app | Dialog with the current and the new PIN |
| Instant alerts | The foreground service turns each `alert` into a phone notification |
| Visual status and connectivity | Status card built from the retained `status` and `connection` messages |
| Error feedback | Shows the device's reply, e.g. wrong PIN or disarm rejected during a fire alarm |

Simplifications that keep the app small:

- **One screen.** A status card (armed state, power source, alarm, device online), the Arm, Disarm and Change PIN buttons, and a list of the last \~20 alerts. PIN entry and PIN change are dialogs built with default Material widgets.
- **No login screen.** The broker address and app credentials are in a config file inside the app; the ACL rules limit what a leaked app credential can do.
- **No validation in the app.** It sends the command JSON, waits for the `response` and shows the result; the Pi decides everything.
- **Android only,** one phone, portrait orientation.

Left out of the prototype: a full event-history screen, configuring the entry/exit delays and the silence period from the app, multiple homes or users, and iOS with Firebase push.

| Function | Implementation |
| --- | --- |
| MQTT 5 client over TLS | [`mqtt5_client`](https://pub.dev/packages/mqtt5_client) package (secure TCP, all QoS levels, authentication, keep-alive) |
| Single MQTT connection | Runs inside the Android foreground service (e.g. the `flutter_foreground_task` plugin), whether the app is open or closed; the screen shows what the service receives |
| Recent alerts and last `event_id` | Key-value storage (`shared_preferences`) |
| Alert notifications | Local-notifications plugin, with a high-priority channel for alerts and a silent channel for the service notification |
| Broker address and credentials | Config file inside the app |

Two points are verified during implementation, not now:

- **CONNACK session expiry.** `mqtt5_client` does not act on every CONNACK parameter, so the app must still be able to read the session expiry EMQX reports back. It is needed for the expiry-cap test.
- **Foreground-service type.** The chosen plugin must allow a service type without the Android 15 time limit (e.g. `specialUse`).

## MQTT 5: what it solves and what it does not

MQTT 5 removes the fixed 2-hour session expiry but not the 1000-message queue limit, so it is used together with the `EVENT_LOG` sync, not instead of it.

| Limit on EMQX Serverless | With MQTT 3.x | With MQTT 5 |
| --- | --- | --- |
| Session lifetime after the phone disconnects | Fixed by the broker at 2 h | Chosen by the client (Session Expiry Interval); the broker may cap it, cap to be confirmed |
| Offline queue size | 1000 messages per client | 1000 messages per client (broker quota, not adjustable on Serverless) |
| Session lost after app reinstall or a new phone (new client ID) | Lost | Lost |
| Session survival across broker maintenance | Not documented | Not documented |

**Confirming the expiry cap.** The app, or a test script, connects with a Session Expiry Interval of 7 days (604,800 s). Under MQTT 5, a broker that applies a different value reports it in its connection acknowledgment (CONNACK), and Paho exposes that value. The result is recorded as a test case.

Persistent sessions also consume EMQX session minutes while the phone is offline. This is already included in the quota estimate, which assumes every client is connected 24/7.

## Design rules for reliable delivery

Four rules keep each phone's queue far below 1000 messages and guarantee that no event is permanently lost.

**1. Publish only meaningful messages.** Everything published on a topic the phone subscribes to fills its queue while it is offline. Days of offline cover = 1000 ÷ messages per day; at about 50 messages per day, the queue covers about 20 days.

| Topic | Rule | Typical volume |
| --- | --- | --- |
| `alert` | Only real alerts: intrusion, fire/gas, power loss and restoration, hazard cleared | A few per incident |
| `status` | Only when something changes, never periodic; always retained | A few per day |
| `event` | Arm, disarm, wrong PIN, PIN change; never individual keypad digits or beeps | About 10–30 per day |
| `connection` | Online and offline (Last Will) only | Rare |

**2. Persistent session on the app.** Each phone connects with a fixed client ID, Clean Start = 0, QoS 1 subscriptions and the longest Session Expiry Interval the broker accepts.

**3. Sync from `EVENT_LOG`.** The app recovers anything the broker dropped by asking the Pi directly:

1. Every event carries an increasing `event_id` (the `EVENT_LOG` primary key), which the app stores with the event.
2. On every connection, the app publishes on `cmd`: `{"type": "get_events", "since": <last event_id>}`.
3. The Pi answers on `response` with all later events, in batches of about 50 events per message, staying under the 10 publishes/s limit.
4. If the Pi is offline, the app waits for "online" on `connection` and sends the request again.

Each phone keeps its own last `event_id`, so the Pi stores no per-phone state. The sync uses the existing `cmd` and `response` topics, so the ACL rules do not change.

**4. Ignore duplicates.** QoS 1 delivery and the sync can both deliver the same event twice. The app discards any `event_id` it already has.

## Mosquitto as the local test broker

Mosquitto runs on a development laptop and is used only for development and testing. The deployed system and the final demo use EMQX. Mosquitto is needed because a managed cloud broker cannot be switched off on demand, and several requirements are about what happens when the broker disappears.

&#91;embedded content: production vs. test setup · same device code\]

The device code is identical in both setups; only `broker.conf` decides which broker it connects to.

The local broker mirrors the EMQX setup: TLS on port 8883 with our own test CA, a password file with the same two usernames, and an ACL file with the same rules. Its verbose log shows every CONNECT, PUBLISH and PUBACK. Its `mosquitto_pub` and `mosquitto_sub` command-line clients play the role of the mobile app before the app exists. These clients also work against EMQX.

| Test case | How Mosquitto is used | Requirement verified |
| --- | --- | --- |
| Broker loss with Wi-Fi still up | Stop the Mosquitto service while the Pi stays on Wi-Fi | Connectivity monitoring: the RGB LED shows broker loss within the keep-alive bound, and alerts go to `PENDING_NOTIFICATION` |
| Ordered delivery after restoration | Trigger several events while the broker is stopped, then restart it | Pending notifications: `mosquitto_sub` receives each event once, in chronological order, and rows are marked SENT |
| Resend pacing | Create a backlog of about 50 events, then restore the broker | The resend rate stays within the EMQX limit of 10 publishes/s per client |
| Last Will | Cut the Pi's Wi-Fi or power without a clean disconnect | The broker publishes "offline" on `connection` after 1.5 × the keep-alive interval |
| Remote commands before the app exists | Send arm, disarm and PIN-change JSON with `mosquitto_pub` and watch `response` | Remote control, including disarm rejected while a fire alarm is active |
| ACL enforcement | Try to publish on `alert` with the app credential | Least privilege: the broker rejects the publish |
| TLS validation | Present a certificate from the wrong CA, or an expired one | Security: the device refuses to connect |
| Local latency baseline | Compare the alert's publish timestamp with its arrival at `mosquitto_sub` on the LAN | Real-time responsiveness, measured without internet delay |

After passing locally, the key cases are repeated against EMQX: connection, ACL enforcement, end-to-end alert latency over the internet, and ordered delivery. On EMQX, broker loss is simulated with a firewall rule (iptables) on the Pi that drops traffic to the broker's address.

## Broker-agnostic implementation

Switching between EMQX and Mosquitto is a configuration change, not a code change. Both brokers implement standard MQTT 5, so the same Paho C++ client, topic tree and JSON messages work with both. The CommunicationThread reads its connection settings from a file on the device at startup:

```ini
[broker]
host        = <emqx-serverless-host>   ; or the laptop's IP for Mosquitto
port        = 8883
ca_file     = /etc/vigil/certs/ca.pem
client_id   = vigil-device-home1
username    = vigil-device
secret_file = /etc/vigil/secrets/mqtt_pass   ; readable only by the service (chmod 600)
keepalive_s = 30
```

This also answers the lock-in concern. If EMQX's free tier changes, VigilSystem can move to a self-hosted Mosquitto on a small VPS by editing this file and copying the ACL rules.

## Communication stack

Both ends speak MQTT 5 over TLS on port 8883. The Pi uses the Paho C++ library on top of Paho C and OpenSSL; the phone uses `mqtt5_client`.

| Layer | Raspberry Pi (VigilSystem) | Phone (Flutter app) |
| --- | --- | --- |
| Application | C++ application (`CommunicationThread`) | Flutter app (Dart) |
| MQTT library | Eclipse Paho MQTT C++ → Paho MQTT C | `mqtt5_client` |
| Protocol | MQTT 5.0 | MQTT 5.0 |
| Security | TLS 1.2/1.3 (OpenSSL); server certificate and hostname verified | TLS 1.2/1.3 (Dart secure sockets) |
| Transport | TCP, port 8883 | TCP, port 8883 |
| Network | IPv4 (Linux kernel) | IPv4/IPv6 (Android) |
| Link | IEEE 802.11 + WPA2 (wpa\_supplicant) | Wi-Fi or 4G/5G |
| Driver / hardware | brcmfmac + CYW43455 firmware (SDIO) | Phone's own |

**MQTT 5 settings that differ per side:**

- **The app sets a Message Expiry Interval on every `cmd` message (e.g. 30 s).** If the Pi is offline, the broker discards the command instead of delivering it hours later; a stale disarm must never run.
- **The Pi connects with a clean session (or a very short session expiry).** It never inherits queued commands from an earlier connection.
- **The app uses a persistent session,** as described in the delivery rules above.

**Supporting services on the Pi:** the DHCP client gets an address, DNS resolves the EMQX hostname, NTP sets the clock and the CA bundle lets OpenSSL verify EMQX's certificate. The Pi 4 has no hardware clock, so NTP must run before the TLS handshake or the certificate looks invalid.

**Boot order:** load `brcmfmac` → start `wpa_supplicant` on `wlan0` → `udhcpc -i wlan0` → NTP sync → start VigilSystem, which then does the TLS handshake and MQTT CONNECT with the Last Will.

## Buildroot packages (2026.08.1)

Every component of the device stack is available in Buildroot 2026.08.1, the version used by the project. The check was made against that release's package tree.

| Component | Buildroot package | Notes |
| --- | --- | --- |
| Paho MQTT C | `paho-mqtt-c` (1.3.16) | Supports MQTT 5 |
| Paho MQTT C++ | `paho-mqtt-cpp` (1.3.2) | Selects Paho C automatically; needs a C++ toolchain (the Bootlin toolchain of the Pi 4 defconfig has one) |
| TLS | `openssl` | Also switches TLS on in Paho |
| CA certificates | `ca-certificates` | Lets OpenSSL verify EMQX's certificate |
| WPA2/WPA3 | `wpa_supplicant` | nl80211 and WPA3 options available |
| Wi-Fi firmware | `brcmfmac_sdio-firmware-rpi`, `-wifi` sub-option | Includes the Pi 4 Model B files for the CYW43455 |
| Wi-Fi driver | Pi kernel `bcm2711` defconfig | `CONFIG_BRCMFMAC=m` (module) |
| DHCP client | BusyBox `udhcpc` | Enabled by default |
| NTP | `chrony` (chosen); `ntp` or BusyBox `ntpd` also possible | BusyBox `ntpd` is off by default; chrony can step the clock at boot |
| Local database / JSON | `sqlite`, `json-for-modern-cpp` |  |

**Points to watch:**

- **The stock `raspberrypi4_64_defconfig` has no Wi-Fi.** It enables neither the Wi-Fi firmware nor wpa\_supplicant, and runs DHCP only on `eth0`.
- **Paho has no TLS option of its own.** Its build enables SSL only when OpenSSL is selected; without OpenSSL, connections to port 8883 fail.
- **`brcmfmac` is a module.** It is loaded at boot by an init script (`modprobe brcmfmac`) or by device management that loads modules (eudev or mdev).
- **There is no `fake-hwclock` package.** A small script saves the date to a file periodically and restores it at boot, unless an RTC module is added.

Defconfig fragment added to the project configuration:

```
BR2_PACKAGE_OPENSSL=y
BR2_PACKAGE_CA_CERTIFICATES=y
BR2_PACKAGE_PAHO_MQTT_C=y
BR2_PACKAGE_PAHO_MQTT_CPP=y
BR2_PACKAGE_WPA_SUPPLICANT=y
BR2_PACKAGE_WPA_SUPPLICANT_NL80211=y
BR2_PACKAGE_WPA_SUPPLICANT_CLI=y
BR2_PACKAGE_BRCMFMAC_SDIO_FIRMWARE_RPI=y
BR2_PACKAGE_BRCMFMAC_SDIO_FIRMWARE_RPI_WIFI=y
BR2_PACKAGE_CHRONY=y
BR2_PACKAGE_SQLITE=y
BR2_PACKAGE_JSON_FOR_MODERN_CPP=y
```

## References

Prices and limits as published on 25 October 2026; re-check before submission.

- [EMQX Cloud – Quotas and Limits](https://docs.emqx.com/en/cloud/latest/create/restriction.html)
- [EMQX Cloud – Pricing and Billing](https://docs.emqx.com/en/cloud/latest/price/pricing.html)
- [EMQX Cloud – Default Authorization](https://docs.emqx.com/en/cloud/latest/deployments/default_authz.html)
- [EMQX Cloud – Client Authentication](https://docs.emqx.com/en/cloud/latest/deployments/auth_overview.html)
- [HiveMQ – Access Management Options per Cloud Plan](https://docs.hivemq.com/hivemq-platform/connect/access-management-options-per-cloud-plan.html)
- [HiveMQ – Pricing](https://www.hivemq.com/pricing/)
- [HiveMQ Community – Free plan limits](https://community.hivemq.com/t/any-limit-on-message-per-minute-hour-per-connection/2410)
- [HiveMQ Community – Retained message expiry on the free plan](https://community.hivemq.com/t/free-pay-as-you-go-data-retention-retained-messages/2238)
- [AWS IoT Core – Pricing](https://aws.amazon.com/iot-core/pricing/)
- [AWS IoT Core – MQTT](https://docs.aws.amazon.com/iot/latest/developerguide/mqtt.html)
- [AWS IoT Core – Custom authorizers](https://docs.aws.amazon.com/iot/latest/developerguide/config-custom-auth.html)
- [mosquitto.conf(5) manual page](https://www.mankier.com/5/mosquitto.conf)
