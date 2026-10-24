**VLC**, redesigned: an unofficial build of the VLC media player with an interface based on
**["I Redesigned VLC After 25 Years"](https://www.youtube.com/watch?v=ERFeOgqk324) by [Juxtopposed](https://www.youtube.com/@juxtopposed)**.

VLC media player is made by [VideoLAN](https://www.videolan.org) and its contributors. This project is not affiliated with or endorsed by VideoLAN or Juxtopposed. It installs side by side with VideoLAN's VLC.

### Downloads

| Platform | File |
| --- | --- |
| Windows 10/11 (x64) | `VLC-*-Windows-x64-Setup.exe` (or the portable `.zip`) |
| macOS, Apple Silicon | `VLC-*-macOS-arm64.app.zip` |
| macOS, Intel | `VLC-*-macOS-x86_64.app.zip` |
| Linux x86_64 | `VLC-*-Linux-x86_64.AppImage` |

### Installing

- **macOS:** unzip and open **VLC.app** (the first time, right-click it → **Open**, because the app isn't notarized). It installs itself into Applications, adds Dock and Desktop shortcuts and clears macOS's "damaged" warning. If VideoLAN's VLC is already there, you can replace it or keep both.
- **Windows:** run the Setup `.exe` (SmartScreen: **More info** → **Run anyway**). It installs to `Program Files\VLC`, separate from VideoLAN's VLC.
- **Linux:** double-click the AppImage. A setup wizard installs it with a menu entry, desktop shortcut and the `vlc-redesign` command. If double-clicking does nothing, run `chmod +x VLC-*.AppImage` once and open it again.

Built from the in-development VLC 4.0 branch, so expect nightly-level stability.
