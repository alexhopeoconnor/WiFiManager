#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 vMAJOR.MINOR.PATCH [--tag]"
    exit 2
}

tag="${1:-}"
[[ "$tag" =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]] || usage
[[ "${2:-}" == "" || "${2:-}" == "--tag" ]] || usage

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
version="${tag#v}"
manifest_version="$(sed -n 's/.*"version": "\([^"]*\)".*/\1/p' "$root/library.json" | head -n 1)"

if [[ "$manifest_version" != "$version" ]]; then
    echo "library.json is $manifest_version; expected $version for $tag" >&2
    exit 1
fi

grep -q "^## $version$" "$root/CHANGELOG.md" || {
    echo "CHANGELOG.md has no $version heading" >&2
    exit 1
}

if [[ -f "$root/library.properties" ]]; then
    properties_version="$(sed -n 's/^version=//p' "$root/library.properties" | head -n 1)"
    if [[ "$properties_version" != "$version" ]]; then
        echo "library.properties is $properties_version; expected $version for $tag" >&2
        exit 1
    fi
fi

if awk -v heading="## $version" '
    $0 == heading { found = 1; next }
    found && /^## / { exit }
    found { print }
' "$root/CHANGELOG.md" | grep -Fq 'TODO: Describe this release.'; then
    echo "CHANGELOG.md still has the generated TODO for $version" >&2
    exit 1
fi
repo_url="https://github.com/alexhopeoconnor/WiFiManager.git"
validate_reference() {
    local file="$1"
    local reference_count
    reference_count="$(grep -F "$repo_url#v" "$root/$file" | wc -l)"
    [[ "$reference_count" -eq 1 ]] || { echo "$file must contain exactly one canonical release reference" >&2; exit 1; }
    grep -Fq "$repo_url#$tag" "$root/$file" || { echo "$file does not reference $tag" >&2; exit 1; }
}
validate_reference README.md
validate_reference docs/GETTING_STARTED.md

git -C "$root" diff --check
package_dir="$(mktemp -d)"
trap 'rm -rf "$package_dir"' EXIT
pio pkg pack "$root" --output "$package_dir/package.tar.gz" >/dev/null
echo "Validated release metadata and PlatformIO package for $tag"

if [[ "${2:-}" == "--tag" ]]; then
    git -C "$root" diff --quiet
    git -C "$root" diff --cached --quiet
    git -C "$root" tag -a "$tag" -m "Release $tag"
    echo "Created $tag. Push the branch and tag; GitHub Actions will publish the release."
fi
