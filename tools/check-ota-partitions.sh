#!/usr/bin/env bash
# Verify the tracked ESP32 OTA table used by the portal A/B fixtures. Keep the
# check independent of PlatformIO so CI rejects a layout regression before it
# downloads a framework or a hardware runner erases a board.
set -euo pipefail

project_dir="$(cd "$(dirname "$0")/.." && pwd)"
table="${1:-$project_dir/test/portal-harness/partitions/esp32_ota_4m_no_fs.csv}"

[[ $# -le 1 && -r "$table" ]] || {
    echo "Usage: $0 [partition-table.csv]" >&2
    exit 2
}

partition_row() {
    local name="$1" type="$2" subtype="$3"
    awk -F, -v name="$name" -v type="$type" -v subtype="$subtype" '
        function trim(value) {
            gsub(/^[[:space:]]+|[[:space:]]+$/, "", value)
            return value
        }
        /^[[:space:]]*#/ || NF < 5 { next }
        trim($1) == name && trim($2) == type && trim($3) == subtype {
            print trim($1) "," trim($2) "," trim($3) "," trim($4) "," trim($5)
        }
    ' "$table"
}

require_row() {
    local name="$1" type="$2" subtype="$3" offset="$4" size="$5" actual expected
    expected="$name,$type,$subtype,$offset,$size"
    actual="$(partition_row "$name" "$type" "$subtype")"
    [[ "$actual" == "$expected" ]] || {
        echo "Expected exactly this OTA partition row: $expected" >&2
        echo "Found: ${actual:-<none>}" >&2
        return 1
    }
}

require_row nvs data nvs 0x9000 0x5000
require_row otadata data ota 0xe000 0x2000
require_row app0 app ota_0 0x10000 0x1F0000
require_row app1 app ota_1 0x200000 0x1F0000

app_count="$(awk -F, '
    function trim(value) { gsub(/^[[:space:]]+|[[:space:]]+$/, "", value); return value }
    /^[[:space:]]*#/ || NF < 5 { next }
    trim($2) == "app" { count++ }
    END { print count + 0 }
' "$table")"
[[ "$app_count" == 2 ]] || {
    echo "OTA layout must contain exactly two application partitions, found $app_count." >&2
    exit 1
}

if awk -F, '
    function trim(value) { gsub(/^[[:space:]]+|[[:space:]]+$/, "", value); return value }
    /^[[:space:]]*#/ || NF < 5 { next }
    trim($2) == "data" && trim($3) ~ /^(spiffs|littlefs|fat)$/ { found = 1 }
    END { exit found ? 0 : 1 }
' "$table"; then
    echo "OTA test layout must not reserve a filesystem partition." >&2
    exit 1
fi

echo "ESP32 OTA partition contract passed: $table"
