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

if [[ -f "$root/library.properties" ]]; then
    properties_version="$(sed -n 's/^version=//p' "$root/library.properties" | head -n 1)"
    if [[ "$properties_version" != "$version" ]]; then
        echo "library.properties is $properties_version; expected $version for $tag" >&2
        exit 1
    fi
fi

package_dir="$(mktemp -d)"
trap 'rm -rf "$package_dir"' EXIT
pio pkg pack "$root" --output "$package_dir/package.tar.gz" >/dev/null
echo "Validated PlatformIO package for $tag"

if [[ "${2:-}" == "--tag" ]]; then
    git -C "$root" diff --quiet
    git -C "$root" diff --cached --quiet
    git -C "$root" tag -a "$tag" -m "Release $tag"
    echo "Created $tag. Push the branch and tag; GitHub Actions will publish the release."
fi
