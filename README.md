# Splashmem v1.1

A multiplayer memory-filling game in C with SDL2.

## Overview

A competitive 4-player game where each player is a program competing to fill memory cells. Each player controls an AI that performs actions to mark cells with their color. The player who owns the most cells at game end wins.

## Build

```bash
make
```

Requires: `gcc`, `libsdl2-dev`

**macOS:**
```bash
brew install sdl2
```

**Linux:**
```bash
sudo apt install libsdl2-dev
```

## Run

```bash
make run
```

Or manually:
```bash
./splash players/player1.so players/player2.so players/player3.so players/player4.so
```

You can pass 1 to 4 player libraries (shared objects `.so` or text scripts `.txt`):
```bash
./splash players/player1.so players/player2.so
```

## Controls

| Key | Action |
|-----|--------|
| `SPACE` | Pause/Resume game |
| `R` | Restart game |
| `UP` | Increase speed (1-10x) |
| `DOWN` | Decrease speed (1-10x) |
| `ESC` | Quit |

## Player Libraries

Players can be implemented as:
- **Shared object (.so)**: Compiled C functions returning action codes
- **Text script (.txt)**: Comma-separated action names (e.g., `MOVE_R,DASH_U,BOMB`)

## Actions

| Code | Cost | Duration | Description |
|------|------|----------|-------------|
| `MOVE_L/R/U/D` | 1 | instant | Move 1 cell, paint trail |
| `DASH_L/R/U/D` | 10 | instant | Move 8 cells, paint all cells along path |
| `TELEPORT_L/R/U/D` | 2 | instant | Jump 8 cells, paint only destination |
| `STILL` | 1 | instant | Stay in place |
| `BOMB` | 9 | 5 turns | Plant bomb, explodes in 3×3 area after 5-turn fuse |
| `FORK` | 20 | 20 turns | Create clone at spawn point, costs doubled during fork |
| `CLEAN` | 40 | instant | Clear 7×7 area (remove opponent cells only) |
| `MUTE` | 30 | 10 turns | Opponent actions are forced to `STILL` |
| `SWAP` | 35 | 5 turns | Swap positions with random opponent |

### Paint Trails

All movement actions (MOVE, DASH, TELEPORT) paint cells with the player's color. Paint layers build up over time.

### Effect Stacking

- **Same effect type extends**: Applying MUTE when already MUTED extends the timer
- **Different effect replaces**: Applying SWAP when MUTED replaces MUTE with SWAP

## Game Rules

- Each player starts with **9000 credits**
- Grid: **50×50** cells
- Wrapping edges (Pac-Man style wrapping)
- Most cells owned at game end wins
- Highest percentage win tiebreaker

## Grid Coordinates

- `(0,0)` = top-left
- `(49,49)` = bottom-right
- Edges wrap around (moving left from x=0 goes to x=49)
