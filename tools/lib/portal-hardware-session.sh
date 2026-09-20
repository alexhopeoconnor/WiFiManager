#!/usr/bin/env bash
# shellcheck source=tools/lib/harness-locks.sh
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/harness-locks.sh"
# Shared host-side helpers for the WiFiManager portal hardware test harness.
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

wm_nmcli_permission() {
    local permission="$1"
    awk -F: -v permission="$permission" '$1 == permission { print $2; exit }' \
        <<<"${WM_NMCLI_PERMISSIONS:-}"
}

wm_prepare_networkmanager_authorization() {
    # A GUI Polkit agent can authorize direct nmcli actions. An SSH/headless
    # shell has no such agent on many Linux hosts, even for a sudo-capable
    # developer. Resolve that host-side boundary before an erase/upload, then
    # use one fixed command path rather than suppressing an unauthorized scan
    # and later misreporting an SSID timeout.
    local permission value direct=yes
    case "${WM_NMCLI_AUTH:-auto}" in
        auto|direct|sudo) ;;
        *)
            echo 'WM_NMCLI_AUTH must be auto, direct, or sudo.' >&2
            return 2
            ;;
    esac
    [[ "${WM_NMCLI_AUTH_READY:-no}" == yes ]] && return 0

    WM_NMCLI_PERMISSIONS="$(nmcli -t -f PERMISSION,VALUE general permissions 2>/dev/null || true)"
    for permission in \
        org.freedesktop.NetworkManager.wifi.scan \
        org.freedesktop.NetworkManager.network-control \
        org.freedesktop.NetworkManager.settings.modify.system; do
        value="$(wm_nmcli_permission "$permission")"
        [[ "$value" == yes ]] || direct=no
    done

    case "${WM_NMCLI_AUTH:-auto}" in
        direct)
            WM_NMCLI_MODE=direct
            ;;
        sudo)
            WM_NMCLI_MODE=sudo
            ;;
        auto)
            if [[ "$direct" == yes ]]; then
                WM_NMCLI_MODE=direct
            # A session D-Bus socket is common over SSH but does not itself
            # provide a graphical Polkit agent, so require an actual display.
            elif [[ -z "${DISPLAY:-}" && -z "${WAYLAND_DISPLAY:-}" ]]; then
                WM_NMCLI_MODE=sudo
            else
                # Give a graphical Polkit agent the chance to authorize the
                # action. A failure is reported verbatim by wm_nmcli.
                WM_NMCLI_MODE=direct
            fi
            ;;
    esac

    if [[ "$WM_NMCLI_MODE" == sudo ]]; then
        command -v sudo >/dev/null 2>&1 || {
            echo 'NetworkManager requires authorization, but sudo is unavailable. Use a graphical Polkit session or install/configure sudo.' >&2
            return 1
        }
        echo 'NetworkManager requires scoped authorization for the named portal adapter; validating sudo before the board is flashed.' >&2
        sudo -v || {
            echo 'Could not validate sudo for the scoped NetworkManager portal actions.' >&2
            return 1
        }
    fi
    WM_NMCLI_AUTH_READY=yes
    export WM_NMCLI_MODE WM_NMCLI_AUTH_READY WM_NMCLI_PERMISSIONS
}

wm_report_networkmanager_authorization() {
    local permission value direct=yes
    WM_NMCLI_PERMISSIONS="$(nmcli -t -f PERMISSION,VALUE general permissions 2>/dev/null || true)"
    for permission in \
        org.freedesktop.NetworkManager.wifi.scan \
        org.freedesktop.NetworkManager.network-control \
        org.freedesktop.NetworkManager.settings.modify.system; do
        value="$(wm_nmcli_permission "$permission")"
        [[ "$value" == yes ]] || direct=no
    done
    if [[ "$direct" == yes ]]; then
        echo 'NetworkManager portal authorization: direct.'
    elif [[ -z "${DISPLAY:-}" && -z "${WAYLAND_DISPLAY:-}" ]]; then
        echo 'NetworkManager portal authorization: scoped sudo will be requested before a portal command flashes the board.'
    else
        echo 'NetworkManager portal authorization: graphical Polkit may authorize actions; set WM_NMCLI_AUTH=sudo to use scoped sudo instead.'
    fi
}

