#!/usr/bin/env bash
set -euo pipefail

# Keep standalone compilation predictable on laptops and shared workstations.
# A developer may explicitly raise this for an isolated local diagnosis.
export PLATFORMIO_RUN_JOBS="${PLATFORMIO_RUN_JOBS:-2}"

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
# shellcheck source=tools/lib/platformio.sh
source "$root/tools/lib/platformio.sh"
# shellcheck source=tools/lib/ota-fixture-identity.sh
source "$root/tools/lib/ota-fixture-identity.sh"

pio_for_platform() {
    if [[ "$platform" != "esp32" ]]; then
        wm_pio "$@"
        return
    fi

    # Keep the maintained Core 3.3.11 package form in a persistent project
    # cache shared by the maintained framework repositories. It is never
    # cleared by this script and avoids stale global package metadata selecting
    # an incompatible uploader without redownloading this same pinned graph.
    local core_dir packages_dir cache_dir
    core_dir="${WIFIMANAGER_PLATFORMIO_CORE_DIR:-${XDG_CACHE_HOME:-$HOME/.cache}/arduino-framework-platformio/core-3.3.11}"
    packages_dir="${WIFIMANAGER_PLATFORMIO_PACKAGES_DIR:-$core_dir/packages}"
    cache_dir="${WIFIMANAGER_PLATFORMIO_CACHE_DIR:-$core_dir/cache}"
    install -d -m 700 "$core_dir" "$packages_dir" "$cache_dir"
    PLATFORMIO_CORE_DIR="$core_dir" PLATFORMIO_PACKAGES_DIR="$packages_dir" \
        PLATFORMIO_CACHE_DIR="$cache_dir" wm_pio "$@"
}

assert_ota_fixture_pair() {
    local firmware_a="$1" firmware_b="$2" firmware size capacity
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
                echo "ESP32 OTA fixture $(basename "$firmware") is $size bytes; it exceeds the $capacity-byte app slot." >&2
                return 1
            }
        done
    fi
}

if [[ "$mode" == "hardware" ]]; then
    # Unity uses only the named serial device; it does not own a portal AP or
    # secondary adapter, so it may run beside an unrelated station test.
    # shellcheck source=tools/lib/harness-locks.sh
    source "$root/tools/lib/harness-locks.sh"
    wm_harness_lock_serial_port "$port"
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
    fixture_environment="${platform}_ota"
    if [[ "$platform" == "esp32" ]]; then
        "$root/tools/check-ota-partitions.sh"
    fi

    # The A/B marker is the only changed source input. Capture each resulting
    # binary before rebuilding the same platform environment so CI proves the
    # update images differ without paying for duplicate dependency builds.
    fixture_dir="$(mktemp -d "${TMPDIR:-/tmp}/wifimanager-ota-fixtures.XXXXXX")"
    chmod 700 "$fixture_dir"
    export WIFIMANAGER_OTA_IDENTITY_DIR="$fixture_dir/identity"
    fixture_build_dir="$fixture_dir/build/$fixture_environment"
    wm_lock_ota_fixture_environment "$fixture_environment"
    wm_write_ota_fixture_identity A
    fixture_a="$fixture_dir/ota-a.bin"
    fixture_b="$fixture_dir/ota-b.bin"
    cleanup_ota_fixture_build() {
        wm_remove_ota_fixture_identity
        rm -rf -- "$fixture_dir"
    }
    on_ota_fixture_build_signal() {
        local status="$1"
        # Remove the generated identity before leaving.  EXIT will call the
        # same idempotent cleanup once more, which is intentional.
        trap - HUP INT TERM
        cleanup_ota_fixture_build
        exit "$status"
    }
    trap cleanup_ota_fixture_build EXIT
    trap 'on_ota_fixture_build_signal 129' HUP
    trap 'on_ota_fixture_build_signal 130' INT
    trap 'on_ota_fixture_build_signal 143' TERM

    PLATFORMIO_BUILD_DIR="$fixture_dir/build" \
        pio_for_platform run -d "$root/test/portal-harness" -e "$fixture_environment" </dev/null
    install -m 600 "$fixture_build_dir/firmware.bin" "$fixture_a"
    wm_write_ota_fixture_identity B
    PLATFORMIO_BUILD_DIR="$fixture_dir/build" \
        pio_for_platform run -d "$root/test/portal-harness" -e "$fixture_environment" </dev/null
    install -m 600 "$fixture_build_dir/firmware.bin" "$fixture_b"
    assert_ota_fixture_pair "$fixture_a" "$fixture_b"
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
