#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 compile --platform esp8266|esp32" >&2
    exit 2
}

[[ "${1:-}" == "compile" && "${2:-}" == "--platform" && $# -eq 3 ]] || usage
platform="${3:-}"
[[ "$platform" == "esp8266" || "$platform" == "esp32" ]] || usage

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
pio run -d "$root/test/compile-project" -e "$platform"

echo "WiFiManager consumer compile check passed for $platform"
