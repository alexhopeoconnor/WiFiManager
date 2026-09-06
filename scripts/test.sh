#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'EOF' >&2
Usage:
  ./scripts/test.sh compile  --platform esp8266|esp32
  ./scripts/test.sh examples --platform esp8266|esp32
  ./scripts/test.sh hardware --platform esp8266|esp32 --port /dev/serial/by-id/...
EOF
    exit 2
}

mode="${1:-}"
[[ "$mode" == "compile" || "$mode" == "examples" || "$mode" == "hardware" ]] || usage
shift

platform=""
port=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --platform) [[ $# -ge 2 ]] || usage; platform="${2:-}"; shift 2 ;;
        --port) [[ $# -ge 2 ]] || usage; port="${2:-}"; shift 2 ;;
        *) usage ;;
    esac
done

[[ "$platform" == "esp8266" || "$platform" == "esp32" ]] || usage
[[ "$mode" != "hardware" || -n "$port" ]] || usage
[[ "$mode" != "hardware" || -e "$port" ]] || { echo "Serial port not found: $port" >&2; exit 1; }

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ "$mode" == "examples" ]]; then
    mapfile -t examples < <(find "$root/examples" -mindepth 2 -maxdepth 2 -type f -name platformio.ini -printf '%h\n' | sort)
    if (( ${#examples[@]} == 0 )); then
        echo "No example projects found" >&2
        exit 1
    fi
    for example in "${examples[@]}"; do
        pio run -d "$example" -e "$platform" </dev/null
    done
    echo "WiFiManager examples compile check passed for $platform"
    exit 0
fi

if [[ "$mode" == "hardware" ]]; then
    # Upload first, then capture from the normal boot reset. The Unity sketch
    # deliberately waits two seconds before it begins its test sequence.
    pio test -d "$root" -e "$platform" --filter test_wifimanager \
        --upload-port "$port" --without-testing
    "$root/scripts/capture-unity-serial.sh" --port "$port" --timeout 300
    echo "WiFiManager hardware test passed for $platform on $port"
    exit 0
fi

cached_library="$root/test/compile-project/.pio/libdeps/${platform}/WiFiManager"
# The fixture intentionally declares only this local package. Remove a prior
# link so each check resolves the current manifest as a fresh consumer would.
if [[ -d "$cached_library" || -e "${cached_library}.pio-link" ]]; then
    pio pkg uninstall -d "$root/test/compile-project" -e "$platform" \
        -l WiFiManager --no-save --skip-dependencies >/dev/null
fi
pio run -d "$root/test/compile-project" -e "$platform"

echo "WiFiManager consumer compile check passed for $platform"
