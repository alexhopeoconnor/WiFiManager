"""Add the run-owned A/B identity directory to this portal test-harness build."""

import os

from SCons.Script import Exit

Import("env")

if env["PIOENV"].endswith("_ota"):
    identity_dir = os.environ.get("WIFIMANAGER_OTA_IDENTITY_DIR")
    if not identity_dir:
        print(
            "WIFIMANAGER_OTA_IDENTITY_DIR is required; "
            "run this fixture through its named test harness."
        )
        Exit(1)

    identity_header = os.path.join(identity_dir, "ota_fixture_identity.h")
    if not os.path.isfile(identity_header):
        print(f"OTA fixture identity header is missing: {identity_header}")
        Exit(1)

    env.Append(CPPPATH=[identity_dir])
