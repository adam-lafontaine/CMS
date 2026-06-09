## Change Log

### v0.13 [ 2026-05-25 - 2026-06-06 ]: Upgrade visuals
* Fixed point tile coordinates and physics
* Smooth camera motion
* Enable in-game framerate adjustement
* Fix window texture scaling mode (sharper resolution)
* Upgrade engine image texture inspection


### v0.12 [ 2026-05-16 - 2026-05-21 ]: Sprite and camera movement
* Add more sprites
* Camera automatically moves back to origin
* Screen shake when the player lands
* Add a flying bat sprite that follows the player
    * Hold [ X ] to make the bat fly away
    * Release [ X ] to make the bat return            
* Keyboard controls match gamepad (A, B, X, Y)
* Updated controls
    * Run/Stop [ A ] or [ Enter ]
    * Jump [ Y ] or [ Space ]
    * Cycle sprite theme [ B ]
    * Control bat sprite [ X ]

    
### v0.11 [ 2026-05-15 ]: Custom font        
* Add game font
* Display game controls on screen
   

### v0.10 [ 2026-04-24 - 2026-05-14 ]: Tile generation
* Random tiles spawned for the ground and background
* Different tilesets per theme - [ Ctrl ]
* Player sprite falls from the sky
* Allow camera to move outside of gameplay scene
* Redesign asset storage layout
* Tiles spawn using new grid system
        
    
### v0.9 [ 2026-03-23 - 2026-04-22 ]: Asset grouping
* Group sprites together for variations
* e.g. trees, rocks, grass, ladder, conveyor
* Update Zig build system to version 0.16
* Update SDL3, static link SDL3
* All builds with SDL3
* Add SDL3 window debug message
* Refactor SDL2 and SDL3 window rendering
        
    
### v0.8 [ 2026-03-16 - 2026-03-21 ]: Animated sky        
* Moving sky in the background, using asset pack files
* Parralax background motion
* Gradual lighting mode change from day to night
* Running sprite on title screen
* Added new themes
        
    
### v0.7 [ 2026-03-07 - 2026-03-14 ]: Animated background sprites and sprite themes
* Sprites and tiles grouped by asset pack theme
* Change theme - [ Ctrl ]


### v0.6 [ 2026-03-04 - 2026-03-06 ]: Random background sprites
* Spawn non-animated sprites in the background
* Rocks, trees, grass
* Spawn random ground tiles


### v0.5 [ 2026-02-04 - 2026-03-02 ]: Mobile version with touch support
* Create version for web (Emscripten/WASM)
* Integrate with buttons on a web page
* Size viewport to make the user's screen


### v0.4 [ 2026-01-16 - 2026-02-03 ]: Punk jumps
* Add jump animation
* Start/Stop - [ Enter ]
* Jump - [ Spacebar ]


### v0.3 [ 2026-01-13 - 2026-01-14 ]: Data setup
* Create tile and sprite SOA
* No release
        
    
### v0.2 [ 2025-12-20 - 2026-01-12 ]: Punk actually runs
* Player run movement and animation
* Moving randomized backgrounds
* Ground tile relative motion
* Separate title screen


### v0.1 [ 2025-12-06 - 2025-12-20 ]: Show title
* Create asset binary
* Load assets from binary
* Draw background images
* Draw tiles
* Display title
* Memory allocations
* Add RNG
* Windows SDL2 build
* Ubuntu, WASM SDL3 build
* Zig build system
