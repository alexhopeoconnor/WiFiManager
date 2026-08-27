#!/usr/bin/env bash
set -euo pipefail

tag="${1:-}"
[[ "$tag" =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]] || {
    echo "Usage: $0 vMAJOR.MINOR.PATCH" >&2
    exit 2
}

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
version="${tag#v}"
notes="$(mktemp)"
trap 'rm -f "$notes"' EXIT
awk -v heading="## ${version}" '
    $0 == heading { found = 1; next }
    found && /^## / { exit }
    found { print }
    END { if (!found) exit 1 }
' "$root/CHANGELOG.md" > "$notes" || {
    echo "CHANGELOG.md has no ${version} section" >&2
    exit 1
}
if [[ ! -s "$notes" ]]; then
    echo "No release notes found for $tag in CHANGELOG.md" >&2
    exit 1
fi
printf '%s\n\n' "# WiFiManager $tag"
cat "$notes"
