#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
failed=0

link_pattern='\]\(([^ )]+)'
while IFS= read -r file; do
    in_fence=false
    while IFS= read -r line || [[ -n "$line" ]]; do
        if [[ "$line" =~ ^[[:space:]]*\`\`\` ]]; then
            [[ "$in_fence" == true ]] && in_fence=false || in_fence=true
            continue
        fi
        [[ "$in_fence" == true ]] && continue
        remainder="$line"
        while [[ "$remainder" =~ $link_pattern ]]; do
            target="${BASH_REMATCH[1]}"
            remainder="${remainder#*]($target)}"
            case "$target" in
                \#*|http://*|https://*|mailto:*|tel:*) continue ;;
            esac
            target="${target%%#*}"
            [[ -z "$target" ]] && continue
            if [[ "$target" == /* ]]; then
                candidate="$root/${target#/}"
            else
                candidate="$(dirname "$file")/$target"
            fi
            if [[ ! -e "$candidate" ]]; then
                printf 'Broken local Markdown link: %s -> %s\n' "${file#$root/}" "$target" >&2
                failed=1
            fi
        done
    done < "$file"
done < <(find "$root" -path "$root/.git" -prune -o -path '*/.pio' -prune -o -type f -name '*.md' -print)

for required in README.md CHANGELOG.md docs/README.md docs/GETTING_STARTED.md docs/PORTAL_UI.md docs/PORTAL_API.md docs/TESTING.md docs/DEVELOPMENT.md; do
    if [[ ! -f "$root/$required" ]]; then
        printf 'Missing required documentation file: %s\n' "$required" >&2
        failed=1
    fi
done

while IFS= read -r example; do
    for required in README.md platformio.ini; do
        if [[ ! -f "$example/$required" ]]; then
            printf 'Incomplete example: %s is missing %s\n' "${example#$root/}" "$required" >&2
            failed=1
        fi
    done
    if ! find "$example" -maxdepth 2 -type f \( -name '*.ino' -o -name '*.cpp' \) -print -quit | grep -q .; then
        printf 'Incomplete example: %s has no sketch source\n' "${example#$root/}" >&2
        failed=1
    fi
done < <(find "$root/examples" -mindepth 1 -maxdepth 1 -type d -print | sort)

if [[ "$failed" -ne 0 ]]; then
    exit 1
fi

echo "WiFiManager documentation checks passed"
