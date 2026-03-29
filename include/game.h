#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <dlfcn.h>
#include "actions.h"

#define WINDOW_WIDTH    900
#define WINDOW_HEIGHT   960
#define CELL_SIZE       8
#define GRID_OFFSET_X   50
#define GRID_OFFSET_Y   80
#define SCOREBOARD_H    130

typedef char (*get_action_fn)(void);

typedef struct {
    unsigned char r, g, b;
} Color3;

typedef struct {
    int         x;
    int         y;
    int         credits;
    int         cells_owned;
    int         active;
    Color3      color;
    void        *lib_handle;
    get_action_fn get_action;
    char        name[32];
} Player;

typedef struct {
    int         owner;
} Cell;

typedef struct {
    Player      players[MAX_PLAYERS];
    Cell        grid[GRID_SIZE][GRID_SIZE];
    int         num_players;
    int         game_over;
    int         tick;
    SDL_Window  *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    unsigned int *pixels;
} GameState;

int     game_init(GameState *gs, int argc, char **argv);
void    game_update(GameState *gs);
void    game_render(GameState *gs);
void    game_cleanup(GameState *gs);
void    game_show_results(GameState *gs);
void    player_move(GameState *gs, int player_idx, char action);
int     action_cost(char action);
int     game_poll_events(GameState *gs);

#endif
