#!/usr/bin/env bash
set -euo pipefail

/work/node_modules/.bin/playwright test --config /work/playwright.config.js

if [[ "${PORTAL_CAPTURE_README_MEDIA:-0}" == "1" ]]; then
    /work/render-readme-media.sh
fi
