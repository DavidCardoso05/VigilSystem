#!/usr/bin/env bash
# Checks a VigilSystem broker (EMQX or the local Mosquitto) against the design:
# TLS, authentication, ACL (least privilege), retained status, cmd with Message Expiry, Last Will.
#
#   cloud/test-broker.sh secrets/local.env     # local Mosquitto (start cloud/mosquitto/run.sh first)
#   cloud/test-broker.sh secrets/emqx.env      # EMQX Serverless
#
# Uses its own test client IDs (vigil-test-*), so a running Pi or app is not kicked off.
# At the end it clears the retained test messages on status and connection.
set -uo pipefail

ENV_FILE="${1:-}"
[[ -f $ENV_FILE ]] || { echo "Usage: $0 <env file>   (e.g. secrets/local.env)"; exit 2; }
# shellcheck disable=SC1090
source "$ENV_FILE"
for v in HOST PORT CAFILE DEVICE_USER DEVICE_PASS_FILE APP_USER APP_PASS_FILE; do
  [[ -n ${!v:-} ]] || { echo "Missing $v in $ENV_FILE"; exit 2; }
done
[[ -f $CAFILE ]] || { echo "CA file not found: $CAFILE"; exit 2; }
DEVICE_PASS="$(cat "$DEVICE_PASS_FILE")"
APP_PASS="$(cat "$APP_PASS_FILE")"
T=vigil/home1
WAIT="${SUB_WAIT:-2}"          # seconds to let a subscriber get ready (raise it for slow links)
TMP="$(mktemp -d)"; trap 'kill $(jobs -p) 2>/dev/null; rm -rf "$TMP"' EXIT
PASS=0; FAIL=0
ok()   { echo "  ✅ $1"; PASS=$((PASS+1)); }
bad()  { echo "  ❌ $1"; FAIL=$((FAIL+1)); }

common=(-V mqttv5 -h "$HOST" -p "$PORT" --cafile "$CAFILE")
as_device=(-u "$DEVICE_USER" -P "$DEVICE_PASS")
as_app=(-u "$APP_USER" -P "$APP_PASS")

# sub_bg <outfile> <timeout_s> <mosquitto_sub args...>: subscribe in background, stop after 1 message
sub_bg() { local out=$1 to=$2; shift 2
  mosquitto_sub "${common[@]}" -C 1 -W "$to" "$@" > "$out" 2>"$out.err" & }

echo "Testing ${NAME:-broker} at $HOST:$PORT"
echo

echo "1. TLS + authentication"
if mosquitto_pub "${common[@]}" "${as_device[@]}" -i vigil-test-dev1 -q 1 -t $T/event -m '{"test":"connect"}' 2>"$TMP/e1"; then
  ok "device user connects over TLS (certificate and host name verified)"
else bad "device user cannot connect: $(cat "$TMP/e1")"; fi
if mosquitto_pub "${common[@]}" "${as_app[@]}" -i vigil-test-app1 -q 1 -t $T/cmd -m '{"test":"connect"}' -D publish message-expiry-interval 30 2>"$TMP/e2"; then
  ok "app user connects over TLS"
else bad "app user cannot connect: $(cat "$TMP/e2")"; fi
if mosquitto_pub "${common[@]}" -u "$APP_USER" -P "wrong-password" -i vigil-test-bad1 -q 1 -t $T/cmd -m x 2>/dev/null; then
  bad "wrong password was ACCEPTED"
else ok "wrong password is refused"; fi
if [[ -n ${WRONG_CAFILE:-} && -f ${WRONG_CAFILE:-} ]]; then
  if mosquitto_pub -V mqttv5 -h "$HOST" -p "$PORT" --cafile "$WRONG_CAFILE" "${as_app[@]}" -i vigil-test-bad2 -q 1 -t $T/cmd -m x 2>/dev/null; then
    bad "connection with the wrong CA was ACCEPTED"
  else ok "server certificate from an untrusted CA is refused"; fi
fi

echo "2. Retained status (device → app)"
STATUS='{"armed_state":"DISARMED","fire_state":"NORMAL","power_source":"GRID","battery_pct":100,"timestamp":"TEST"}'
mosquitto_pub "${common[@]}" "${as_device[@]}" -i vigil-test-dev2 -q 1 -r -t $T/status -m "$STATUS" 2>/dev/null
mosquitto_sub "${common[@]}" "${as_app[@]}" -i vigil-test-app2 -C 1 -W 5 -t $T/status > "$TMP/st" 2>/dev/null
if grep -q '"timestamp":"TEST"' "$TMP/st"; then ok "app receives the retained status when it subscribes"
else bad "app did not receive the retained status"; fi

echo "3. Command with Message Expiry (app → device)"
sub_bg "$TMP/cmd" 8 "${as_device[@]}" -i vigil-test-dev3 -q 1 -t $T/cmd
sleep "$WAIT"
mosquitto_pub "${common[@]}" "${as_app[@]}" -i vigil-test-app3 -q 1 -t $T/cmd \
  -D publish message-expiry-interval 30 -m '{"request_id":"t1","type":"arm"}' 2>/dev/null
