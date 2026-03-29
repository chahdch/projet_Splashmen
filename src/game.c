#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "game.h"
#include "actions.h"

static Color3 player_colors[MAX_PLAYERS] = {
    {220,  60,  60},
    { 60, 120, 220},
    { 60, 200,  80},
    {220, 200,  40}
};

static const char *player_names[MAX_PLAYERS] = {
    "Player 1", "Player 2", "Player 3", "Player 4"
};

static int start_pos[MAX_PLAYERS][2] = {
    {25, 25}, {74, 25}, {25, 74}, {74, 74}
};

int action_cost(char action)
{
    switch ((unsigned char)action) {
        case ACTION_MOVE_L: case ACTION_MOVE_R:
        case ACTION_MOVE_U: case ACTION_MOVE_D: return COST_MOVE;
        case ACTION_DASH_L: case ACTION_DASH_R:
        case ACTION_DASH_U: case ACTION_DASH_D: return COST_DASH;
        case ACTION_TELEPORT_L: case ACTION_TELEPORT_R:
        case ACTION_TELEPORT_U: case ACTION_TELEPORT_D: return COST_TELEPORT;
        default: return COST_STILL;
    }
}

static int wrap(int val, int max)
{
    if (val < 0)    return val + max;
    if (val >= max) return val - max;
    return val;
}

static void mark_cell(GameState *gs, int x, int y, int idx)
{
    if (gs->grid[y][x].owner == -1) {
        gs->grid[y][x].owner = idx;
        gs->players[idx].cells_owned++;
    }
}

void player_move(GameState *gs, int idx, char action)
{
    Player *p = &gs->players[idx];
    int dx = 0, dy = 0, steps = 1;
    unsigned char ua = (unsigned char)action;

    switch (ua) {
        case ACTION_MOVE_L:     dx=-1; dy= 0; steps=1; break;
        case ACTION_MOVE_R:     dx= 1; dy= 0; steps=1; break;
        case ACTION_MOVE_U:     dx= 0; dy=-1; steps=1; break;
        case ACTION_MOVE_D:     dx= 0; dy= 1; steps=1; break;
        case ACTION_DASH_L:     dx=-1; dy= 0; steps=DASH_DISTANCE; break;
        case ACTION_DASH_R:     dx= 1; dy= 0; steps=DASH_DISTANCE; break;
        case ACTION_DASH_U:     dx= 0; dy=-1; steps=DASH_DISTANCE; break;
        case ACTION_DASH_D:     dx= 0; dy= 1; steps=DASH_DISTANCE; break;
        case ACTION_TELEPORT_L: dx=-1; dy= 0; steps=TELEPORT_DISTANCE; break;
        case ACTION_TELEPORT_R: dx= 1; dy= 0; steps=TELEPORT_DISTANCE; break;
        case ACTION_TELEPORT_U: dx= 0; dy=-1; steps=TELEPORT_DISTANCE; break;
        case ACTION_TELEPORT_D: dx= 0; dy= 1; steps=TELEPORT_DISTANCE; break;
        case ACTION_STILL: default: return;
    }

    int is_dash = (ua >= ACTION_DASH_L && ua <= ACTION_DASH_D);
    if (is_dash) {
        for (int s = 1; s <= steps; s++) {
            int nx = wrap(p->x + dx * s, GRID_SIZE);
            int ny = wrap(p->y + dy * s, GRID_SIZE);
            mark_cell(gs, nx, ny, idx);
        }
    }
    p->x = wrap(p->x + dx * steps, GRID_SIZE);
    p->y = wrap(p->y + dy * steps, GRID_SIZE);
    mark_cell(gs, p->x, p->y, idx);
}

int game_init(GameState *gs, int argc, char **argv)
{
    memset(gs, 0, sizeof(GameState));

    for (int y = 0; y < GRID_SIZE; y++)
        for (int x = 0; x < GRID_SIZE; x++)
            gs->grid[y][x].owner = -1;

    gs->num_players = argc - 1;
    if (gs->num_players > MAX_PLAYERS) gs->num_players = MAX_PLAYERS;

    for (int i = 0; i < gs->num_players; i++) {
        Player *p = &gs->players[i];
        p->lib_handle = dlopen(argv[i + 1], RTLD_NOW | RTLD_LOCAL);
        if (!p->lib_handle) {
            fprintf(stderr, "Cannot load %s: %s\n", argv[i+1], dlerror());
            return -1;
        }
        p->get_action = (get_action_fn)dlsym(p->lib_handle, "get_action");
        if (!p->get_action) {
            fprintf(stderr, "No get_action in %s: %s\n", argv[i+1], dlerror());
            return -1;
        }
        p->x = start_pos[i][0];
        p->y = start_pos[i][1];
        p->credits = INITIAL_CREDITS;
        p->cells_owned = 0;
        p->active = 1;
        p->color = player_colors[i];
        snprintf(p->name, sizeof(p->name), "%s", player_names[i]);
        mark_cell(gs, p->x, p->y, i);
    }

    /* SDL2 init */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    gs->window = SDL_CreateWindow("Splashmem",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN);
    
    if (!gs->window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    gs->renderer = SDL_CreateRenderer(gs->window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    if (!gs->renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(gs->window);
        SDL_Quit();
        return -1;
    }

    /* Pixel buffer */
    gs->pixels = (unsigned int *)calloc(WINDOW_WIDTH * WINDOW_HEIGHT, sizeof(unsigned int));
    if (!gs->pixels) return -1;

    gs->texture = SDL_CreateTexture(gs->renderer,
        SDL_PIXELFORMAT_RGB888,
        SDL_TEXTUREACCESS_STREAMING,
        WINDOW_WIDTH, WINDOW_HEIGHT);

    if (!gs->texture) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        return -1;
    }

    return 0;
}

void game_update(GameState *gs)
{
    for (int i = 0; i < gs->num_players; i++) {
        Player *p = &gs->players[i];
        if (!p->active) continue;

        char action = p->get_action();
        int cost = action_cost(action);
        if (cost > p->credits) cost = p->credits;
        p->credits -= cost;
        player_move(gs, i, action);
        if (p->credits <= 0) p->active = 0;
    }

    int all_done = 1;
    for (int i = 0; i < gs->num_players; i++)
        if (gs->players[i].active) { all_done = 0; break; }

    if (all_done) {
        gs->game_over = 1;
        game_show_results(gs);
    }
    gs->tick++;
}

void game_show_results(GameState *gs)
{
    int winner = 0;
    printf("\n========== SPLASHMEM RESULTS ==========\n");
    for (int i = 0; i < gs->num_players; i++) {
        Player *p = &gs->players[i];
        printf("  %s: %d cells\n", p->name, p->cells_owned);
        if (p->cells_owned > gs->players[winner].cells_owned) winner = i;
    }
    printf("\n  WINNER: %s with %d cells!\n",
        gs->players[winner].name, gs->players[winner].cells_owned);
    printf("========================================\n");
}

int game_poll_events(GameState *gs)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                return 0;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE ||
                    event.key.keysym.sym == SDLK_q) {
                    return 0;
                }
                break;
        }
    }
    return 1;
}

void game_cleanup(GameState *gs)
{
    for (int i = 0; i < gs->num_players; i++)
        if (gs->players[i].lib_handle) dlclose(gs->players[i].lib_handle);

    if (gs->texture) SDL_DestroyTexture(gs->texture);
    if (gs->renderer) SDL_DestroyRenderer(gs->renderer);
    if (gs->window) SDL_DestroyWindow(gs->window);
    if (gs->pixels) free(gs->pixels);
    SDL_Quit();
}
