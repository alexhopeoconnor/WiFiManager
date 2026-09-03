#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 compile|examples --platform esp8266|esp32" >&2
    exit 2
}

[[ $# -eq 3 && ( "${1:-}" == "compile" || "${1:-}" == "examples" ) && "${2:-}" == "--platform" ]] || usage
platform="${3:-}"
[[ "$platform" == "esp8266" || "$platform" == "esp32" ]] || usage

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ "$1" == "examples" ]]; then
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

cached_library="$root/test/compile-project/.pio/libdeps/${platform}/WiFiManager"
# The fixture intentionally declares only this local package. Remove a prior
# link so each check resolves the current manifest as a fresh consumer would.
if [[ -d "$cached_library" || -e "${cached_library}.pio-link" ]]; then
    pio pkg uninstall -d "$root/test/compile-project" -e "$platform" \
        -l WiFiManager --no-save --skip-dependencies >/dev/null
fi
pio run -d "$root/test/compile-project" -e "$platform"

echo "WiFiManager consumer compile check passed for $platform"
