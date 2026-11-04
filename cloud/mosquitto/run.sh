#!/usr/bin/env bash
# Starts the local test broker in the foreground (Ctrl+C stops it = "broker lost" test).
set -euo pipefail
cd "$(dirname "$0")"
[[ -f certs/server.crt && -f passwd ]] || { echo "Run cloud/mosquitto/setup.sh first."; exit 1; }
exec mosquitto -c mosquitto.conf
