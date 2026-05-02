<h1 align ="center"> ✈️ Dusty Air Rush ✈️</h1>
<p align="center">
  <img src="docs/readme-assets/menu.png" alt="Dusty Air Rush main menu" width="100%">
</p>

<p align="center">
  A fan-made 3D flying game inspired by Dusty from Disney's <i>Planes</i>, built with C++, OpenGL, GLFW, GLAD, ImGui, and an ECS-style gameplay architecture.
</p>

## Overview

`Dusty Air Rush` is a stylized aerial race where Dusty flies through a curved sky track, clears scoring rings, dodges moving tornados, collects pickups, and pushes to the finish line before health runs out.

The project includes a complete menu flow, persistent scoring, themed visuals, cassette music switching, and both keyboard and gamepad support.

## What The Game Has

### Flight Gameplay

- Forward flight with real-time steering and banking
- Pitch control for climbing and diving
- Manual roll control for aerial alignment
- Speed boost and slow-flight control
- Backflip / emote move
- Headlights toggle during gameplay
- Mouse-assisted look / aim while flying

### Track And Challenge Systems

- Procedurally placed rings across the course
- Moving tornado hazards with damage and pull-in behavior
- Finish line that only accepts a full ring clear
- Collectible coins in classic mode
- Collectible bows in girlish mode
- Health pack pickups for recovery
- World boundary clamp with warning flash
- Runway light placement along the route
- Floating arrows above active rings

### HUD, Feedback, And Progression

- Health bar with color-based danger states
- Live score, timer, ring progress, and collectible count
- Mini-map / radar view
- Danger overlay near tornado hazards
- Boundary warning flash
- Score and damage popups
- Persistent top scores
- Persistent last 5 winning runs leaderboard
- Dedicated menu, leaderboard, win, and loss screens

### Visual Theme Work

- Dusty model with themed menu / win / crash presentation
- Sky background with fog-based atmosphere
- Lit meshes, textured assets, and UI overlays
- Optional girlish mode that swaps Dusty's texture and turns collectibles into bows

## Gameplay Loop

1. Start from the main menu.
2. Fly through ring centers to earn major score.
3. Avoid ring frames and tornados to protect health.
4. Collect coins or bows for bonus score.
5. Grab health packs when needed.
6. Reach the finish line after clearing all rings.
7. Save the final score into the leaderboard flow.

## Controls

### Keyboard And Mouse

| Action                        | Control              |
| ----------------------------- | -------------------- |
| Start game / confirm          | `Space`              |
| Exit / return to menu         | `Esc`                |
| Turn left / bank left         | `A` or `Left Arrow`  |
| Turn right / bank right       | `D` or `Right Arrow` |
| Climb                         | `S` or `Down Arrow`  |
| Dive                          | `W`                  |
| Roll left                     | `Q`                  |
| Roll right                    | `E`                  |
| Boost                         | `Left Shift`         |
| Slow down                     | `Left Ctrl`          |
| Backflip / emote              | `Up Arrow`           |
| Toggle headlights             | `H`                  |
| Open leaderboard from menu    | `L`                  |
| Enable girlish mode from menu | `G`                  |
| Hold to steer with mouse      | `Left Mouse Button`  |

### Gamepad Support

| Action            | Control       |
| ----------------- | ------------- |
| Turn / pitch      | `Left Stick`  |
| Look / rotate     | `Right Stick` |
| Boost             | `RT / R2`     |
| Slow down         | `LT / L2`     |
| Roll right        | `RB / R1`     |
| Roll left         | `LB / L1`     |
| Start / confirm   | `A`           |
| Back / exit       | `B`           |
| Toggle headlights | `Y`           |
| Backflip / emote  | `D-Pad Up`    |

The input layer also includes a fallback path for generic joysticks when a controller is connected but not exposed through GLFW's standard gamepad mapping.

## Cassette Player

In-game cassette switching is mapped to `0` through `4`.

| Key | Track             |
| --- | ----------------- |
| `0` | Engine loop       |
| `1` | `انت ازاي تجرحني` |
| `2` | `طبيب جراح`       |
| `3` | `زمردة`           |
| `4` | `اللي قادرة`      |

Notes:

- Cassette tracks are stored under `assets/sounds/cassette/`.
- Tracks `3` and `4` also trigger a pink-tinted visual overlay.
- This section is a good place to add a future cassette screenshot.

## Screenshots

Add your gameplay screenshots here later.

```md
![Main Race](path/to/your-screenshot-1.png)
![Cassette UI](path/to/your-screenshot-2.png)
![Leaderboard](path/to/your-screenshot-3.png)
```

Suggested shots to add:

- Main gameplay view while flying through a ring
- Tornado danger moment
- Mini-map and HUD close-up
- Cassette song banner on screen
- Girlish mode variant
- Win screen and leaderboard

## Build And Run

### Requirements

- CMake `3.15+`
- A C++17-capable compiler
- OpenGL `3.3`

### Linux Packages

On Ubuntu or Debian-based systems, install:

```bash
sudo apt update
sudo apt install -y libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev libxcursor-dev libxinerama-dev
```

### Build

```bash
cmake -S . -B build
cmake --build build
```

### Run

```bash
./bin/GAME_APPLICATION
```

On Windows:

```bash
bin/GAME_APPLICATION.exe
```

### Optional Run Arguments

```bash
./bin/GAME_APPLICATION -c config/app.jsonc
./bin/GAME_APPLICATION -f 300
```

- `-c` chooses the config file.
- `-f` runs for a fixed number of frames, which is useful for testing.

## Project Notes

- Scores are saved in `assets/data/scores.json`.
- The main runtime starts from the `menu` scene.
- The game uses local assets for meshes, textures, shaders, fonts, and audio.
- The executable output path is `bin/`.

## Contributors