wait $! 2>/dev/null
if grep -q '"request_id":"t1"' "$TMP/cmd"; then ok "device receives cmd sent by the app"
else bad "device did not receive the cmd"; fi

echo "4. ACL: least privilege"
# App publishing a fake alert: an app subscriber on alert must NOT get it
sub_bg "$TMP/a1" 5 "${as_app[@]}" -i vigil-test-app4 -t $T/alert
sleep "$WAIT"
mosquitto_pub "${common[@]}" "${as_app[@]}" -i vigil-test-app5 -q 1 -t $T/alert -m '{"fake":"alert"}' >/dev/null 2>&1
wait $! 2>/dev/null
if [[ -s $TMP/a1 ]]; then bad "app user could PUBLISH a fake alert"
else ok "app user cannot publish on alert"; fi

# App faking the device's online message
mosquitto_sub "${common[@]}" "${as_app[@]}" -i vigil-test-app6 -W 5 -t $T/connection > "$TMP/a2" 2>/dev/null &
sleep "$WAIT"
mosquitto_pub "${common[@]}" "${as_app[@]}" -i vigil-test-app7 -q 1 -t $T/connection -m '{"state":"FAKE"}' >/dev/null 2>&1
wait $! 2>/dev/null
if grep -q FAKE "$TMP/a2"; then bad "app user could PUBLISH on connection"
else ok "app user cannot publish on connection"; fi

# Device reading another topic's traffic (it may only read cmd)
sub_bg "$TMP/a4" 5 "${as_device[@]}" -i vigil-test-dev4 -t $T/alert
sleep "$WAIT"
mosquitto_pub "${common[@]}" "${as_device[@]}" -i vigil-test-dev5 -q 1 -t $T/alert -m '{"test":"device-read"}' >/dev/null 2>&1
wait $! 2>/dev/null
if [[ -s $TMP/a4 ]]; then bad "device user could SUBSCRIBE to alert"
else ok "device user cannot subscribe to alert"; fi

# App reading cmd (would expose other phones' PINs)
sub_bg "$TMP/a5" 5 "${as_app[@]}" -i vigil-test-app9 -t $T/cmd
sleep "$WAIT"
mosquitto_pub "${common[@]}" "${as_app[@]}" -i vigil-test-app10 -q 1 -t $T/cmd -D publish message-expiry-interval 30 -m '{"test":"app-read"}' >/dev/null 2>&1
wait $! 2>/dev/null
if [[ -s $TMP/a5 ]]; then bad "app user could SUBSCRIBE to cmd"
else ok "app user cannot subscribe to cmd"; fi

# Topic outside the tree (deny-all rule)
sub_bg "$TMP/a6" 5 "${as_app[@]}" -i vigil-test-app11 -t 'other/#'
sleep "$WAIT"
mosquitto_pub "${common[@]}" "${as_device[@]}" -i vigil-test-dev6 -q 1 -t other/x -m deny-all >/dev/null 2>&1
wait $! 2>/dev/null
if [[ -s $TMP/a6 ]]; then bad "topics outside vigil/home1 are open (deny-all rule missing?)"
else ok "topics outside the tree are blocked (deny-all)"; fi

echo "5. Last Will (device dies without DISCONNECT)"
mosquitto_sub "${common[@]}" "${as_device[@]}" -i vigil-test-dev7 -k 5 -t $T/cmd \
  --will-topic $T/connection --will-payload '{"state":"offline"}' --will-qos 1 --will-retain >/dev/null 2>&1 &
DEV_PID=$!
sleep "$WAIT"
mosquitto_pub "${common[@]}" "${as_device[@]}" -i vigil-test-dev8 -q 1 -r -t $T/connection -m '{"state":"online","timestamp":"TEST"}' 2>/dev/null
# App watches connection (first it gets the retained "online")
mosquitto_sub "${common[@]}" "${as_app[@]}" -i vigil-test-app12 -W 25 -t $T/connection > "$TMP/lw" 2>/dev/null &
APP_PID=$!
sleep "$WAIT"
kill -9 $DEV_PID 2>/dev/null; wait $DEV_PID 2>/dev/null
START=$(date +%s); GOT=""
for _ in $(seq 1 40); do
  grep -q '"offline"' "$TMP/lw" && { GOT=1; break; }
  sleep 0.5
done
kill $APP_PID 2>/dev/null
if [[ -n $GOT ]]; then ok "broker published the Last Will \"offline\" ($(( $(date +%s) - START )) s after the kill)"
else bad "no Last Will within 20 s"; fi

echo "6. Clean-up"
mosquitto_pub "${common[@]}" "${as_device[@]}" -i vigil-test-dev9 -q 1 -r -n -t $T/status 2>/dev/null
mosquitto_pub "${common[@]}" "${as_device[@]}" -i vigil-test-dev9 -q 1 -r -n -t $T/connection 2>/dev/null
ok "retained test messages cleared"

echo
echo "Result: $PASS passed, $FAIL failed"
[[ $FAIL -eq 0 ]]
