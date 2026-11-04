#!/usr/bin/env bash
# Creates the test certificates for the local Mosquitto broker in cloud/mosquitto/certs/.
#   ca.crt / ca.key          test CA (the Pi and the app trust ca.crt)
#   server.crt / server.key  broker certificate, valid for localhost, this PC's name and its IPs
#   wrong-ca.crt             an unrelated CA, used by the "TLS validation" test
# Extra host names or IPs (e.g. the IP the Pi will use): ./make-certs.sh 192.168.1.50 mylaptop.local
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p certs && cd certs

if [[ -f server.crt && "${FORCE:-0}" != 1 ]]; then
  echo "certs/ already exists. Run with FORCE=1 to recreate."; exit 0
fi

# Names the certificate must cover (the client checks the host name it connects to)
SAN="DNS:localhost,IP:127.0.0.1,DNS:$(hostname)"
for ip in $(hostname -I); do SAN="$SAN,IP:$ip"; done
for extra in "$@"; do
  if [[ $extra =~ ^[0-9.]+$ ]]; then SAN="$SAN,IP:$extra"; else SAN="$SAN,DNS:$extra"; fi
done

# 1. Test CA (10 years)
openssl req -x509 -newkey rsa:2048 -nodes -days 3650 -sha256 \
  -keyout ca.key -out ca.crt -subj "/O=VigilSystem/CN=VigilSystem Test CA" 2>/dev/null

# 2. Broker certificate signed by the test CA (2 years)
openssl req -newkey rsa:2048 -nodes -keyout server.key -out server.csr \
  -subj "/O=VigilSystem/CN=localhost" 2>/dev/null
printf "subjectAltName=%s\nextendedKeyUsage=serverAuth\nbasicConstraints=CA:FALSE\n" "$SAN" > server.ext
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
  -days 730 -sha256 -extfile server.ext -out server.crt 2>/dev/null
rm -f server.csr server.ext

# 3. Unrelated CA for the negative TLS test
openssl req -x509 -newkey rsa:2048 -nodes -days 3650 -sha256 \
  -keyout wrong-ca.key -out wrong-ca.crt -subj "/O=Nobody/CN=Wrong CA" 2>/dev/null
rm -f wrong-ca.key

chmod 600 ./*.key
echo "Certificates created in cloud/mosquitto/certs/"
echo "Server certificate valid for: $SAN"
