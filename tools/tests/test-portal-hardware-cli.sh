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
export XDG_STATE_HOME="$tmp/state"

printf '%s\n' '#!/usr/bin/env bash' \
'if [[ "$1" == "link" && "$2" == "show" ]]; then exit 0; fi' \
'if [[ "$1" == "route" && "$2" == "show" ]]; then echo "default via 192.0.2.1 dev wlan-main"; exit 0; fi' \
'echo "192.168.4.1 dev wlan-client src 192.168.4.2"' >"$stub_bin/ip"
printf '%s\n' '#!/usr/bin/env bash' \
'printf "%s\\n" "$*" >>"$CALL_LOG"' \
'if [[ "${NMCLI_FAIL_UP:-}" == "yes" && "$1" == "connection" && "$2" == "up" ]]; then exit 7; fi' \
'if [[ "$1" == "-t" && "$2" == "-f" && "$3" == "SSID" ]]; then echo "WM Contract ESP8266"; exit 0; fi' \
'if [[ "$1" == "-g" && "$2" == "connection.uuid" ]]; then echo "stub-uuid"; exit 0; fi' \
'if [[ "$1" == "-g" ]]; then echo "--"; fi' >"$stub_bin/nmcli"
printf '%s\n' '#!/usr/bin/env bash' 'exit 0' >"$stub_bin/pio"
printf '%s\n' '#!/usr/bin/env bash' \
'printf "docker %s\n" "$*" >>"$CALL_LOG"' \
'if [[ "$1" == "compose" && "$2" == "version" ]]; then echo "Docker Compose"; exit 0; fi' \
'exit 0' >"$stub_bin/docker"
printf '%s\n' '#!/usr/bin/env bash' \
'printf "{\"state\":\"complete\",\"results_valid\":true}"' >"$stub_bin/curl"
chmod 755 "$stub_bin"/*
export PATH="$stub_bin:$PATH"

"$root/tools/portal-hardware" doctor --client-interface wlan-client >/dev/null
if grep -Eq 'connection (add|modify|delete)|device disconnect' "$CALL_LOG"; then
    echo 'doctor unexpectedly changed a NetworkManager connection' >&2
    exit 1
fi

if "$root/tools/portal-hardware" doctor --client-interface wlan-main >/dev/null 2>&1; then
    echo 'default-route adapter guard did not reject the request' >&2
    exit 1
fi
if "$root/tools/portal-hardware" up --platform >/dev/null 2>&1; then
    echo 'missing option value did not reject the request' >&2
    exit 1
fi
if "$root/tools/portal-hardware" run --platform esp8266 --port /dev/null \
    --client-interface wlan-client --capture-readme-media >/dev/null 2>&1; then
    echo 'ESP8266 README media capture was accepted' >&2
    exit 1
fi
if "$root/tools/portal-hardware" run --platform esp32 --port /dev/null \
    --client-interface wlan-client --custom-parameter-stress >/dev/null 2>&1; then
    echo 'ESP32 custom-parameter stress was accepted' >&2
    exit 1
fi
if "$root/tools/portal-hardware" run --platform esp8266 --port /dev/null \
    --client-interface wlan-client --browser skip --custom-parameter-stress >/dev/null 2>&1; then
    echo 'browser-skipped custom-parameter stress was accepted' >&2
    exit 1
fi
if "$root/tools/portal-hardware" ota --platform esp8266 --port /dev/null \
    --client-interface wlan-client --browser skip >/dev/null 2>&1; then
    echo 'browser-skipped OTA was accepted' >&2
    exit 1
fi
if "$root/tools/portal-hardware" ota --platform esp8266 --port /dev/null \
    --client-interface wlan-client --station-env "$root/test/portal-station.env.example" >/dev/null 2>&1; then
    echo 'station handoff inputs were accepted by portal OTA' >&2
    exit 1
fi
if "$root/scripts/test.sh" ota-fixtures --platform esp32 >/dev/null 2>&1; then
    echo 'legacy-cache ESP32 OTA-fixture selector was accepted' >&2
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

# A retained session must be removed explicitly, never silently overwritten.
wm_write_state wlan-client esp8266 stale-uuid stale-name
wm_load_state
[[ "$WM_PORTAL_INTERFACE" == "wlan-client" && "$WM_PORTAL_PLATFORM" == "esp8266" ]]
[[ "$WM_PORTAL_CONNECTION_UUID" == "stale-uuid" && "$WM_PORTAL_CONNECTION_NAME" == "stale-name" ]]
mutations_before="$(grep -Ec '^(device disconnect|connection (add|modify|delete|down))' "$CALL_LOG" || true)"
if "$root/tools/portal-hardware" up --platform esp8266 --port /dev/null \
    --client-interface wlan-client --take-over-client-adapter >/dev/null 2>&1; then
    echo 'stale portal session was silently overwritten' >&2
    exit 1
fi
mutations_after="$(grep -Ec '^(device disconnect|connection (add|modify|delete|down))' "$CALL_LOG" || true)"
[[ "$mutations_before" == "$mutations_after" ]] || {
    echo 'stale portal session mutated the selected adapter' >&2
    exit 1
}
wm_clear_state

# The runner must build the copied contract source before it starts the container.
"$root/tools/portal-hardware" run --platform esp8266 --port /dev/null --client-interface wlan-client --browser skip >/dev/null
build_line="$(grep -n " build portal-contract$" "$CALL_LOG" | tail -1 | cut -d: -f1)"
run_line="$(grep -n " run --rm portal-contract$" "$CALL_LOG" | tail -1 | cut -d: -f1)"
[[ -n "$build_line" && -n "$run_line" && "$build_line" -lt "$run_line" ]] || {
    echo "portal contract was not rebuilt before execution" >&2
    exit 1
}
grep -Fq 'wait_for_portal_ready' "$root/tools/portal-hardware"
grep -Fq 'api/wifi/scan-status' "$root/tools/portal-hardware"
grep -Fq 'PORTAL_CUSTOM_PARAMETER_STRESS' "$root/tools/portal-hardware"
grep -Fq 'compose.ota.yaml' "$root/tools/portal-hardware"
grep -Fq 'wait_for_ota_marker B' "$root/tools/portal-hardware"
grep -Fq 'pio_for_portal_environment "$ota_environment_a"' "$root/tools/portal-hardware"
grep -Fq 'WIFIMANAGER_PLATFORMIO_CORE_DIR' "$root/tools/portal-hardware"
grep -Fq 'assert_ota_fixture_pair' "$root/scripts/test.sh"
grep -Fq 'WiFiManager Unity compile check passed' "$root/scripts/test.sh"
grep -Fq 'eagle.flash.4m1m.ld' "$root/test/portal-harness/platformio.ini"
grep -Fq 'esp32_ota_4m_no_fs.csv' "$root/test/portal-harness/platformio.ini"
grep -Fq 'README media GIF exceeds its 2 MiB documentation budget' \
    "$root/tests/portal-contract/render-readme-media.sh"

echo 'portal-hardware CLI safety checks passed'
