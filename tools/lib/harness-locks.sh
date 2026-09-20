#!/usr/bin/env bash
# Resource locks shared by WiFiManager and DeviceFramework physical test harnesses.
#
# The protocol deliberately uses a stable, non-secret path and hash input so
# standalone checkouts still coordinate when they use the same host resources.

declare -A WM_HARNESS_LOCK_FDS=()

wm_harness_lock_root() {
    local runtime_root
    if [[ -n "${ARDUINO_TEST_HARNESS_LOCK_DIR:-}" ]]; then
        printf '%s\n' "$ARDUINO_TEST_HARNESS_LOCK_DIR"
        return 0
    fi
    runtime_root="${XDG_RUNTIME_DIR:-}"
    if [[ -n "$runtime_root" && -d "$runtime_root" && -w "$runtime_root" ]]; then
        printf '%s/arduino-framework-test-harness-locks\n' "$runtime_root"
    else
        printf '%s/arduino-framework-test-harness-locks\n' "${TMPDIR:-/tmp}"
    fi
}

wm_harness_lock_resource() {
    local label="$1" resource="$2" lock_root digest lock_file lock_fd
    [[ -n "$label" && -n "$resource" ]] || {
        echo "A test-harness resource lock needs a label and key." >&2
        return 2
    }
    [[ -n "${WM_HARNESS_LOCK_FDS[$resource]:-}" ]] && return 0
    command -v flock >/dev/null 2>&1 || {
        echo "flock is required to protect test-harness resources." >&2
        return 1
    }
    command -v sha256sum >/dev/null 2>&1 || {
        echo "sha256sum is required to name test-harness resource locks." >&2
        return 1
    }
    lock_root="$(wm_harness_lock_root)"
    install -d -m 700 "$lock_root"
    digest="$(printf '%s' "$resource" | sha256sum | awk '{print $1}')"
    lock_file="$lock_root/${digest}.lock"
    exec {lock_fd}>"$lock_file"
    if ! flock -n "$lock_fd"; then
        printf "Cannot start: %s is already in use by another local test-harness process.\n" "$label" >&2
        return 1
    fi
    WM_HARNESS_LOCK_FDS["$resource"]="$lock_fd"
}

wm_harness_lock_serial_port() {
    local port="$1" canonical
    canonical="$(readlink -f -- "$port" 2>/dev/null || printf '%s' "$port")"
    wm_harness_lock_resource "serial port $canonical" "serial-port:$canonical"
}

wm_harness_lock_portal_network() {
    wm_harness_lock_resource "the 192.168.4.0/24 portal network" "portal-network:192.168.4.0/24"
}

wm_harness_lock_wifi_adapter() {
    local interface="$1"
    wm_harness_lock_resource "Wi-Fi adapter $interface" "wifi-adapter:$interface"
}
