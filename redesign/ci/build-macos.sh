#!/usr/bin/env bash
# Build VLC Redesign for macOS and package it as a self-installing
# "VLC Redesign.app" (zipped for download).
#
# Usage: redesign/ci/build-macos.sh <x86_64|arm64>
#
# Uses VideoLAN's prebuilt contribs when VLC_PREBUILT_CONTRIBS_URL is set.

set -euo pipefail

ARCH="${1:?usage: $0 <x86_64|arm64>}"
SRC_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$SRC_DIR/build-macos-$ARCH}"
OUT_DIR="${OUT_DIR:-$SRC_DIR/out}"
JOBS="$(sysctl -n hw.ncpu)"

case "$ARCH" in
    x86_64) VLC_ARCH=x86_64 ;;
    arm64|aarch64) VLC_ARCH=aarch64; ARCH=arm64 ;;
    *) echo "unknown arch $ARCH" >&2; exit 1 ;;
esac

mkdir -p "$BUILD_DIR" "$OUT_DIR"

echo "::group::Build VLC ($ARCH)"
cd "$BUILD_DIR"
# No Sparkle: it would "update" this fork to the official VLC.
# No ncurses: a terminal UI is useless in the .app, and the prebuilt ncurses
# references the SDK path of VideoLAN's build machine, which breaks linking.
export VLC_CONFIGURE_ARGS="${VLC_CONFIGURE_ARGS:-} --disable-sparkle --disable-ncurses"
"$SRC_DIR/extras/package/macosx/build.sh" -d -a "$VLC_ARCH" -j "$JOBS"
echo "::endgroup::"

VERSION="$(sed -n 's/^#define PACKAGE_VERSION "\(.*\)"/\1/p' config.h)"
APP="$BUILD_DIR/VLC Redesign.app"

echo "::group::Prepare VLC Redesign.app"
rm -rf "$APP"
cp -Rp "$BUILD_DIR/VLC.app" "$APP"
# Unsigned CI builds: give the bundle an ad-hoc signature so Apple Silicon
# will run it (users still need to right-click > Open the first time).
codesign --force --deep --sign - "$APP"
codesign --verify --verbose "$APP" || true
echo "::endgroup::"

echo "::group::Check the self-installer is in the app"
# The installer lives in the macosx interface plugin (VLCRedesignInstaller.m)
grep -q VLCRedesignSkipInstallOffer "$APP/Contents/Frameworks/plugins/libmacosx_plugin.dylib" \
    || { echo "::error::VLC Redesign self-installer missing from libmacosx_plugin"; exit 1; }
echo "::endgroup::"

echo "::group::Package VLC Redesign.app"
# Like Jace Launcher, the Mac download is just the .app (zipped so it survives
# the download). Opened from Downloads, it installs itself into Applications.
cd "$BUILD_DIR"
ditto -c -k --sequesterRsrc --keepParent "VLC Redesign.app" \
    "$OUT_DIR/VLC-Redesign-$VERSION-macOS-$ARCH.app.zip"
echo "::endgroup::"

ls -la "$OUT_DIR"
