#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <dlfcn.h>
#include "actions.h"

#define WINDOW_WIDTH    900
#define WINDOW_HEIGHT   990
#define CELL_SIZE       8
#define GRID_OFFSET_X   50
#define GRID_OFFSET_Y   80
#define SCOREBOARD_H    160

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
    char        *action_seq;
    int         action_seq_len;
    int         action_seq_pos;
    int         effect_type;
    int         effect_timer;
    int         effect_source;
    int         fork_timer;
    char        current_action;
} Player;

typedef struct {
    int         owner;
} Cell;

typedef struct {
    int         active;
    int         owner;
    int         x;
    int         y;
    int         timer;
    int         spawn_timer;
    int         spawn_x;
    int         spawn_y;
} Clone;

typedef struct {
    int         active;
    int         owner;
    int         x;
    int         y;
    int         timer;
} Bomb;

typedef struct {
    Player      players[MAX_PLAYERS];
    Cell        grid[GRID_SIZE][GRID_SIZE];
    Clone       clones[MAX_PLAYERS];
    Bomb        bombs[MAX_BOMBS];
    int         bomb_count;
    int         num_players;
    int         game_over;
    int         tick;
    int         paused;
    int         speed_factor;
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
int     should_restart(GameState *gs);

#endif
