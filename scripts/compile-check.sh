#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 --platform esp8266|esp32" >&2
    exit 2
}

[[ "${1:-}" == "--platform" ]] || usage
platform="${2:-}"
[[ "$platform" == "esp8266" || "$platform" == "esp32" ]] || usage
[[ $# -eq 2 ]] || usage

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
pio run -d "$root/test/compile-project" -e "$platform"

echo "WiFiManager consumer compile check passed for $platform"
