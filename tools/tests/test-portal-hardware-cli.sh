#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
tmp="$(mktemp -d "${TMPDIR:-/tmp}/wifimanager-portal-cli.XXXXXX")"
cleanup() { rm -rf "$tmp"; }
trap cleanup EXIT

stub_bin="$tmp/bin"
mkdir -p "$stub_bin"
export CALL_LOG="$tmp/calls.log"
export WM_HARDWARE_LOCK_FILE="$tmp/hardware.lock"

printf '%s\n' '#!/usr/bin/env bash' \
'if [[ "$1" == "link" && "$2" == "show" ]]; then exit 0; fi' \
'if [[ "$1" == "route" && "$2" == "show" ]]; then echo "default via 192.0.2.1 dev wlan-main"; exit 0; fi' \
'echo "192.168.4.1 dev wlan-client src 192.168.4.2"' >"$stub_bin/ip"
printf '%s\n' '#!/usr/bin/env bash' \
'printf "%s\\n" "$*" >>"$CALL_LOG"' \
'if [[ "${NMCLI_FAIL_UP:-}" == "yes" && "$1" == "connection" && "$2" == "up" ]]; then exit 7; fi' \
'if [[ "$1" == "-g" ]]; then echo "--"; fi' >"$stub_bin/nmcli"
printf '%s\n' '#!/usr/bin/env bash' 'exit 0' >"$stub_bin/pio"
printf '%s\n' '#!/usr/bin/env bash' \
'if [[ "$1" == "compose" && "$2" == "version" ]]; then echo "Docker Compose"; exit 0; fi' \
'exit 0' >"$stub_bin/docker"
chmod 755 "$stub_bin"/*
export PATH="$stub_bin:$PATH"

"$root/tools/portal-hardware" doctor --client-interface wlan-client >/dev/null
! grep -Eq 'connection (add|modify|delete)|device disconnect' "$CALL_LOG"

if "$root/tools/portal-hardware" doctor --client-interface wlan-main >/dev/null 2>&1; then
    echo 'default-route adapter guard did not reject the request' >&2
    exit 1
fi
if "$root/tools/portal-hardware" up --platform >/dev/null 2>&1; then
    echo 'missing option value did not reject the request' >&2
    exit 1
fi

# A failed association must delete the only connection it just created.
source "$root/tools/lib/portal-hardware-session.sh"
wm_wait_for_portal_ssid() { return 0; }
export NMCLI_FAIL_UP=yes
if wm_create_portal_connection wlan-client 'fixture portal' placeholder; then
    echo 'failed association was reported as success' >&2
    exit 1
fi
unset NMCLI_FAIL_UP
grep -Eq 'connection delete wifimanager-portal-' "$CALL_LOG"
! grep -Eq 'connection (add|modify|delete)|device disconnect' "$CALL_LOG"

echo 'portal-hardware CLI safety checks passed'