wm_nmcli() {
    # Only this small set of nmcli calls is elevated when the preflight selects
    # sudo. The runner, artifacts, and session record remain owned by the
    # invoking developer; every mutating call still names the guarded adapter
    # or a generated temporary connection.
    if [[ "${WM_NMCLI_MODE:-direct}" == sudo ]]; then
        sudo -n true || {
            echo 'The sudo authorization for scoped NetworkManager actions expired; run sudo -v and retry the portal command.' >&2
            return 1
        }
        sudo -n -- nmcli "$@"
    else
        nmcli "$@"
    fi
}

wm_default_route_interface() {
    ip route show default 2>/dev/null | awk '/^default/{print $5; exit}'
}

wm_acquire_hardware_lock() {
    # All ordinary ESP portals use this gateway/subnet. Keep portal commands
    # mutually exclusive even when they name different boards or adapters.
    wm_harness_lock_portal_network
}

wm_require_client_adapter() {
    local interface="$1" allow_takeover="$2" default_interface device_type active_connection
    ip link show "$interface" >/dev/null 2>&1 || {
        echo "Wi-Fi interface not found: $interface" >&2
        return 1
    }
    default_interface="$(wm_default_route_interface)"
    [[ "$interface" != "$default_interface" ]] || {
        echo "Refusing to use the host default-route interface: $interface" >&2
        return 1
    }
    # A non-default Ethernet, tunnel, or virtual interface can otherwise look
    # harmless here and reach board flashing before the first Wi-Fi scan
    # fails. Ask NetworkManager through the already-selected authorization
    # path, so a denied inspection cannot be mistaken for a usable adapter.
    if ! device_type="$(wm_nmcli -g GENERAL.TYPE device show "$interface" 2>/dev/null)"; then
        echo "NetworkManager could not determine the selected portal adapter type: $interface" >&2
        return 1
    fi
    [[ "$device_type" == "wifi" ]] || {
        echo "Client adapter is not Wi-Fi: $interface ($device_type)." >&2
        return 1
    }
    wm_harness_lock_wifi_adapter "$interface"
    if ! active_connection="$(wm_nmcli -g GENERAL.CONNECTION device show "$interface" 2>/dev/null)"; then
        echo "NetworkManager could not inspect the selected portal adapter: $interface" >&2
        return 1
    fi
    if [[ -n "$active_connection" && "$active_connection" != "--" && "$allow_takeover" != "yes" ]]; then
        echo "Client adapter $interface already has connection '$active_connection'." >&2
        echo "Pass --take-over-client-adapter to replace only that adapter's connection." >&2
        return 1
    fi
}

wm_portal_ssid() {
    case "$1" in
        esp8266) printf '%s\n' 'WM Test Harness ESP8266' ;;
        esp32) printf '%s\n' 'WM Test Harness ESP32' ;;
        *) return 1 ;;
    esac
}

wm_wait_for_portal_ssid() {
    local interface="$1" ssid="$2" attempt advertised
    if ! wm_nmcli device wifi rescan ifname "$interface"; then
        echo "NetworkManager could not scan the selected portal adapter: $interface" >&2
        return 1
    fi
    for attempt in $(seq 1 45); do
        if ! advertised="$(wm_nmcli -t -f SSID device wifi list ifname "$interface")"; then
            echo "NetworkManager could not read Wi-Fi scan results from $interface." >&2
            return 1
        fi
        if grep -Fxq "$ssid" <<<"$advertised"; then
            return 0
        fi
        sleep 1
        if ! wm_nmcli device wifi rescan ifname "$interface"; then
            echo "NetworkManager could not refresh Wi-Fi scan results from $interface." >&2
            return 1
        fi
    done
    echo "Portal SSID not detected on $interface: $ssid" >&2
    return 1
}

wm_remove_connection_by_name() {
    local name="$1"
    [[ -n "$name" ]] || return 0
    wm_nmcli connection down "$name" >/dev/null 2>&1 || true
    if ! wm_nmcli connection delete "$name"; then
        # A pending recovery record can survive an uncatchable exit before
        # `connection add` ran. Only a successful complete listing which does
        # not contain this generated name proves that there is nothing left to
        # remove; an authorization or NetworkManager query failure must retain
        # the record for an explicit later `down` command.
        if wm_connection_name_is_absent "$name"; then
            return 0
        fi
        wm_report_portal_connection_cleanup_failure
        return 1
    fi
}

