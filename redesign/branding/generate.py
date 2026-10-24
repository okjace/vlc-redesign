#!/usr/bin/env python3
"""
Generate the VLC Redesign installer artwork.

The look follows Juxtopposed's "I Redesigned VLC After 25 Years"
(https://www.youtube.com/watch?v=ERFeOgqk324): near-black layered surfaces,
VLC orange as the only accent, and slanted orange shapes like the
video's end card.

Requirements: rsvg-convert (librsvg), ImageMagick 7 (`magick`), and the
Inter font (pass --font-dir if it is not installed system-wide).

Usage: redesign/branding/generate.py [--font-dir DIR]
"""

import argparse
import base64
import os
import subprocess
import tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
CONE_SVG = os.path.join(ROOT, "modules/gui/qt/pixmaps/misc/cone.svg")

ACCENT = "#FF8800"
TEXT = "#F2F2F2"
MUTED = "#8A8A8A"

FONT = "Inter, 'Noto Sans', 'DejaVu Sans', sans-serif"


def cone_href():
    with open(CONE_SVG, "rb") as f:
        return "data:image/svg+xml;base64," + base64.b64encode(f.read()).decode()


def cone(x, y, size):
    return f'<image href="{cone_href()}" x="{x}" y="{y}" width="{size}" height="{size}"/>'


def defs():
    return f"""
  <defs>
    <linearGradient id="bg" x1="0" y1="0" x2="0.35" y2="1">
      <stop offset="0" stop-color="#181818"/>
      <stop offset="1" stop-color="#0A0A0A"/>
    </linearGradient>
    <radialGradient id="glow" cx="0.5" cy="0.5" r="0.5">
      <stop offset="0" stop-color="{ACCENT}" stop-opacity="0.35"/>
      <stop offset="1" stop-color="{ACCENT}" stop-opacity="0"/>
    </radialGradient>
    <linearGradient id="band" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0" stop-color="#FF9A26"/>
      <stop offset="1" stop-color="#E07400"/>
    </linearGradient>
  </defs>"""


def wordmark(x, y, size, anchor="start"):
    """'VLC' in white followed by 'Redesign' in orange, on one baseline."""
    return (f'<text x="{x}" y="{y}" font-family="{FONT}" font-size="{size}" '
            f'font-weight="700" text-anchor="{anchor}" letter-spacing="-0.02em" fill="{TEXT}">'
            f'VLC <tspan fill="{ACCENT}" font-weight="600">Redesign</tspan></text>')


def nsis_welcome():
    """MUI welcome/finish side panel, 164x314."""
    w, h = 164, 314
    return f"""<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">
  {defs()}
  <rect width="{w}" height="{h}" fill="url(#bg)"/>
  <!-- slanted orange bands, after the video's end card -->
  <path d="M118 0 H164 V96 H150 Q144 96 141 102 L96 0 Z" fill="url(#band)"/>
  <path d="M0 214 L38 314 H0 Z" fill="url(#band)" opacity="0.9"/>
  <circle cx="82" cy="104" r="62" fill="url(#glow)"/>
  {cone(50, 70, 64)}
  <text x="82" y="170" font-family="{FONT}" font-size="30" font-weight="800" text-anchor="middle"
        letter-spacing="-0.03em" fill="{TEXT}">VLC</text>
  <text x="82" y="190" font-family="{FONT}" font-size="15" font-weight="600" text-anchor="middle"
        fill="{ACCENT}">Redesign</text>
  <rect x="62" y="202" width="40" height="2" rx="1" fill="#2F2F2F"/>
  <text x="82" y="222" font-family="{FONT}" font-size="7.5" text-anchor="middle" fill="{MUTED}">The all-time favourite</text>
  <text x="82" y="232" font-family="{FONT}" font-size="7.5" text-anchor="middle" fill="{MUTED}">media player, reimagined.</text>
  <text x="96" y="292" font-family="{FONT}" font-size="6.5" text-anchor="middle" fill="{MUTED}">Design: Juxtopposed</text>
  <text x="96" y="302" font-family="{FONT}" font-size="6.5" text-anchor="middle" fill="{MUTED}">VLC: VideoLAN</text>
</svg>"""


def nsis_header():
    """MUI page header image, 150x57. Sits on the (light) header strip."""
    w, h = 150, 57
    return f"""<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">
  {defs()}
  <rect width="{w}" height="{h}" fill="#111111"/>
  <path d="M112 0 H150 V57 H138 Z" fill="url(#band)"/>
  {cone(10, 12, 33)}
  <text x="49" y="27" font-family="{FONT}" font-size="16" font-weight="800" letter-spacing="-0.03em" fill="{TEXT}">VLC</text>
  <text x="49" y="42" font-family="{FONT}" font-size="10" font-weight="600" fill="{ACCENT}">Redesign</text>
</svg>"""


