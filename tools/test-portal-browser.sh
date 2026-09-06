#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'EOF' >&2
Usage:
  ./tools/test-portal-browser.sh \
    --platform esp8266|esp32 \
    --port /dev/ttyUSB... \
    --wifi-interface wlx... \
    [--output /absolute/output-directory]
EOF
    exit 2
}

platform=""
port=""
wifi_interface=""
output_dir=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --platform) [[ $# -ge 2 ]] || usage; platform="$2"; shift 2 ;;
        --port) [[ $# -ge 2 ]] || usage; port="$2"; shift 2 ;;
        --wifi-interface) [[ $# -ge 2 ]] || usage; wifi_interface="$2"; shift 2 ;;
        --output) [[ $# -ge 2 ]] || usage; output_dir="$2"; shift 2 ;;
        *) usage ;;
    esac
done

[[ "$platform" == "esp8266" || "$platform" == "esp32" ]] || usage
[[ -e "$port" ]] || { echo "Serial port not found: $port" >&2; exit 1; }
ip link show "$wifi_interface" >/dev/null 2>&1 || {
    echo "Wi-Fi interface not found: $wifi_interface" >&2
    exit 1
}
command -v nmcli >/dev/null || { echo "nmcli is required" >&2; exit 1; }
command -v curl >/dev/null || { echo "curl is required" >&2; exit 1; }
command -v rg >/dev/null || { echo "rg is required" >&2; exit 1; }

browser_bin=""
for candidate in google-chrome google-chrome-stable chromium chromium-browser; do
    if command -v "$candidate" >/dev/null; then
        browser_bin="$candidate"
        break
    fi
done

default_route_interface="$(ip route show default | awk '/^default/{print $5; exit}')"
[[ "$wifi_interface" != "$default_route_interface" ]] || {
    echo "Refusing to use the host default-route interface: $wifi_interface" >&2
    exit 1
}

case "$platform" in
    esp8266) portal_ssid="WM Browser ESP8266" ;;
    esp32) portal_ssid="WM Browser ESP32" ;;
esac
portal_password="default1"

if [[ -z "$output_dir" ]]; then
    output_dir="$(mktemp -d /tmp/wifimanager-browser.XXXXXX)"
else
    mkdir -p "$output_dir"
fi

temporary_connection=""
cleanup() {
    if [[ -n "$temporary_connection" ]]; then
        nmcli connection down "$temporary_connection" >/dev/null 2>&1 || true
        nmcli connection delete "$temporary_connection" >/dev/null 2>&1 || true
    fi
}
trap cleanup EXIT

script_dir="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$script_dir/.." && pwd)"

# Flash only the explicitly selected idle serial board.
pio run -d "$root/test/portal-harness" \
    -e "$platform" -t upload --upload-port "$port"

# The secondary adapter is the only adapter NetworkManager may touch.
nmcli device disconnect "$wifi_interface" >/dev/null 2>&1 || true
nmcli device wifi rescan ifname "$wifi_interface" || true
for attempt in $(seq 1 30); do
    if nmcli -t -f SSID device wifi list ifname "$wifi_interface" | grep -Fxq "$portal_ssid"; then
        break
    fi
    sleep 1
    nmcli device wifi rescan ifname "$wifi_interface" >/dev/null 2>&1 || true
done
nmcli -t -f SSID device wifi list ifname "$wifi_interface" | grep -Fxq "$portal_ssid" || {
    echo "Portal SSID not detected on $wifi_interface: $portal_ssid" >&2
    exit 1
}

# Apply never-default before bringing this temporary portal connection up.
temporary_connection="wifimanager-browser-$platform-$$"
nmcli connection add type wifi ifname "$wifi_interface" con-name "$temporary_connection" \
    ssid "$portal_ssid" ipv4.method auto ipv4.never-default yes ipv6.method ignore >/dev/null
nmcli connection modify "$temporary_connection" wifi-sec.key-mgmt wpa-psk \
    wifi-sec.psk "$portal_password"
nmcli connection up "$temporary_connection" ifname "$wifi_interface" >/dev/null

portal_url="http://192.168.4.1"
curl --silent --show-error --fail --interface "$wifi_interface" --connect-timeout 3 \
    "$portal_url/" >"$output_dir/portal.html"
rg -iq '<html' "$output_dir/portal.html"

# Exercise two independent low-priority responses at once before the scan flow.
curl --silent --show-error --fail --interface "$wifi_interface" --connect-timeout 3 \
    "$portal_url/api/bootstrap" >"$output_dir/bootstrap-concurrent.json" &
bootstrap_pid="$!"
curl --silent --show-error --fail --interface "$wifi_interface" --connect-timeout 3 \
    "$portal_url/" >"$output_dir/portal-concurrent.html" &
portal_pid="$!"
wait "$bootstrap_pid"
wait "$portal_pid"
rg -Fq '"contractVersion":3' "$output_dir/bootstrap-concurrent.json"
rg -iq '<html' "$output_dir/portal-concurrent.html"
for attempt in $(seq 1 30); do
    if curl --silent --show-error --fail --interface "$wifi_interface" --connect-timeout 2 \
        "$portal_url/api/bootstrap" >"$output_dir/bootstrap.json"; then
        break
    fi
    sleep 1
done
rg -Fq '"contractVersion":3' "$output_dir/bootstrap.json"

curl --silent --show-error --fail --interface "$wifi_interface" --connect-timeout 3 \
    -X POST "$portal_url/api/wifi/scan" >"$output_dir/scan-start.json"
for attempt in $(seq 1 30); do
    curl --silent --show-error --fail --interface "$wifi_interface" --connect-timeout 3 \
        "$portal_url/api/wifi/scan-status" >"$output_dir/scan-status.json"
    if ! rg -Fq '"scanning":true' "$output_dir/scan-status.json"; then
        break
    fi
    sleep 1
done
rg -Fq '"state":"complete"' "$output_dir/scan-status.json"

if [[ -n "$browser_bin" ]]; then
    "$browser_bin" --headless=new --disable-gpu --no-first-run --enable-logging=stderr \
        --window-size=1440,1100 --screenshot="$output_dir/portal.png" "$portal_url" \
        >"$output_dir/browser.log" 2>&1
    ! rg -i "console.*error|uncaught|exception" "$output_dir/browser.log"
else
    echo "HTTP portal checks passed; no compatible local Chromium binary for screenshot capture" \
        >"$output_dir/browser.log"
fi

echo "WiFiManager portal browser test passed"
echo "Artifacts: $output_dir"