wm_connection_name_is_absent() {
    local name="$1" names
    if ! names="$(wm_nmcli -t -f NAME connection show 2>/dev/null)"; then
        return 1
    fi
    ! grep -Fxq -- "$name" <<<"$names"
}

wm_connection_uuid_is_absent() {
    local uuid="$1" uuids
    if ! uuids="$(wm_nmcli -t -f UUID connection show 2>/dev/null)"; then
        return 1
    fi
    ! grep -Fxq -- "$uuid" <<<"$uuids"
}

wm_report_portal_connection_cleanup_failure() {
    echo 'Could not remove the temporary WiFiManager portal connection. Its recovery state was retained; restore NetworkManager authorization and run ./tools/portal-hardware down.' >&2
}

wm_create_portal_connection() {
    local interface="$1" ssid="$2" password="$3" platform="${4:-}" reconnect_after_drop="${5:-no}" name uuid
    case "$reconnect_after_drop" in
        yes|no) ;;
        *)
            echo "Portal connection reconnect policy must be yes or no." >&2
            return 2
            ;;
    esac
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
    wm_nmcli device disconnect "$interface" >/dev/null 2>&1 || true
    if ! wm_wait_for_portal_ssid "$interface" "$ssid"; then
        # No `connection add` has run on this path, so this process knows that
        # its pending record has no NetworkManager profile to remove. Clear it
        # directly rather than requiring a privileged delete of a profile that
        # cannot exist.
        if ! wm_clear_state; then
            wm_report_portal_connection_cleanup_failure
            return 1
        fi
        WM_PORTAL_CONNECTION_OWNED=no
        unset WM_PORTAL_CONNECTION_UUID WM_PORTAL_CONNECTION_NAME
        return 1
    fi
    if ! wm_nmcli connection add type wifi ifname "$interface" con-name "$name" ssid "$ssid" \
        ipv4.method auto ipv4.never-default yes ipv6.method ignore connection.autoconnect no >/dev/null; then
        wm_cleanup_created_connection
        return 1
    fi
    # NetworkManager creates the UUID at `connection add`, before any later
    # configuration or association step.  Capture it immediately so cleanup
    # has an exact identifier throughout the remaining critical section.
    uuid="$(wm_nmcli -g connection.uuid connection show "$name")"
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
    if ! wm_nmcli connection modify "$name" wifi-sec.key-mgmt wpa-psk wifi-sec.psk "$password"; then
        wm_cleanup_created_connection
        return 1
    fi
    # The ordinary portal runner must leave its disposable connection inert so
    # it cannot surprise a developer later. The OTA runner is different: the
    # board intentionally disappears after POST /u, then returns as the same
    # AP, so its one owned connection needs to reassociate autonomously for the
    # browser and host-side B checks. Cleanup still deletes this exact profile.
    if [[ "$reconnect_after_drop" == yes ]] && ! wm_nmcli connection modify "$name" connection.autoconnect yes; then
        wm_cleanup_created_connection
        return 1
    fi
    if ! wm_nmcli connection up "$name" ifname "$interface"; then
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
    [[ -n "$uuid" || -n "$name" ]] || return 0
    if [[ -n "$uuid" ]]; then
        wm_nmcli connection down uuid "$uuid" >/dev/null 2>&1 || true
        if ! wm_nmcli connection delete uuid "$uuid"; then
            # An active UUID means this runner did create a profile. Retain
            # the exact recovery state unless the same authorized
            # NetworkManager view positively proves another actor already
            # removed it. A failed listing (including an expired scoped sudo
            # ticket) is never treated as absence.
            if ! wm_connection_uuid_is_absent "$uuid"; then
                wm_report_portal_connection_cleanup_failure
                return 1
            fi
        fi
    elif [[ -n "$name" ]]; then
        wm_remove_connection_by_name "$name" || return 1
    fi
}

wm_cleanup_created_connection() {
    # Only remove the connection this process named.  This is intentionally
    # separate from `finish_portal_session`, which reads a retained state file
    # for an explicit later `down` command.
    [[ "${WM_PORTAL_CONNECTION_OWNED:-no}" == "yes" ]] || return 0
    wm_remove_connection "${WM_PORTAL_CONNECTION_UUID:-}" "${WM_PORTAL_CONNECTION_NAME:-}" || return 1
    if ! wm_clear_state; then
        echo 'The temporary WiFiManager portal connection was removed, but its recovery state could not be cleared.' >&2
        return 1
    fi
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
    rm -f -- "$file"
}
