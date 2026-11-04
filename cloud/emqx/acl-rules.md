# EMQX Serverless authorization rules (VigilSystem)

Enter these 13 rules in **Access Control → Authorization**. They match `cloud/mosquitto/acl`.

EMQX checks username and client-ID rules first, then "All users" rules, and applies the first match.
Serverless **allows** anything that no rule matches, so rule 13 (deny-all) is mandatory.

| # | Type | Username | Topic | Action | Permission | Why |
|---|------|----------|-------|--------|------------|-----|
| 1 | Username | `vigil-device` | `vigil/home1/status` | Publish | Allow | Retained system state |
| 2 | Username | `vigil-device` | `vigil/home1/alert` | Publish | Allow | Intrusion, fire, power and fault alerts |
| 3 | Username | `vigil-device` | `vigil/home1/event` | Publish | Allow | Log events |
| 4 | Username | `vigil-device` | `vigil/home1/response` | Publish | Allow | Replies to commands |
| 5 | Username | `vigil-device` | `vigil/home1/connection` | Publish | Allow | "online" + Last Will "offline" |
| 6 | Username | `vigil-device` | `vigil/home1/cmd` | Subscribe | Allow | Receive commands |
| 7 | Username | `vigil-app` | `vigil/home1/cmd` | Publish | Allow | Send commands |
| 8 | Username | `vigil-app` | `vigil/home1/status` | Subscribe | Allow | Status card |
| 9 | Username | `vigil-app` | `vigil/home1/alert` | Subscribe | Allow | Alert notifications |
| 10 | Username | `vigil-app` | `vigil/home1/event` | Subscribe | Allow | Recent events list |
| 11 | Username | `vigil-app` | `vigil/home1/response` | Subscribe | Allow | Command results |
| 12 | Username | `vigil-app` | `vigil/home1/connection` | Subscribe | Allow | Device online/offline |
| 13 | AllUsers | — | `#` | Publish & Subscribe | **Deny** | Block everything else |

What this guarantees (checked by `cloud/test-broker.sh`, section 4):
- A leaked **app** password can only send commands. Disarming still needs the PIN, and the app can't fake alerts, status or "online", or read other phones' commands (which carry PINs).
- A leaked **device** password needs physical access to the SD card. Even then it can't read alerts or status.
