#!/usr/bin/env bash
# Shared host-side helpers for the WiFiManager portal hardware contract.
# They never modify a network interface other than the explicit client adapter.

wm_portal_state_root() {
    printf '%s/wifimanager-portal-hardware' "${XDG_STATE_HOME:-$HOME/.local/state}"
}

wm_require() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "Required command not found: $1" >&2
        return 1
    }
}

wm_default_route_interface() {
    ip route show default 2>/dev/null | awk '/^default/{print $5; exit}'
}

wm_acquire_hardware_lock() {
    local lock_file="${WM_HARDWARE_LOCK_FILE:-/tmp/wifimanager-hardware.lock}"
    # `ota` deliberately acquires this before it builds firmware, then calls
    # the shared portal-start helper which also acquires it. Keep that nested
    # path idempotent so the lock covers the whole A/B contract rather than
    # only the serial flash and adapter connection.
    [[ "${WM_HARDWARE_LOCK_HELD:-no}" == "yes" ]] && return 0
    exec 9>"$lock_file"
    flock -n 9 || {
        echo "Another WiFiManager hardware task is already running; wait for it to finish." >&2
        return 1
    }
    WM_HARDWARE_LOCK_HELD=yes
}

wm_require_client_adapter() {
    local interface="$1" allow_takeover="$2" default_interface active_connection
    ip link show "$interface" >/dev/null 2>&1 || {
        echo "Wi-Fi interface not found: $interface" >&2
        return 1
    }
    default_interface="$(wm_default_route_interface)"
    [[ "$interface" != "$default_interface" ]] || {
        echo "Refusing to use the host default-route interface: $interface" >&2
        return 1
    }
    active_connection="$(nmcli -g GENERAL.CONNECTION device show "$interface" 2>/dev/null || true)"
    if [[ -n "$active_connection" && "$active_connection" != "--" && "$allow_takeover" != "yes" ]]; then
        echo "Client adapter $interface already has connection '$active_connection'." >&2
        echo "Pass --take-over-client-adapter to replace only that adapter's connection." >&2
        return 1
    fi
}

wm_portal_ssid() {
    case "$1" in
        esp8266) printf '%s\n' 'WM Contract ESP8266' ;;
        esp32) printf '%s\n' 'WM Contract ESP32' ;;
        *) return 1 ;;
    esac
}

wm_wait_for_portal_ssid() {
    local interface="$1" ssid="$2" attempt
    nmcli device wifi rescan ifname "$interface" >/dev/null 2>&1 || true
    for attempt in $(seq 1 45); do
        if nmcli -t -f SSID device wifi list ifname "$interface" | grep -Fxq "$ssid"; then
            return 0
        fi
        sleep 1
        nmcli device wifi rescan ifname "$interface" >/dev/null 2>&1 || true
    done
    echo "Portal SSID not detected on $interface: $ssid" >&2
    return 1
}

wm_remove_connection_by_name() {
    local name="$1"
    [[ -n "$name" ]] || return 0
    nmcli connection down "$name" >/dev/null 2>&1 || true
    nmcli connection delete "$name" >/dev/null 2>&1 || true
}

wm_create_portal_connection() {
    local interface="$1" ssid="$2" password="$3" name uuid
    name="wifimanager-portal-${RANDOM}-$(date +%s)"
    nmcli device disconnect "$interface" >/dev/null 2>&1 || true
    wm_wait_for_portal_ssid "$interface" "$ssid"
    if ! nmcli connection add type wifi ifname "$interface" con-name "$name" ssid "$ssid" \
        ipv4.method auto ipv4.never-default yes ipv6.method ignore connection.autoconnect no >/dev/null; then
        return 1
    fi
    if ! nmcli connection modify "$name" wifi-sec.key-mgmt wpa-psk wifi-sec.psk "$password"; then
        wm_remove_connection_by_name "$name"
        return 1
    fi
    if ! nmcli connection up "$name" ifname "$interface"; then
        wm_remove_connection_by_name "$name"
        return 1
    fi
    uuid="$(nmcli -g connection.uuid connection show "$name")"
    if [[ -z "$uuid" || "$uuid" == "--" ]]; then
        wm_remove_connection_by_name "$name"
        echo "NetworkManager did not return a UUID for the portal connection." >&2
        return 1
    fi
    WM_PORTAL_CONNECTION_UUID="$uuid"
    WM_PORTAL_CONNECTION_NAME="$name"
    export WM_PORTAL_CONNECTION_UUID WM_PORTAL_CONNECTION_NAME
}

wm_verify_portal_route() {
    local interface="$1" route
    route="$(ip route get 192.168.4.1 2>/dev/null || true)"
    [[ "$route" == *" dev $interface "* ]] || {
        echo "Portal route does not use the selected adapter: $route" >&2
        return 1
    }
}

wm_remove_connection() {
    local uuid="$1"
    [[ -n "$uuid" ]] || return 0
    nmcli connection down uuid "$uuid" >/dev/null 2>&1 || true
    nmcli connection delete uuid "$uuid" >/dev/null 2>&1 || true
}

wm_state_file() {
    printf '%s/session.env\n' "$(wm_portal_state_root)"
}

wm_require_no_active_session() {
    local file
    file="$(wm_state_file)"
    [[ ! -e "$file" ]] || {
        echo "An existing WiFiManager portal session is recorded; run ./tools/portal-hardware down first." >&2
        return 1
    }
}

wm_write_state() {
    local interface="$1" platform="$2" uuid="$3" name="$4" root file
    root="$(wm_portal_state_root)"
    file="$(wm_state_file)"
    install -d -m 700 "$root"
    (
        umask 077
        printf 'WM_PORTAL_INTERFACE=%s\nWM_PORTAL_PLATFORM=%s\nWM_PORTAL_CONNECTION_UUID=%s\nWM_PORTAL_CONNECTION_NAME=%s\n' \
            "$interface" "$platform" "$uuid" "$name" >"$file"
    )
    chmod 600 "$file"
}

wm_load_state() {
    local file key value
    file="$(wm_state_file)"
    [[ -f "$file" ]] || {
        echo "No active WiFiManager portal session was found." >&2
        return 1
    }
    WM_PORTAL_INTERFACE=""
    WM_PORTAL_PLATFORM=""
    WM_PORTAL_CONNECTION_UUID=""
    WM_PORTAL_CONNECTION_NAME=""
    while IFS='=' read -r key value; do
        case "$key" in
            WM_PORTAL_INTERFACE|WM_PORTAL_PLATFORM|WM_PORTAL_CONNECTION_UUID|WM_PORTAL_CONNECTION_NAME)
                printf -v "$key" '%s' "$value"
                ;;
            '') ;;
            *)
                echo "Invalid WiFiManager portal session state." >&2
                return 1
                ;;
        esac
    done <"$file"
    [[ -n "$WM_PORTAL_INTERFACE" && -n "$WM_PORTAL_PLATFORM" && -n "$WM_PORTAL_CONNECTION_UUID" && -n "$WM_PORTAL_CONNECTION_NAME" ]] || {
        echo "Incomplete WiFiManager portal session state." >&2
        return 1
    }
    export WM_PORTAL_INTERFACE WM_PORTAL_PLATFORM WM_PORTAL_CONNECTION_UUID WM_PORTAL_CONNECTION_NAME
}

wm_clear_state() {
    local file
    file="$(wm_state_file)"
    rm -f "$file"
}
