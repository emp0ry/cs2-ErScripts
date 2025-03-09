# ErScripts
ErScripts is multi scripts in one, for Counter-Strike 2. Doesn't interact with game memory and doesn't inject into the game. It is full safe to play in Valve servers such as a Premier or Matchmaking.

## How ErScripts works?
### Overlay
ErScripts creating overlay outside Counter-Strike 2 and tracking position and size of CS2 on the screen.
### Sending commands
ErScripts sending commands to Counter-Strike 2 via configs and bind. For example it is creating config erscripts.cfg and with bind sending it, like `bind "KEY" "exec erscripts"`.
### Getting output from console
Counter-Strike 2 have [Command-Line Parameter](https://developer.valvesoftware.com/wiki/Command_line_options) `-condebug` it is logging game console to `Counter-Strike Global Offensive\game\csgo\console.log` file.
### Getting what weapon you use, or is bomb planted
Counter-Stike 2 have [Game State Integration](https://developer.valvesoftware.com/wiki/Counter-Strike:_Global_Offensive_Game_State_Integration). In first start ErScripts creating GSI (Game State Integration) config gamestate_integration_erscripts.cfg and use game data from here, such as a active weapon, is bomb planted, etc.
### Getting game path
ErScripts getting game path from Windows Registry.
### Getting Game Command-Line Parameter
ErScripts getting Counter-Strike 2 Command-Line Parameter from `"Steam\userdata\{userID}\config\localconfig.vdf"` file.

# Functionality
- ## Auto Accept
    - ### About
        - When match found going to Counter-Strike 2 (if it is not in CS2) and pressing **Accept** button.
    - ### How it's working?
        - It is know that the game was found from `console.log` file. (About `console.log` [Click](How ErScripts works?->Getting output from console)).
- ## Pixel Trigger
    - ### About
        - When pressing Pixel trigger bind, it is waiting when in center color changes, and when changes it is shoting.
    - ### Configuration
        - Threshold - How big is the difference between the saved color and the real-time color.
        - Disaplcement - How much are the pixel coordinates displacement from the center, there are 2 of them x, y and -x, -y from center.
- ## Sniper Crosshair
    - ### About
        - When active weapon is sniper it is drawing crosshair
    - ### How it's working?
        - From [Game State Integration](https://developer.valvesoftware.com/wiki/Counter-Strike:_Global_Offensive_Game_State_Integration) config it is getting active weapon, and if it's a sniper drawing similar crosshair from Counter-Strike 2.
        - Counter-Strike 2 crosshair config taking from command output. It is writing in console `print_changed_convars` and getting crosshair config values.
- ## Bomb Timer
    - ### About
        - When Bomb planted it is giving time when bomb'll be exploed
    - ### How it's working?
        - From [Game State Integration](https://developer.valvesoftware.com/wiki/Counter-Strike:_Global_Offensive_Game_State_Integration) config it is getting if bomb planted, and getting if you have defuse kit for make red if you cann't defuse
- ## RGB Crosshair
    - ### About
        - Making Counter-Strike 2 crosshair Gradient.
    - ### How it is working
        - Changing crosshair color via sending commands.
- ## Knife Switch
    - ### About
        - It is switching knife hand, for example if weapon in left hand, knife changing to right hand.
    - ### How it is working
        - When active weapon is knife sending `switchhands` command.
- ## Auto Pistol
    - ### About
        - You can use pistol like automatic weapon
    - ### How it is working
        - When active weapon is pistol and left mouse button is pressed, it is spamming `attack` command
- ## Anti AFK
    - ### About
        - You'll not get kicked from the server because of afk
    - ### How it is working
        - In start of the round it is pressing W to unflag afk state
- ## Keystrokes
    - ### About
        - It is showing what key you pressed,  w, a, s, d, left mouse, right mouse.
    - ### Configuration
        - Animation Speed - it is a speed how fast animation of pressed button will last
