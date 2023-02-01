# SndVolTray
A tray application for Windows' Volume Mixer (`SndVol.exe`) & remake of [ClassicMixer](https://github.com/7gxycn08/ClassicMixer) in **`C`**.

# Features
1. The application spawns a `SndVol.exe` instance at the notification area on the monitor, the tray icon from invoked from.
2. The application attempts to emulate behavior of taskbar flyouts for `SndVol.exe`.
3. Extremely lightweight with UPX compression!

# Usage
1. Download the latest release from [GitHub Releases](https://github.com/Aetopia/SndVolTray/releases/latest).
2. Start `SndVolTray.exe`.
3. Tray Icon Operations:    
    - Left clicking the icon once will invoke `SndVol.exe` or Volume Mixer.
    - Double right clicking will prompt if you want to exit the application or not.

# Building
1. Install `GCC` and [`UPX`](https://upx.github.io) for optional compression.
2. Run `build.bat`.
3. Run `SndVolTray.exe`.
