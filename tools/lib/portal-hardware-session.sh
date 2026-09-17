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
    # First-party runners deliberately share this lock: a WiFiManager portal
    # test and a DeviceFramework hardware test can otherwise serial-flash the
    # same selected board concurrently.  Keep the WiFiManager override for
    # isolated tests and let callers redirect the common lock with TMPDIR.
    local lock_file="${WM_HARDWARE_LOCK_FILE:-${TMPDIR:-/tmp}/deviceframework-hardware-test.lock}"
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
    local interface="$1" ssid="$2" password="$3" platform="${4:-}" name uuid
    name="wifimanager-portal-${RANDOM}-$(date +%s)"
    # Publish the owned name before the first NetworkManager mutation.  The
    # caller's signal trap can then remove it throughout the pending-to-active
    # state transition.
    WM_PORTAL_CONNECTION_NAME="$name"
    WM_PORTAL_CONNECTION_UUID=""
    WM_PORTAL_CONNECTION_OWNED=yes
    export WM_PORTAL_CONNECTION_UUID WM_PORTAL_CONNECTION_NAME WM_PORTAL_CONNECTION_OWNED
    # A pending record is atomically committed before the first NetworkManager
    # mutation.  If the host dies after that point, `down` can remove this
    # exact name even before NetworkManager's UUID has been obtained.
    if [[ -n "$platform" ]] && ! wm_write_state "$interface" "$platform" "" "$name"; then
        WM_PORTAL_CONNECTION_OWNED=no
        unset WM_PORTAL_CONNECTION_UUID WM_PORTAL_CONNECTION_NAME
        return 1
    fi
    nmcli device disconnect "$interface" >/dev/null 2>&1 || true
    if ! wm_wait_for_portal_ssid "$interface" "$ssid"; then
        wm_cleanup_created_connection
        return 1
    fi
    if ! nmcli connection add type wifi ifname "$interface" con-name "$name" ssid "$ssid" \
        ipv4.method auto ipv4.never-default yes ipv6.method ignore connection.autoconnect no >/dev/null; then
        wm_cleanup_created_connection
        return 1
    fi
    # NetworkManager creates the UUID at `connection add`, before any later
    # configuration or association step.  Capture it immediately so cleanup
    # has an exact identifier throughout the remaining critical section.
    uuid="$(nmcli -g connection.uuid connection show "$name")"
    if [[ -z "$uuid" || "$uuid" == "--" ]]; then
        wm_cleanup_created_connection
        echo "NetworkManager did not return a UUID for the portal connection." >&2
        return 1
    fi
    WM_PORTAL_CONNECTION_UUID="$uuid"
    export WM_PORTAL_CONNECTION_UUID
    # Atomically replace the pending record with the exact UUID before
    # association.  A normal signal trap removes it immediately; the durable
    # record also makes `down` useful after an uncatchable host termination.
    if [[ -n "$platform" ]] && ! wm_write_state "$interface" "$platform" "$uuid" "$name"; then
        wm_cleanup_created_connection
        return 1
    fi
    if ! nmcli connection modify "$name" wifi-sec.key-mgmt wpa-psk wifi-sec.psk "$password"; then
        wm_cleanup_created_connection
        return 1
    fi
    if ! nmcli connection up "$name" ifname "$interface"; then
        wm_cleanup_created_connection
        return 1
    fi
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
    local uuid="${1:-}" name="${2:-}"
    if [[ -n "$uuid" ]]; then
        nmcli connection down uuid "$uuid" >/dev/null 2>&1 || true
        nmcli connection delete uuid "$uuid" >/dev/null 2>&1 || true
    elif [[ -n "$name" ]]; then
        wm_remove_connection_by_name "$name"
    fi
}

wm_cleanup_created_connection() {
    # Only remove the connection this process named.  This is intentionally
    # separate from `finish_portal_session`, which reads a retained state file
    # for an explicit later `down` command.
    [[ "${WM_PORTAL_CONNECTION_OWNED:-no}" == "yes" ]] || return 0
    wm_remove_connection "${WM_PORTAL_CONNECTION_UUID:-}" "${WM_PORTAL_CONNECTION_NAME:-}"
    wm_clear_state
    WM_PORTAL_CONNECTION_OWNED=no
    unset WM_PORTAL_CONNECTION_UUID WM_PORTAL_CONNECTION_NAME
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
    local interface="$1" platform="$2" uuid="$3" name="$4" root file temporary_file state
    root="$(wm_portal_state_root)"
    file="$(wm_state_file)"
    install -d -m 700 "$root"
    if [[ -n "$uuid" ]]; then
        state="active"
    else
        state="pending"
    fi
    temporary_file="$(mktemp "$root/.session.env.XXXXXX")" || return 1
    if ! {
        printf 'WM_PORTAL_STATE=%s\nWM_PORTAL_INTERFACE=%s\nWM_PORTAL_PLATFORM=%s\nWM_PORTAL_CONNECTION_UUID=%s\nWM_PORTAL_CONNECTION_NAME=%s\n' \
            "$state" "$interface" "$platform" "$uuid" "$name"
    } >"$temporary_file"; then
        rm -f "$temporary_file"
        return 1
    fi
    chmod 600 "$temporary_file"
    mv -f "$temporary_file" "$file"
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
    WM_PORTAL_STATE=""
    while IFS='=' read -r key value; do
        case "$key" in
            WM_PORTAL_STATE|WM_PORTAL_INTERFACE|WM_PORTAL_PLATFORM|WM_PORTAL_CONNECTION_UUID|WM_PORTAL_CONNECTION_NAME)
                printf -v "$key" '%s' "$value"
                ;;
            '') ;;
            *)
                echo "Invalid WiFiManager portal session state." >&2
                return 1
                ;;
        esac
    done <"$file"
    [[ -n "$WM_PORTAL_INTERFACE" && -n "$WM_PORTAL_PLATFORM" && -n "$WM_PORTAL_CONNECTION_NAME" ]] || {
        echo "Incomplete WiFiManager portal session state." >&2
        return 1
    }
    # State files written before this recovery format carried an exact UUID
    # but no phase.  Continue to accept those retained sessions as active.
    if [[ -z "$WM_PORTAL_STATE" && -n "$WM_PORTAL_CONNECTION_UUID" ]]; then
        WM_PORTAL_STATE="active"
    fi
    case "$WM_PORTAL_STATE" in
        pending)
            [[ -z "$WM_PORTAL_CONNECTION_UUID" ]] || {
                echo "Invalid WiFiManager pending portal session state." >&2
                return 1
            }
            ;;
        active)
            [[ -n "$WM_PORTAL_CONNECTION_UUID" ]] || {
                echo "Incomplete WiFiManager active portal session state." >&2
                return 1
            }
            ;;
        *)
            echo "Invalid WiFiManager portal session state." >&2
            return 1
            ;;
    esac
    export WM_PORTAL_STATE WM_PORTAL_INTERFACE WM_PORTAL_PLATFORM WM_PORTAL_CONNECTION_UUID WM_PORTAL_CONNECTION_NAME
}

wm_clear_state() {
    local file
    file="$(wm_state_file)"
    rm -f "$file"
}