def dmg_background(scale=1):
    """DMG window background. dmgbuild window is 573x340 incl. title bar; the
    icons sit at (181,168) and (392,168) in content coordinates."""
    w, h = 573, 297
    return f"""<svg xmlns="http://www.w3.org/2000/svg" width="{w*scale}" height="{h*scale}" viewBox="0 0 {w} {h}">
  {defs()}
  <rect width="{w}" height="{h}" fill="url(#bg)"/>
  <path d="M500 0 H573 V118 H556 Q549 118 546 112 L470 0 Z" fill="url(#band)"/>
  <path d="M0 236 L26 297 H0 Z" fill="url(#band)" opacity="0.9"/>
  <ellipse cx="181" cy="160" rx="96" ry="80" fill="url(#glow)" opacity="0.6"/>
  {cone(22, 20, 26)}
  {wordmark(54, 39, 18)}
  <text x="22" y="62" font-family="{FONT}" font-size="11" fill="{MUTED}">Drag VLC Redesign into your Applications folder to install.</text>
  <!-- drop arrow between the app and Applications -->
  <g fill="none" stroke="{ACCENT}" stroke-width="3" stroke-linecap="round" stroke-linejoin="round">
    <path d="M250 160 H318" stroke-dasharray="1 9"/>
    <path d="M314 150 L326 160 L314 170"/>
  </g>
  <text x="{w/2}" y="282" font-family="{FONT}" font-size="9" text-anchor="middle" fill="#5A5A5A">UI design by Juxtopposed  ·  VLC media player by VideoLAN  ·  Unofficial build</text>
</svg>"""


def app_icon(size=1024):
    """Cone on a dark rounded tile, used for installer / AppImage / volume icons."""
    return f"""<svg xmlns="http://www.w3.org/2000/svg" width="{size}" height="{size}" viewBox="0 0 1024 1024">
  {defs()}
  <rect x="64" y="64" width="896" height="896" rx="200" fill="url(#bg)"/>
  <rect x="64.5" y="64.5" width="895" height="895" rx="200" fill="none" stroke="#2F2F2F" stroke-width="3"/>
  <circle cx="512" cy="560" r="380" fill="url(#glow)" opacity="0.8"/>
  {cone(212, 200, 600)}
</svg>"""


def render(svg, out_png, width=None, env=None):
    with tempfile.NamedTemporaryFile("w", suffix=".svg", delete=False) as f:
        f.write(svg)
        path = f.name
    try:
        cmd = ["rsvg-convert", path, "-o", out_png]
        if width:
            cmd[1:1] = ["-w", str(width)]
        subprocess.run(cmd, check=True, env=env)
    finally:
        os.unlink(path)


def magick(*args):
    subprocess.run(["magick", *args], check=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--font-dir", help="directory containing Inter .ttf files")
    args = ap.parse_args()

    env = dict(os.environ)
    tmp = tempfile.mkdtemp()
    if args.font_dir:
        conf = os.path.join(tmp, "fonts.conf")
        with open(conf, "w") as f:
            f.write(f"""<?xml version="1.0"?><!DOCTYPE fontconfig SYSTEM "fonts.dtd">
<fontconfig><dir>{os.path.abspath(args.font_dir)}</dir><include ignore_missing="yes">/etc/fonts/fonts.conf</include>
<cachedir>{tmp}/cache</cachedir></fontconfig>""")
        env["FONTCONFIG_FILE"] = conf

    out = os.path.join(ROOT, "redesign/branding/out")
    os.makedirs(out, exist_ok=True)

    # Windows / NSIS (render at 2x, downsample for clean anti-aliasing)
    nsis = os.path.join(ROOT, "extras/package/win32/NSIS")
    render(nsis_welcome(), f"{tmp}/welcome.png", 164 * 2, env)
    magick(f"{tmp}/welcome.png", "-resize", "164x314!", "-background", "#0A0A0A", "-flatten",
           "-type", "TrueColor", f"BMP3:{nsis}/vlc_branding.bmp")
    render(nsis_header(), f"{tmp}/header.png", 150 * 2, env)
    magick(f"{tmp}/header.png", "-resize", "150x57!", "-background", "#111111", "-flatten",
           "-type", "TrueColor", f"BMP3:{nsis}/vlc_header.bmp")

    # macOS DMG background: 1x + 2x in one TIFF (what Finder expects)
    dmg = os.path.join(ROOT, "extras/package/macosx/dmg")
    render(dmg_background(1), f"{tmp}/dmg1.png", None, env)
    render(dmg_background(2), f"{tmp}/dmg2.png", None, env)
    magick(f"{tmp}/dmg1.png", "-density", "72", "-units", "PixelsPerInch",
           f"{tmp}/dmg2.png", "-density", "144", "-units", "PixelsPerInch",
           "-alpha", "off", "-compress", "lzw", f"{dmg}/background.tiff")

    # Icons
    sizes = [16, 24, 32, 48, 64, 128, 256, 512, 1024]
    for s in sizes:
        render(app_icon(), f"{out}/icon-{s}.png", s, env)
    magick(*[f"{out}/icon-{s}.png" for s in (16, 24, 32, 48, 64, 128, 256)],
           os.path.join(ROOT, "redesign/branding/vlc-redesign.ico"))
    for s in sizes:
        if s not in (256, 512):
            os.unlink(f"{out}/icon-{s}.png")
    os.replace(f"{out}/icon-256.png", os.path.join(ROOT, "redesign/branding/vlc-redesign.png"))
    os.replace(f"{out}/icon-512.png", os.path.join(ROOT, "redesign/branding/vlc-redesign-512.png"))
    os.rmdir(out)

    # Previews for the README
    prev = os.path.join(ROOT, "redesign/branding/previews")
    os.makedirs(prev, exist_ok=True)
    magick(f"{tmp}/dmg2.png", f"{prev}/dmg-background.png")
    magick(f"{tmp}/welcome.png", f"{prev}/nsis-welcome.png")
    magick(f"{tmp}/header.png", f"{prev}/nsis-header.png")
    print("artwork written")


if __name__ == "__main__":
    main()
