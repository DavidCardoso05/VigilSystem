# Cloud setup guide: EMQX Serverless + local Mosquitto

Two brokers with **the same users, the same ACL and the same port (8883, TLS)**:

| Broker | Used for | How it's set up |
|--------|----------|-----------------|
| **EMQX Serverless** | The real system and the final demo | By hand in the web console (Part A, ~20 min) |
| **Local Mosquitto** | Development and every "broker lost" test | **One script** (Part B, ~1 min) |

Both are checked by the same script, `cloud/test-broker.sh`: 13 checks covering TLS, passwords, ACL, retained status, commands and Last Will.

All commands are run from the project root (`~/VigilSystem`). Do Part B first: it's quick and it creates `wrong-ca.crt`, which the EMQX test also uses.

---

## Part B: local Mosquitto (automatic)

```bash
cloud/mosquitto/setup.sh
```
Creates the test certificates, random passwords for both users (in `secrets/local-*.pass`), the password file and `secrets/local.env`. It's safe to run again: it keeps existing certificates and passwords.

If the Pi will connect to this laptop, the certificate must include the IP the Pi uses. Your current IPs are added automatically. To add another IP or name, recreate the certificates:
```bash
FORCE=1 cloud/mosquitto/setup.sh 192.168.1.50
```

Start the broker (leave this terminal open; its log shows every CONNECT, PUBLISH, PUBACK and denial):
```bash
cloud/mosquitto/run.sh
```

In a second terminal:
```bash
cloud/test-broker.sh secrets/local.env
```
Expected result: `13 passed, 0 failed` (takes about 30 s).

**Broker-lost test later:** press **Ctrl+C** in the broker terminal. That simulates the broker going down while Wi-Fi stays up.

---

## Part A: EMQX Serverless (by hand in the console)

The console's menu names may differ slightly from the ones below; the settings are what matter.

### A1. Account and deployment
1. Sign up at **https://www.emqx.com/en/cloud** (free, no credit card) and confirm your email.
2. **New Deployment** → **Serverless**.
3. Region: the one closest to Portugal (e.g. Frankfurt / eu-central-1).
4. **Spend limit: 0**. The deployment then pauses instead of charging you if the free quota ever runs out (expected use is about 13 %).
5. Name it `vigil-home1` → **Deploy**. Wait until the status is *Running* (1–2 min).

### A2. Connection details and CA certificate
On the deployment's **Overview** page:
1. Copy the **address** (something like `xxxxxxxx.ala.eu-central-1.emqxsl.com`). Use **MQTT over TLS, port 8883**.
2. Download the **CA certificate** and save it as `secrets/emqx-ca.pem`:
   ```bash
   mv ~/Transferências/<downloaded file>.crt secrets/emqx-ca.pem
   ```
3. Create the test config:
   ```bash
   cp cloud/emqx/emqx.env.example secrets/emqx.env
   ```
   Then open `secrets/emqx.env` and set `HOST=` to your address.

### A3. Users (Access Control → Authentication)
Generate two strong passwords. Each command prints the password and saves it:
```bash
openssl rand -base64 24 | tr -d '\n' | tee secrets/vigil-device.pass; echo
```
```bash
openssl rand -base64 24 | tr -d '\n' | tee secrets/vigil-app.pass; echo
```
```bash
chmod 600 secrets/*.pass
```
In the console, **+ Add** two users and paste each password:
- Username `vigil-device`, password from `secrets/vigil-device.pass`
- Username `vigil-app`, password from `secrets/vigil-app.pass`

These are separate from the local Mosquitto passwords, on purpose.

### A4. Topic rules (Access Control → Authorization)
Add the **13 rules** from [`emqx/acl-rules.md`](emqx/acl-rules.md):
- 6 rules for username `vigil-device`
- 6 rules for username `vigil-app`
- **last: All users, topic `#`, Publish & Subscribe, Deny**

Don't skip the deny-all rule. Serverless allows anything not matched, and the test script will flag it if it's missing.

### A5. Test it
```bash
SUB_WAIT=3 cloud/test-broker.sh secrets/emqx.env
```
Expected result: `13 passed, 0 failed`. `SUB_WAIT=3` gives each subscriber more time over the internet.

If a check fails:

| Failure | Likely cause |
|---------|--------------|
| "cannot connect" + certificate error | Wrong CA file or wrong host name. Use the exact address from the Overview page. Check that the PC clock is right |
| "cannot connect" + not authorised | Username or password typo. Check that the `.pass` file has no newline or extra space |
| "could PUBLISH/SUBSCRIBE …" | A rule is missing or has the wrong action |
| "topics outside the tree are open" | The deny-all rule is missing |
| "did not receive" | Slow link: retry with `SUB_WAIT=5` |

### A6. One open point (from the Word file)
Find out the **longest session expiry EMQX accepts** for the app. The app asks for 7 days. Command-line tools don't show the value the broker sends back, so check it when the Flutter (or Paho) client connects, then record it as a test case.

---

## Files

| File | In git? | What it is |
|------|---------|------------|
| `mosquitto/mosquitto.conf`, `mosquitto/acl` | ✅ | Local broker config and rules |
| `mosquitto/setup.sh`, `make-certs.sh`, `run.sh` | ✅ | Setup and start scripts |
| `mosquitto/certs/`, `mosquitto/passwd` | ❌ | Generated keys, certificates and password hashes |
| `emqx/acl-rules.md`, `emqx/emqx.env.example` | ✅ | EMQX rules and config template |
| `test-broker.sh` | ✅ | Test script for either broker |
| `../secrets/*` | ❌ | Passwords, EMQX CA, `local.env`, `emqx.env` |
