#!/usr/bin/env bash
set -euo pipefail

playwright_args=(test --config /work/playwright.config.js)
if [[ -n "${PORTAL_TEST_FILE:-}" ]]; then
    # OTA is a destructive, time-bounded board test harness. Run only its browser
    # spec instead of allowing ordinary portal tests to consume its AP window.
    playwright_args+=("$PORTAL_TEST_FILE")
fi
/work/node_modules/.bin/playwright "${playwright_args[@]}"

if [[ "${PORTAL_CAPTURE_README_MEDIA:-0}" == "1" ]]; then
    /work/render-readme-media.sh
fi
