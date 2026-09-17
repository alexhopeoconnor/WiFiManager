#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'USAGE' >&2
Usage:
  ./scripts/test.sh compile      --platform esp8266|esp32|esp32-current
  ./scripts/test.sh unity        --platform esp8266|esp32|esp32-current
  ./scripts/test.sh examples     --platform esp8266|esp32
  ./scripts/test.sh ota-fixtures --platform esp8266|esp32|esp32-current
  ./scripts/test.sh packages     --platform esp8266|esp32|esp32-current
  ./scripts/test.sh hardware     --platform esp8266|esp32 --port /dev/serial/by-id/...
USAGE
    exit 2
}

mode="${1:-}"
[[ "$mode" == "compile" || "$mode" == "unity" || "$mode" == "examples" || "$mode" == "ota-fixtures" || "$mode" == "packages" || "$mode" == "hardware" ]] || usage
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
    esp8266|esp32)
        environment="$platform"
        ;;
    esp32-current)
        environment="esp32_core_3_3_11"
        ;;
    *) usage ;;
esac
[[ "$mode" != "examples" || "$platform" != "esp32-current" ]] || {
    echo "The current ESP32 lane is a clean-consumer check; examples retain their explicit compatibility environments." >&2
    exit 2
}
[[ "$mode" != "hardware" || "$platform" != "esp32-current" ]] || {
    echo "The current ESP32 lane is a board-free clean-consumer check; use esp32 for the existing Unity hardware suite." >&2
    exit 2
}
[[ "$mode" != "ota-fixtures" || "$platform" != "esp32" ]] || {
    echo "ESP32 OTA fixtures are pinned to the isolated 3.3.11 graph; use --platform esp32-current." >&2
    exit 2
}
[[ "$mode" != "hardware" || -n "$port" ]] || usage
[[ "$mode" != "hardware" || -e "$port" ]] || { echo "Serial port not found: $port" >&2; exit 1; }

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

pio_for_platform() {
    if [[ "$platform" != "esp32-current" ]]; then
        pio "$@"
        return
    fi

    # pioarduino Core 3.3.11's esptool package form cannot safely share a
    # PlatformIO Core directory with a legacy Core 3.0.5 tool-esptoolpy
    # installation. Keep the current validation lane in a project-owned,
    # user-cache location unless the developer deliberately supplies one.
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
    if [[ "$platform" == "esp32-current" ]]; then
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
        pio_for_platform run -d "$example" -e "$platform" </dev/null
    done
    echo "WiFiManager examples compile check passed for $platform"
    exit 0
fi

if [[ "$mode" == "ota-fixtures" ]]; then
    fixture_platform="$platform"
    # ESP32 OTA fixtures are deliberately pinned to the current 3.3.11 lane.
    # Accept the same public target name as the clean-consumer check so CI
    # never mixes legacy and current platform package graphs in one worker.
    if [[ "$fixture_platform" == "esp32-current" ]]; then
        fixture_platform="esp32"
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

if [[ "$mode" == "packages" ]]; then
    pio_for_platform pkg list -d "$root/test/compile-project" -e "$environment"
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

cached_library="$root/test/compile-project/.pio/libdeps/${environment}/WiFiManager"
# The fixture intentionally declares only this local package. Remove a prior
# link so each check resolves the current manifest as a fresh consumer would.
if [[ -d "$cached_library" || -e "${cached_library}.pio-link" ]]; then
    pio_for_platform pkg uninstall -d "$root/test/compile-project" -e "$environment" \
        -l WiFiManager --no-save --skip-dependencies >/dev/null
fi
pio_for_platform run -d "$root/test/compile-project" -e "$environment"

echo "WiFiManager consumer compile check passed for $platform"
