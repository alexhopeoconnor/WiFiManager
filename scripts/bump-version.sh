#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 vMAJOR.MINOR.PATCH" >&2
    exit 2
}

tag="${1:-}"
[[ "$tag" =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]] || usage

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
version="${tag#v}"
repo_url="https://github.com/alexhopeoconnor/WiFiManager.git"
reference_files=(README.md docs/GETTING_STARTED.md)

current_version="$(sed -n 's/.*"version": "\([^"]*\)".*/\1/p' "$root/library.json" | head -n 1)"
[[ "$current_version" != "$version" ]] || {
    echo "library.json already declares $version; choose a new version." >&2
    exit 1
}
grep -q "^## $version$" "$root/CHANGELOG.md" && {
    echo "CHANGELOG.md already has a $version section; choose a new version." >&2
    exit 1
}

sed -i -E '0,/"version": "[0-9]+\.[0-9]+\.[0-9]+"/s//"version": "'"$version"'"/' "$root/library.json"
for file in "${reference_files[@]}"; do
    sed -i -E "s|${repo_url}#v[0-9]+\.[0-9]+\.[0-9]+|${repo_url}#v${version}|g" "$root/$file"
done

temp_file="$(mktemp)"
trap 'rm -f "$temp_file"' EXIT
{
    IFS= read -r changelog_heading < "$root/CHANGELOG.md"
    [[ "$changelog_heading" == "# Changelog" ]] || {
        echo "CHANGELOG.md must begin with # Changelog" >&2
        exit 1
    }
    printf '%s\n\n## %s\n\n- TODO: Describe this release.\n' "$changelog_heading" "$version"
    tail -n +2 "$root/CHANGELOG.md"
} > "$temp_file"
mv "$temp_file" "$root/CHANGELOG.md"

echo "Updated WiFiManager declarations and canonical install references to $tag."
echo "Replace the generated changelog TODO with the release summary, then run scripts/check-docs.sh and scripts/prepare-release.sh $tag."
