<p align="center">
  <img src="https://erscripts.emp0ry.com/site/icon.svg" alt="ErScripts logo" width="180">
</p>

<h1 align="center">ErScripts</h1>

<p align="center">
  <b>Crosshairs, overlays and config backups for Counter-Strike 2.</b>
</p>

<p align="center">
  <a href="https://github.com/emp0ry/cs2-ErScripts/releases/latest">
    <img alt="Latest Release" src="https://img.shields.io/github/v/release/emp0ry/cs2-ErScripts?style=flat-square&amp;logo=github&amp;logoColor=white&amp;label=Release&amp;color=8b5cf6&amp;labelColor=1f2937">
  </a>
  <a href="https://github.com/emp0ry/cs2-ErScripts/releases">
    <img alt="Total Downloads" src="https://img.shields.io/github/downloads/emp0ry/cs2-ErScripts/total?style=flat-square&amp;logo=github&amp;logoColor=white&amp;label=Downloads&amp;color=ff6d00&amp;labelColor=1f2937">
  </a>
  <a href="https://www.buymeacoffee.com/emp0ry">
    <img alt="Buy Me a Coffee" src="https://img.shields.io/badge/Buy%20Me%20a%20Coffee-emp0ry-FFDD00?style=flat-square&amp;logo=buymeacoffee&amp;logoColor=111827">
  </a>
  <br>
  <img alt="Platforms: Windows 10 and 11, x64" src="https://img.shields.io/badge/Platforms-Windows%2010%20%7C%2011%20%C2%B7%20x64-111827?style=flat-square">
  <img alt="C++20" src="https://img.shields.io/badge/C%2B%2B-20-00599C?style=flat-square&amp;logo=cplusplus&amp;logoColor=white&amp;labelColor=1f2937">
  <a href="LICENSE.txt">
    <img alt="License: MIT" src="https://img.shields.io/badge/License-MIT-8b5cf6?style=flat-square&amp;labelColor=1f2937">
  </a>
</p>

<p align="center">
Fine-tune your crosshair, keep round information visible and back up your CS2 settings in one compact Windows app. External, free and open source.
</p>

<p align="center">
  <a href="https://github.com/emp0ry/cs2-ErScripts/releases/tag/v2.0.1"><b>Download 2.0.1</b></a> ·
  <a href="https://erscripts.emp0ry.com/">Website</a> ·
  <a href="https://github.com/emp0ry/cs2-ErScripts/issues">Report a bug</a>
</p>

<p align="center">
  <img src="https://erscripts.emp0ry.com/img/screenshots/general.png" alt="ErScripts 2.0.0 General menu" width="820">
</p>

## What's new in 2.0.2

- SHA-256 verification through Windows CNG before automatic update execution or installation, with native restart and rollback.
- Consistent GSI snapshots, bounded log monitoring, safer input release and configuration export concurrency.
- Correct UTF-8 and escaped quotes in Steam VDF files, validated registry values and overlay positioning recovery.
- Time-based gradient animation without a permanent worker thread, synchronized logging and reliable audio cleanup.
- Consistent C++20 source naming, private JSON parsing, formatting, static analysis and CMake presets.

## What's new in 2.0.1

- Managed worker shutdown and synchronized runtime settings.
- Safer configuration saves, exports, GSI updates, and graphics resource cleanup.
- UI Access initialization is disabled in Debug builds and retained in Release builds.

## What's new in 2.0.0

- Compact dark menu with search, six tabs and settings that expand per feature.
- Crosshair editor with ten styles, colors, outlines, scope and grenade controls, and a live preview.
- Sniper shot animation driven by your own GSI ammo changes, with separate profiles for AWP, SSG 08, SCAR-20 and G3SG1.
- Automatic resolution tracking and crosshair scaling when CS2's render size changes.
- Config export from saved files or a live CS2 capture.
- Consistent overlay colors, improved spacing and a default overlay limit of 120 FPS.

## Setup

1. Download `ErScripts.exe` from the release above and place it in a writable folder.
2. In Steam, open **Counter-Strike 2 > Properties > General** and add these launch options:

   ```text
   -conclearlog -condebug +bind scancode104 exec erscripts1
   ```

3. For the first run, start ErScripts before CS2 so it can create its config and GSI files. If CS2 is already open when those files are created, restart the game.
4. Press **Insert** to show or hide the menu. **End** exits. Change these keys in **Settings > App shortcuts**.

Use the arrow on a feature's right to open its settings. **Save** and **Load** use your local `configs/default.json` profile. For fullscreen overlays, leave **Disable fullscreen optimizations** unchecked in `cs2.exe` compatibility settings.

## Features

| Tab | Controls |
| --- | --- |
| General | Auto Accept, Pixel Trigger, Auto Stop, Angle Bind, Knife Switch, Auto Pistol, Anti AFK, Custom Binds |
| Crosshair | Crosshair editor, Sniper Crosshair, Recoil Crosshair, RGB Crosshair |
| Overlay | Bomb Timer, Keystrokes, Watermark |
| Audio / chat | Kill Sound, Round Start Alert, Kill Say, Chat Spammer |
| Settings | Performance, Capture Bypass, Gradient, App shortcuts |
| Config | Export saved files, Capture from CS2 |

### Crosshairs

Edit dimensions, colors and outlines, then choose **Apply to CS2** or **Copy commands**. Reload reads your saved CS2 crosshair. Dimensions use CS2's limits and resolution scaling; controls unused by the selected style are disabled.

Sniper animation uses only your active, living player's GSI data. Spectator data is ignored. Magazine decreases confirm shots; reloads, weapon changes and session changes reset the tracker. GSI does not expose exact shot timing, movement or aim punch, so dynamic behavior can differ from CS2. No game memory is read.

