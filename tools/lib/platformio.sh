#!/usr/bin/env bash
# PlatformIO discovery shared by WiFiManager's local test and hardware runners.
# Non-interactive shells, including an SSH hardware lab session, do not always
# include PlatformIO's standard virtual environment in PATH.

wm_pio_executable() {
    local executable
    if [[ -n "${WIFIMANAGER_PIO_EXECUTABLE:-}" ]]; then
        executable="$WIFIMANAGER_PIO_EXECUTABLE"
    else
        executable="$(command -v pio 2>/dev/null || true)"
        if [[ -z "$executable" && -x "$HOME/.platformio/penv/bin/pio" ]]; then
            executable="$HOME/.platformio/penv/bin/pio"
        fi
    fi
    [[ -n "$executable" && -x "$executable" ]] || {
        echo "PlatformIO is required; install it or set WIFIMANAGER_PIO_EXECUTABLE." >&2
        return 1
    }
    printf '%s\n' "$executable"
}

wm_pio_available() {
    wm_pio_executable >/dev/null
}

wm_pio() {
    local executable
    executable="$(wm_pio_executable)" || return
    "$executable" "$@"
}
