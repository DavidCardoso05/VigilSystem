#!/usr/bin/env bash
# Publishes a cmd sample as the app would: QoS 1, not retained, Message Expiry 30 s (guide §10.11).
# usage: cloud/send-cmd.sh cloud/samples/cmd-arm.json [env file, default secrets/local.env]
#        NO_EXPIRY=1 cloud/send-cmd.sh ...   (send without Message Expiry: the Pi must ignore it)
set -euo pipefail
file=${1:?usage: $0 <sample.json> [env file]}
env=${2:-secrets/local.env}
# shellcheck disable=SC1090
source "$env"
expiry=(-D publish message-expiry-interval 30)
[[ -n "${NO_EXPIRY:-}" ]] && expiry=()
exec mosquitto_pub -V mqttv5 -h "$HOST" -p "$PORT" --cafile "$CAFILE" \
  -u "$APP_USER" -P "$(cat "$APP_PASS_FILE")" -i vigil-test-app1 -q 1 \
  -t vigil/home1/cmd "${expiry[@]}" -f "$file"
