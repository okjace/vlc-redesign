#!/usr/bin/env bash
# Print the URL of VideoLAN's prebuilt contrib tarball for this tree.
#
# VideoLAN publishes contribs keyed by the last *upstream* commit that touched
# the contrib inputs (same rules as extras/ci/get-contrib-sha.sh). This fork
# edits files under extras/package/* (installer branding), which would change
# that commit, so we resolve it from the upstream commit we are based on
# (redesign/UPSTREAM_COMMIT) instead of HEAD.
#
# Usage: prebuilt-contrib-url.sh <ci-group> <triplet>
#   e.g. prebuilt-contrib-url.sh win64 x86_64-w64-mingw32
#        prebuilt-contrib-url.sh macos-arm64 aarch64-apple-darwin19

set -euo pipefail

GROUP="${1:?ci group}"
TRIPLET="${2:?triplet}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BASE="$(cat "$ROOT/redesign/UPSTREAM_COMMIT")"

paths=(extras/tools contrib extras/ci)
case "$GROUP" in
    win32*|win64*|uwp*) paths+=(extras/package/win32) ;;
    macos*)             paths+=(extras/package/macosx) ;;
esac

SHA="$(awk -v g="$GROUP" '$1 == g { print $2 }' "$ROOT/redesign/ci/prebuilt-shas.txt")"
if [ -z "$SHA" ]; then
    SHA="$(git -C "$ROOT" rev-list -1 "$BASE" -- "${paths[@]}" 2>/dev/null || true)"
fi
URL="https://artifacts.videolan.org/vlc/$GROUP/vlc-contrib-$TRIPLET-$SHA.tar.zst"

if curl -sfIL -o /dev/null "$URL"; then
    echo "$URL"
else
    echo "warning: no prebuilt contribs at $URL, contribs will be built from source" >&2
fi
