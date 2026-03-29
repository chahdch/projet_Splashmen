# projet_Splashmen
Un jeu multijoueur (robot), 4 joueurs max. Chaque joueur est un programme dont l'objectif est de remplir des cases mémoires. Le programme/joueur qui aura rempli le plus de cases avec ses crédits gagne la partie.




mak# Splashmem

A multiplayer memory-filling game in C with SDL2.

## Build

```bash
make
```

Requires: `gcc`, `libsdl2-dev`

```bash
sudo apt install libsdl2-dev
```

## Run

```bash
./splash players/player1.so players/player2.so players/player3.so players/player4.so
```

You can pass 1 to 4 player libraries:
```bash
./splash players/player1.so players/player2.so
```

## Controls

| Key | Action |
|-----|--------|
| `ESC` | Quit |
| `+` | Speed up |
| `-` | Slow down |


Compile as shared library:
```bash
make run
```

## Actions

| Code | Cost | Description |
|------|------|-------------|
| `ACTION_MOVE_L/R/U/D` | 1 | Move 1 cell |
| `ACTION_DASH_L/R/U/D` | 10 | Move 8 cells, marks all cells along the path |
| `ACTION_TELEPORT_L/R/U/D` | 2 | Jump 8 cells, marks only destination |
| `ACTION_STILL` | 1 | Stay in place |

- Each player starts with **9000 credits**
- Grid: **100×100** cells
- Wrapping edges (Pac-Man style)
- Most cells owned at game end wins

## Grid

- `(0,0)` = top-left
- `(99,99)` = bottom-right