### Config backups

**Export saved files** backs up the active Steam account. **Capture from CS2** also reads exposed console variables, binds and aliases. Capture hides the menu; return to CS2 and wait for completion. **Open export folder** shows the result under `exports/`.

Exports include a generated `erscripts-export.cfg`, `settings.json`, original settings files, and detected local scripts. Video and machine settings are preserved as original files. To restore the generated CFG, copy it into `game/csgo/cfg` and run `exec erscripts-export` in CS2. Export does not apply settings automatically. Saved files can lag behind unsaved changes, and hidden engine state cannot be exported.

## Screenshots

All captures below are from 2.0.0. Click an image to open it at full size.

| Crosshair editor | Crosshair features |
| --- | --- |
| ![Crosshair editor with preview](https://erscripts.emp0ry.com/img/screenshots/crosshair.png) | ![Sniper, recoil and RGB crosshair controls](https://erscripts.emp0ry.com/img/screenshots/crosshair2.png) |

| Overlay | Audio / chat |
| --- | --- |
| ![Bomb timer, keystrokes and watermark controls](https://erscripts.emp0ry.com/img/screenshots/overlay.png) | ![Sound and chat controls](https://erscripts.emp0ry.com/img/screenshots/audio_chat.png) |

| Settings | Config export |
| --- | --- |
| ![Performance, gradient and shortcut controls](https://erscripts.emp0ry.com/img/screenshots/settings.png) | ![Saved and live CS2 config export](https://erscripts.emp0ry.com/img/screenshots/config.png) |

| Bomb Timer | Keystrokes |
| --- | --- |
| ![Bomb timer overlay](https://erscripts.emp0ry.com/img/screenshots/bomb_timer.png) | ![WASD and mouse keystrokes overlay](https://erscripts.emp0ry.com/img/screenshots/keystrokes.png) |

## Use responsibly

Auto Stop, Pixel Trigger, Angle Bind and Auto Pistol require an explicit risk acknowledgement each time you enable them. Loading a profile leaves them disabled. Automation can violate game, server or platform rules and lead to restrictions or bans. External operation does not guarantee safety or permission. ErScripts is not affiliated with Valve.

## Build

Source layout: `src/app` owns startup and shutdown; `src/game` integrates CS2 and Steam; `src/features` owns feature workers; `src/graphics` renders the existing overlay; `src/platform` owns native input, files and audio; `src/update` implements automatic updates. Configuration and shared snapshots are in `src/core`. Windows resources are in `resources`; the Visual Studio solution remains `ErScripts.sln`.

Requires Windows 10/11, Visual Studio with the C++ desktop workload, Windows SDK and the v143 toolset. The application uses C++20.

Project-owned C++ files use `snake_case.cpp` and `snake_case.hpp`. The C UIAccess interface, resource definitions and version macros retain `.h`. Methods, variables and constants use `camelCase`; private instance members end in `_`. C++20 provides the required facilities without experimental module integration or a newer language requirement. Third-party code and external API names retain their own conventions.

```powershell
git clone --recurse-submodules https://github.com/emp0ry/cs2-ErScripts.git
cd cs2-ErScripts
```

Open `ErScripts.sln`, select **Release | x64** and build. If you cloned without submodules, run `git submodule update --init --recursive` first.

Alternatively, use CMake 3.24 or newer from a Visual Studio Developer PowerShell with Ninja:

```powershell
cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release
```

Use `windows-debug` for Debug builds. Presets use the architecture selected by the Developer shell on first configuration; use separate build directories when changing architecture. Run `clang-format --dry-run --Werror` on owned source files and `clang-tidy -p build/windows-debug src/game/game_console.cpp` from the same shell for local checks. The Visual Studio project is in `windows/erscripts.vcxproj`.

`--preview` opens the menu without CS2. `--no-update` skips automatic updates.

Updates require an uploaded EXE with a valid `sha256:` digest and matching size in the [GitHub release API](https://docs.github.com/en/rest/releases/assets). Downloads use HTTPS certificate validation, a 64 MiB limit and Windows CNG SHA-256 verification before execution. A verified temporary EXE waits on the original process handle, preserves the previous EXE, installs and restarts the new version, and restores the previous file if installation or startup fails. Successful startup means reaching the executable entry point, not a full CS2 integration test. The previous EXE remains in `.erscripts-update-*/previous.exe` for recovery.

SHA-256 detects corruption and mismatched artifacts; an EXE and digest controlled by the same GitHub source do not independently authenticate the publisher. No signing keys or publisher certificates are configured.

GSI packets publish one consistent game-state snapshot. State expires after 25 seconds without a valid packet. Partial HTTP requests are interrupted after two seconds so shutdown cannot be held open by a client sending incomplete data.

Dependencies are pinned in `.gitmodules`: [Dear ImGui 1.91.5](https://github.com/ocornut/imgui), [JSON for Modern C++ 3.11.3](https://github.com/nlohmann/json), and [cpp-httplib 0.19.0](https://github.com/yhirose/cpp-httplib).

## Website

The [product website](https://erscripts.emp0ry.com/) is also available at [emp0ry.com/cs2-ErScripts/](https://emp0ry.com/cs2-ErScripts/). Website sources and publication are maintained in the private emp0ry.com project. Completed application releases notify its central static build; this repository contains the application and its technical documentation.

## License

[MIT](LICENSE.txt). Dependency licenses are included in their submodules. The website uses [Geist under the SIL Open Font License](https://erscripts.emp0ry.com/site/fonts/OFL.txt).

[Support the project](https://www.buymeacoffee.com/emp0ry) · [More by emp0ry](https://emp0ry.com/)
