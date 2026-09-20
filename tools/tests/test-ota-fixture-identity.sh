#!/usr/bin/env bash
# Check OTA fixture identity and environment locking without PlatformIO or a board.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
temporary_root="$(mktemp -d "${TMPDIR:-/tmp}/wifimanager-ota-identity-test.XXXXXX")"
chmod 700 "$temporary_root"
cleanup() {
    rm -rf -- "$temporary_root"
}
trap cleanup EXIT HUP INT TERM

export ARDUINO_TEST_HARNESS_LOCK_DIR="$temporary_root/locks"
export WIFIMANAGER_OTA_IDENTITY_DIR="$temporary_root/identity"
# shellcheck source=tools/lib/ota-fixture-identity.sh
source "$root/tools/lib/ota-fixture-identity.sh"
# shellcheck source=tools/lib/harness-locks.sh
source "$root/tools/lib/harness-locks.sh"

wm_write_ota_fixture_identity A
identity_file="$WIFIMANAGER_OTA_IDENTITY_DIR/ota_fixture_identity.h"
[[ "$(stat -c '%a' "$WIFIMANAGER_OTA_IDENTITY_DIR")" == 700 ]]
[[ "$(stat -c '%a' "$identity_file")" == 600 ]]
rg -Fqx '#define WM_OTA_FIXTURE_IMAGE "A"' "$identity_file"

wm_lock_ota_fixture_environment esp8266_ota
collision_output="$temporary_root/lock-collision.log"
if (
    root="$root"
    export WIFIMANAGER_OTA_IDENTITY_DIR="$temporary_root/other-identity"
    # shellcheck source=tools/lib/ota-fixture-identity.sh
    source "$root/tools/lib/ota-fixture-identity.sh"
    wm_lock_ota_fixture_environment esp8266_ota
) 2>"$collision_output"; then
    echo "A second test-harness process unexpectedly acquired the same OTA environment lock." >&2
    exit 1
fi
rg -Fqx "OTA fixture environment 'esp8266_ota' is already in use by another local test-harness run." "$collision_output"

wm_harness_lock_resource "test resource" "identity-test:shared-resource"
if (
    # shellcheck source=tools/lib/harness-locks.sh
    source "$root/tools/lib/harness-locks.sh"
    wm_harness_lock_resource "test resource" "identity-test:shared-resource"
) 2>"$collision_output"; then
    echo "A second test-harness process unexpectedly acquired the same resource lock." >&2
    exit 1
fi
rg -Fqx 'Cannot start: test resource is already in use by another local test-harness process.' "$collision_output"

wm_remove_ota_fixture_identity
[[ ! -e "$identity_file" ]]
echo "WiFiManager OTA fixture identity test-harness check passed"
