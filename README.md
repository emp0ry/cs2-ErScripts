# ErScripts
ErScripts is a multi-script tool designed for Counter-Strike 2 (CS2). It enhances gameplay with various features without interacting with game memory or injecting into the game, making it fully safe to use on Valve servers like Premier or Matchmaking.

## Overview
ErScripts runs outside the game by:
- Creating an overlay outside of CS2 and tracking its window position and size.
- Sending commands through configs and binds (e.g., `bind "KEY" "exec erscripts"`).
- Pulling game data via Valve’s [**Game State Integration (GSI)**](https://developer.valvesoftware.com/wiki/Counter-Strike:_Global_Offensive_Game_State_Integration) and console logs.

## How ErScripts Works

### Core Mechanisms
- **Overlay**: Tracks CS2’s window externally.
- **Command Sending**: Uses config files (e.g., `erscripts.cfg`) and binds to control the game.
- **Console Output**: Reads `console.log` with the `-condebug` parameter.
- **Game State Integration (GSI)**: Creates `gamestate_integration_erscripts.cfg` for real-time data (e.g., weapons, bomb status).
- **Game Path**: Fetched from the Windows Registry.
- **Launch Options**: Pulled from Steam’s `localconfig.vdf`.

## Features

Here’s what ErScripts brings to the table:
- **Auto Accept**: Accepts match automatically.
- **Pixel Trigger**: Fires when the center-screen color shifts (e.g., spotting enemies).
- **Sniper Crosshair**: Draws a custom crosshair for snipers.
- **Bomb Timer**: Counts down bomb explosions with defuse kit alerts.
- **RGB Crosshair**: Adds a gradient effect to your crosshair.
- **Knife Switch**: Flips knife hand based on your weapon.
- **Auto Pistol**: Turns pistols into rapid-fire weapons.
- **Anti AFK**: Stops you from getting kicked for inactivity.
- **Keystrokes**: Displays your key presses (WASD, mouse).
- **Long Jump**: Combines duck and jump for longer leaps.
- **Jump Throw**: Jumps and throws grenades with one bind.
- **Drop Bomb**: Drops the bomb fast and switches back.
- **Watermark**: Shows game info (e.g., ping, time).
- **Capture Bypass**: Makes the overlay invisible in recordings or streams.
- **FPS Limiter**: Caps FPS for smoother performance.

## Installation

1. Grab the latest release from [GitHub](https://github.com/emp0ry/ErScripts/releases).
2. Unzip it wherever you want.
3. Add this Command-Line Parameters `-conclearlog -condebug +bind scancode104 exec erscripts1` in CS2.
4. For the first start, run `ErScripts.exe` without CS2 open.
5. Configure settings in the menu and enjoy the game 😊.

## Configuration

Customize these settings:

### Pixel Trigger
- `Displacement`: X/Y offset from center of screen for getting pixels and it needs to be configured to avoid conflicts with crosshair.
- `Threshold`: Color sensitivity.
- `Delay`: Delay before firing.

### Auto Accept
- `Waiting Time`: Waiting time in seconds until button is found.

### Bomb Timer
- `Scale`: Size.
- `Gradient`: Gradient bomb icon on/off.
- `Transparency`: Background opacity.

### Sniper Crosshair
- `Reload Icon`: Updates crosshair configuration.

### Keystrokes
- `Scale`: Size.
- `Gradient`: Gradient text on/off.
- `Animation Speed`: Pressing animation speed.
- `Pressed Color`: Pressed color.
- `Released Color`: Released color.
- `Transparency`: Opacity in released state.

### Watermark
- `Gradient`: Gradient text on/off.
- `Transparency`: Opacity of background.
- `Ping Update Rate`: Ping refresh rate.

### FPS Limiter
- `FPS Limiter`: Overlay FPS cap for more performance.

### Gradient Manager
- `Steps`: Set how smooth the color fade is.
- `Delay`: How fast it switches colors.
- `Start Hue`: Pick the starting color tone (like a rainbow slider).
- `End Hue`: Choose the ending color tone.
- `Saturation`: Control how vivid the colors are (0 is gray, 1 is bold).
- `Value*`: Adjust brightness (0 is dark, 1 is bright).

### RGB Crosshair, Knife Switch, Auto Pistol, Anti AFK, Long Jump, Jump Throw, Drop Bomb, Capture Bypass
- Doesn't have config.

## How Features Work

- **Auto Accept**: Gets the log of found match in `console.log` and clicks Accept.
- **Pixel Trigger**: Watches center-screen color changes and shoots.
- **Sniper Crosshair**: Detects sniper weapons via GSI, gets crosshair config by sending command via config and getting values from `console.log`, and draws crosshair like in CS2.
- **Bomb Timer**: Tracks bomb status via GSI, highlights defuse limits by changing timer color.
- **RGB Crosshair**: Changing crosshair colors with sending console commands via config.
- **Knife Switch**: Flips hands with `switchhands` when knife’s out.
- **Auto Pistol**: Spams `attack` via sending console command for pistol fire.
- **Anti AFK**: Hits "W" at round start to unflag afk flag.
- **Keystrokes**: Shows WASD/mouse inputs on-screen.
- **Long Jump / Jump Throw / Drop Bomb**: Sends combo commands via sending console command.
- **Watermark**: Displays game/system info.
- **Capture Bypass**: Bypassing ErScripts overlay from recording and streaming.

## Donation

[!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/emp0ry)


## Internal Libraries

ErScripts uses:
- **[Dear ImGui](https://github.com/ocornut/imgui)**: For the overlay and UI.
- **[nlohmann/json](https://github.com/nlohmann/json)**: For GSI and config parsing.
- **[cpp-httplib](https://github.com/yhirose/cpp-httplib)**: For GSI parsing.
- **Windows API**: For registry, window tracking, and inputs.
- **Standard C++**: For file handling and core logic.

## License

MIT License. See [LICENSE](LICENSE.txt) for details.