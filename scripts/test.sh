#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'USAGE' >&2
Usage:
  ./scripts/test.sh compile      --platform esp8266|esp32
  ./scripts/test.sh unity        --platform esp8266|esp32
  ./scripts/test.sh examples     --platform esp8266|esp32
  ./scripts/test.sh ota-fixtures --platform esp8266|esp32
  ./scripts/test.sh hardware     --platform esp8266|esp32 --port /dev/serial/by-id/...
USAGE
    exit 2
}

mode="${1:-}"
[[ "$mode" == "compile" || "$mode" == "unity" || "$mode" == "examples" || "$mode" == "ota-fixtures" || "$mode" == "hardware" ]] || usage
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

case "$platform" in
    esp8266|esp32) environment="$platform" ;;
    *) usage ;;
esac
[[ "$mode" != "hardware" || -n "$port" ]] || usage
[[ "$mode" != "hardware" || -e "$port" ]] || { echo "Serial port not found: $port" >&2; exit 1; }

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

pio_for_platform() {
    if [[ "$platform" != "esp32" ]]; then
        pio "$@"
        return
    fi

    # Keep the maintained Core 3.3.11 package form in a persistent project
    # cache. It is never cleared by this script and avoids stale global
    # package metadata selecting an incompatible uploader.
    local core_dir packages_dir cache_dir
    core_dir="${WIFIMANAGER_PLATFORMIO_CORE_DIR:-${XDG_CACHE_HOME:-$HOME/.cache}/wifimanager-platformio/core-3.3.11}"
    packages_dir="${WIFIMANAGER_PLATFORMIO_PACKAGES_DIR:-$core_dir/packages}"
    cache_dir="${WIFIMANAGER_PLATFORMIO_CACHE_DIR:-$core_dir/cache}"
    install -d -m 700 "$core_dir" "$packages_dir" "$cache_dir"
    PLATFORMIO_CORE_DIR="$core_dir" PLATFORMIO_PACKAGES_DIR="$packages_dir" \
        PLATFORMIO_CACHE_DIR="$cache_dir" pio "$@"
}

assert_ota_fixture_pair() {
    local fixture_platform="$1"
    local firmware_a firmware_b firmware size capacity
    firmware_a="$root/test/portal-harness/.pio/build/${fixture_platform}_ota_a/firmware.bin"
    firmware_b="$root/test/portal-harness/.pio/build/${fixture_platform}_ota_b/firmware.bin"
    [[ -s "$firmware_a" && -s "$firmware_b" ]] || {
        echo "Portal OTA fixture build did not produce both A and B images." >&2
        return 1
    }
    if cmp -s "$firmware_a" "$firmware_b"; then
        echo "Portal OTA fixture A and B are identical." >&2
        return 1
    fi
    if [[ "$platform" == "esp32" ]]; then
        capacity=$((0x1F0000))
        for firmware in "$firmware_a" "$firmware_b"; do
            size="$(wc -c < "$firmware" | tr -d '[:space:]')"
            (( size <= capacity )) || {
                echo "ESP32 OTA fixture $(basename "$(dirname "$firmware")") is $size bytes; it exceeds the $capacity-byte app slot." >&2
                return 1
            }
        done
    fi
}

if [[ "$mode" == "hardware" ]]; then
    # Keep serial flashing and portal-adapter work mutually exclusive.
    # shellcheck source=tools/lib/portal-hardware-session.sh
    source "$root/tools/lib/portal-hardware-session.sh"
    wm_acquire_hardware_lock
fi

if [[ "$mode" == "examples" ]]; then
    mapfile -t examples < <(find "$root/examples" -mindepth 2 -maxdepth 2 -type f -name platformio.ini -printf '%h\n' | sort)
    if (( ${#examples[@]} == 0 )); then
        echo "No example projects found" >&2
        exit 1
    fi
    for example in "${examples[@]}"; do
        pio_for_platform run -d "$example" -e "$environment" </dev/null
    done
    echo "WiFiManager examples compile check passed for $platform"
    exit 0
fi

if [[ "$mode" == "ota-fixtures" ]]; then
    fixture_platform="$platform"
    if [[ "$fixture_platform" == "esp32" ]]; then
        "$root/tools/check-ota-partitions.sh"
    fi
    for image in a b; do
        pio_for_platform run -d "$root/test/portal-harness" -e "${fixture_platform}_ota_${image}" </dev/null
    done
    assert_ota_fixture_pair "$fixture_platform"
    echo "WiFiManager portal OTA fixture compile check passed for $platform"
    exit 0
fi

if [[ "$mode" == "unity" ]]; then
    # Compile WiFiManager's own fixtures without a board. This is separate
    # from the clean-consumer fixture, which protects manifest resolution.
    pio_for_platform test -d "$root" -e "$environment" --filter test_wifimanager \
        --without-uploading --without-testing
    echo "WiFiManager Unity compile check passed for $platform"
    exit 0
fi

if [[ "$mode" == "hardware" ]]; then
    # Upload first, then capture from the normal boot reset. The Unity sketch
    # deliberately waits two seconds before it begins its test sequence.
    pio_for_platform test -d "$root" -e "$platform" --filter test_wifimanager \
        --upload-port "$port" --without-testing
    "$root/scripts/capture-unity-serial.sh" --port "$port" --timeout 300
    echo "WiFiManager hardware test passed for $platform on $port"
    exit 0
fi

pio_for_platform run -d "$root/test/compile-project" -e "$environment"

echo "WiFiManager consumer compile check passed for $platform"
