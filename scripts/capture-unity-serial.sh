#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 --port /dev/serial/by-id/... [--timeout seconds]" >&2
    exit 2
}

port=""
timeout_seconds=300
while [[ $# -gt 0 ]]; do
    case "$1" in
        --port) [[ $# -ge 2 ]] || usage; port="${2:-}"; shift 2 ;;
        --timeout) [[ $# -ge 2 ]] || usage; timeout_seconds="${2:-}"; shift 2 ;;
        *) usage ;;
    esac
done

[[ -n "$port" && -e "$port" ]] || usage
[[ "$timeout_seconds" =~ ^[1-9][0-9]*$ ]] || usage

capture_file="$(mktemp -p /tmp wifimanager-unity.XXXXXX)"
monitor_pid=""
preserve_capture=false
cleanup() {
    if [[ -n "$monitor_pid" ]] && kill -0 "$monitor_pid" 2>/dev/null; then
        kill "$monitor_pid" 2>/dev/null || true
        wait "$monitor_pid" 2>/dev/null || true
    fi
    if [[ "$preserve_capture" == "false" ]]; then
        rm -f "$capture_file"
    fi
}
trap cleanup EXIT

# Start immediately after upload. PlatformIO's interactive monitor cannot run
# without a TTY; socat opens only this port and streams its configured 115200
# baud output into the capture file without touching another board.
timeout --foreground "$timeout_seconds" socat -u "FILE:$port,raw,echo=0,b115200" STDOUT \
    >"$capture_file" 2>&1 &
monitor_pid="$!"

while kill -0 "$monitor_pid" 2>/dev/null; do
    if grep -aqE '[0-9]+ Tests [0-9]+ Failures' "$capture_file"; then
        kill "$monitor_pid" 2>/dev/null || true
        wait "$monitor_pid" 2>/dev/null || true
        monitor_pid=""

        if grep -aq "Tests 0 Failures" "$capture_file" && grep -aq "^OK" "$capture_file"; then
            grep -aE '\[METRIC\]|Tests [0-9]+ Failures|^OK$' "$capture_file" || true
            exit 0
        fi

        echo "Unity reported a test failure:" >&2
        grep -anE ':FAIL|FAIL$|\[METRIC\]|Tests [0-9]+ Failures' "$capture_file" >&2 || true
        tail -n 80 "$capture_file" >&2 || true
        preserve_capture=true
        echo "Full serial capture retained at $capture_file" >&2
        exit 1
    fi
    sleep 0.25
done

wait "$monitor_pid" || true
monitor_pid=""
preserve_capture=true
echo "Serial monitoring ended before Unity produced a summary; capture retained at $capture_file" >&2
exit 1
