#!/usr/bin/env bash
# README-media renderer used by the portal test harness.
set -euo pipefail

artifact_dir="${ARTIFACT_DIR:?ARTIFACT_DIR is required}"
media_dir="$artifact_dir/readme-media"
source_video="$media_dir/raw/portal-tour.webm"
target_gif="$media_dir/portal-tour.gif"

[[ -s "$source_video" ]] || {
    echo "README media video was not recorded: $source_video" >&2
    exit 1
}

ffmpeg -hide_banner -loglevel error -y -i "$source_video" \
    -filter_complex '[0:v]setpts=1.25*PTS,fps=6,scale=720:-2:flags=lanczos,split[a][b];[a]palettegen=max_colors=128[p];[b][p]paletteuse' \
    -loop 0 "$target_gif"

[[ -s "$target_gif" ]] || {
    echo "README media GIF was not rendered: $target_gif" >&2
    exit 1
}

duration="$(ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 "$target_gif")"
awk -v duration="$duration" 'BEGIN { exit !(duration >= 4 && duration <= 30) }' || {
    echo "README media GIF duration is outside the 4–30 second review range: $duration" >&2
    exit 1
}

max_bytes=$((2 * 1024 * 1024))
(( $(wc -c < "$target_gif") <= max_bytes )) || {
    echo "README media GIF exceeds its 2 MiB documentation budget: $target_gif" >&2
    exit 1
}
