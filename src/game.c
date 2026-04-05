#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <dlfcn.h>
#include <limits.h>
#include "game.h"
#include "actions.h"

typedef enum {
    EFFECT_NONE = 0,
    EFFECT_MUTE,
    EFFECT_SWAP
} EffectType;

static int wrap(int val, int max)
    {
    while (val < 0) val += max;
    while (val >= max) val -= max;
    return val;
}

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
    {12, 12}, {37, 12}, {12, 37}, {37, 37}
};

static int str_ends_with(const char *s, const char *suffix)
{
    size_t sl = strlen(s);
    size_t su = strlen(suffix);
    return sl >= su && strcmp(s + sl - su, suffix) == 0;
}

static char *trim_token(char *s)
{
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '\0') return s;
    char *end = s + strlen(s) - 1;
    while (end >= s && isspace((unsigned char)*end)) *end-- = '\0';
    return s;
}

static int action_name_to_code(const char *name)
{
    char token[64];
    strncpy(token, name, sizeof(token) - 1);
    token[sizeof(token) - 1] = '\0';
    char *s = token;
    if (strncmp(s, "ACTION_", 7) == 0)
        s += 7;

    if (strcasecmp(s, "STILL") == 0) return ACTION_STILL;
    if (strcasecmp(s, "MOVE_L") == 0) return ACTION_MOVE_L;
    if (strcasecmp(s, "MOVE_R") == 0) return ACTION_MOVE_R;
    if (strcasecmp(s, "MOVE_U") == 0) return ACTION_MOVE_U;
    if (strcasecmp(s, "MOVE_D") == 0) return ACTION_MOVE_D;
    if (strcasecmp(s, "DASH_L") == 0) return ACTION_DASH_L;
    if (strcasecmp(s, "DASH_R") == 0) return ACTION_DASH_R;
    if (strcasecmp(s, "DASH_U") == 0) return ACTION_DASH_U;
    if (strcasecmp(s, "DASH_D") == 0) return ACTION_DASH_D;
    if (strcasecmp(s, "TELEPORT_L") == 0) return ACTION_TELEPORT_L;
    if (strcasecmp(s, "TELEPORT_R") == 0) return ACTION_TELEPORT_R;
    if (strcasecmp(s, "TELEPORT_U") == 0) return ACTION_TELEPORT_U;
    if (strcasecmp(s, "TELEPORT_D") == 0) return ACTION_TELEPORT_D;
    if (strcasecmp(s, "BOMB") == 0) return ACTION_BOMB;
    if (strcasecmp(s, "FORK") == 0) return ACTION_FORK;
    if (strcasecmp(s, "CLEAN") == 0) return ACTION_CLEAN;
    if (strcasecmp(s, "MUTE") == 0) return ACTION_MUTE;
    if (strcasecmp(s, "SWAP") == 0) return ACTION_SWAP;
    return -1;
}

static void free_player_actions(Player *p)
{
    if (p->action_seq) {
        free(p->action_seq);
        p->action_seq = NULL;
    }
    p->action_seq_len = 0;
    p->action_seq_pos = 0;
}

static int load_player_script(Player *p, const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char buffer[8192];
    size_t read_bytes = fread(buffer, 1, sizeof(buffer) - 1, f);
    fclose(f);
    if (read_bytes == 0) return -1;
    buffer[read_bytes] = '\0';

    char *tokens[256];
    int count = 0;
    char *saveptr = NULL;
    char *token = strtok_r(buffer, ",", &saveptr);
    while (token && count < (int)(sizeof(tokens) / sizeof(tokens[0]))) {
        tokens[count++] = token;
        token = strtok_r(NULL, ",", &saveptr);
    }

    if (count == 0) return -1;

    char *seq = malloc(count);
    if (!seq) return -1;

    int seq_len = 0;
    for (int i = 0; i < count; i++) {
        char *name = trim_token(tokens[i]);
        int code = action_name_to_code(name);
        if (code < 0) {
            free(seq);
            return -1;
        }
        seq[seq_len++] = (char)code;
    }

    if (seq_len == 0) {
        free(seq);
        return -1;
    }

    p->action_seq = seq;
    p->action_seq_len = seq_len;
    p->action_seq_pos = 0;
    p->get_action = NULL;
    return 0;
}

static char script_get_action(Player *p)
{
    if (p->action_seq_len == 0) return ACTION_STILL;
    char action = p->action_seq[p->action_seq_pos];
    p->action_seq_pos = (p->action_seq_pos + 1) % p->action_seq_len;
    return action;
}

static int toroidal_distance(int a, int b)
{
    int d = abs(a - b);
    return d > GRID_SIZE / 2 ? GRID_SIZE - d : d;
}

static int find_nearest_enemy(GameState *gs, int source_idx)
{
    int best = -1;
    int best_dist = INT_MAX;
    Player *source = &gs->players[source_idx];

    for (int i = 0; i < gs->num_players; i++) {
        if (i == source_idx) continue;
        Player *p = &gs->players[i];
        if (!p->active) continue;

        int dx = toroidal_distance(source->x, p->x);
        int dy = toroidal_distance(source->y, p->y);
        int dist = dx + dy;
        if (dist < best_dist) {
            best_dist = dist;
            best = i;
        }
    }
    return best;
}

static void apply_effect(Player *target, int source_idx, int effect, int duration)
{
    if (target->effect_type == effect) {
        target->effect_timer += duration;
        target->effect_source = source_idx;
    } else {
        target->effect_type = effect;
        target->effect_timer = duration;
        target->effect_source = source_idx;
    }
}

static int effective_owner(GameState *gs, int owner, int is_bomb)
{
    if (owner < 0 || is_bomb) return owner;
    Player *p = &gs->players[owner];
    if (p->effect_type == EFFECT_MUTE) return -1;
    if (p->effect_type == EFFECT_SWAP) return p->effect_source;
    return owner;
}

static void mark_cell(GameState *gs, int x, int y, int owner, int override, int is_bomb)
{
    int current = gs->grid[y][x].owner;
    int final_owner = effective_owner(gs, owner, is_bomb);

    if (final_owner == current) return;
    if (!override && current != -1) return;

    if (current != -1 && current >= 0)
        gs->players[current].cells_owned--;

    gs->grid[y][x].owner = final_owner;

    if (final_owner != -1)
        gs->players[final_owner].cells_owned++;
}

static void player_move_clone(GameState *gs, Clone *clone, char action)
{
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
        default:
            return;
    }

    int owner = clone->owner;
    int is_dash = (ua >= ACTION_DASH_L && ua <= ACTION_DASH_D);
    if (is_dash) {
        for (int s = 1; s <= steps; s++) {
            int nx = wrap(clone->x + dx * s, GRID_SIZE);
            int ny = wrap(clone->y + dy * s, GRID_SIZE);
            mark_cell(gs, nx, ny, owner, 0, 0);
        }
    }

    clone->x = wrap(clone->x + dx * steps, GRID_SIZE);
    clone->y = wrap(clone->y + dy * steps, GRID_SIZE);
    mark_cell(gs, clone->x, clone->y, owner, 0, 0);
}

int action_cost(char action)
{
    switch ((unsigned char)action) {
        case ACTION_MOVE_L: case ACTION_MOVE_R:
        case ACTION_MOVE_U: case ACTION_MOVE_D: return COST_MOVE;
        case ACTION_DASH_L: case ACTION_DASH_R:
        case ACTION_DASH_U: case ACTION_DASH_D: return COST_DASH;
        case ACTION_TELEPORT_L: case ACTION_TELEPORT_R:
        case ACTION_TELEPORT_U: case ACTION_TELEPORT_D: return COST_TELEPORT;
        case ACTION_BOMB: return COST_BOMB;
        case ACTION_FORK: return COST_FORK;
        case ACTION_CLEAN: return COST_CLEAN;
        case ACTION_MUTE: return COST_MUTE;
        case ACTION_SWAP: return COST_SWAP;
        case ACTION_STILL:
        default: return COST_STILL;
    }
}

static int load_player_file(Player *p, const char *path)
{
    if (str_ends_with(path, ".so") || str_ends_with(path, ".dylib"))
        return -1;
    return load_player_script(p, path);
}

static int spawn_clone(GameState *gs, int idx)
{
    Clone *clone = &gs->clones[idx];
    clone->owner = idx;
    clone->spawn_timer = FORK_SPAWN_DELAY;
    if (clone->active) {
        clone->timer = FORK_DURATION;
    } else {
        clone->active = 0;
        clone->timer = 0;
        clone->spawn_x = gs->players[idx].x;
        clone->spawn_y = gs->players[idx].y;
    }
    return 0;
}

static void explode_bomb(GameState *gs, int bomb_index)
{
    Bomb *b = &gs->bombs[bomb_index];
    for (int dy = -BOMB_AREA_HALF; dy <= BOMB_AREA_HALF; dy++) {
        for (int dx = -BOMB_AREA_HALF; dx <= BOMB_AREA_HALF; dx++) {
            int nx = wrap(b->x + dx, GRID_SIZE);
            int ny = wrap(b->y + dy, GRID_SIZE);
            mark_cell(gs, nx, ny, b->owner, 1, 1);
        }
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
        case ACTION_BOMB:
            if (gs->bomb_count < MAX_BOMBS) {
                Bomb *b = &gs->bombs[gs->bomb_count++];
                b->active = 1;
                b->owner = idx;
                b->x = p->x;
                b->y = p->y;
                b->timer = BOMB_TIMER;
            }
            return;
        case ACTION_CLEAN:
            for (int ry = -(CLEAN_SIZE / 2); ry <= CLEAN_SIZE / 2; ry++) {
                for (int rx = -(CLEAN_SIZE / 2); rx <= CLEAN_SIZE / 2; rx++) {
                    int nx = wrap(p->x + rx, GRID_SIZE);
                    int ny = wrap(p->y + ry, GRID_SIZE);
                    mark_cell(gs, nx, ny, -1, 1, 0);
                }
            }
            return;
        case ACTION_MUTE: {
            int target = find_nearest_enemy(gs, idx);
            if (target >= 0)
                apply_effect(&gs->players[target], idx, EFFECT_MUTE, MUTE_DURATION);
            return;
        }
        case ACTION_SWAP: {
            int target = find_nearest_enemy(gs, idx);
            if (target >= 0)
                apply_effect(&gs->players[target], idx, EFFECT_SWAP, SWAP_DURATION);
            return;
        }
        case ACTION_FORK:
            p->fork_timer += FORK_DURATION;
            spawn_clone(gs, idx);
            return;
        case ACTION_STILL:
        default:
            return;
    }

    if (steps > 1) {
        for (int s = 1; s < steps; s++) {
            int nx = wrap(p->x + dx * s, GRID_SIZE);
            int ny = wrap(p->y + dy * s, GRID_SIZE);
            mark_cell(gs, nx, ny, idx, 1, 0);
        }
    }

    p->x = wrap(p->x + dx * steps, GRID_SIZE);
    p->y = wrap(p->y + dy * steps, GRID_SIZE);
    mark_cell(gs, p->x, p->y, idx, 0, 0);
}

int game_init(GameState *gs, int argc, char **argv)
{
    memset(gs, 0, sizeof(GameState));
    gs->bomb_count = 0;

    for (int y = 0; y < GRID_SIZE; y++)
        for (int x = 0; x < GRID_SIZE; x++)
            gs->grid[y][x].owner = -1;

    gs->num_players = argc - 1;
    if (gs->num_players > MAX_PLAYERS) gs->num_players = MAX_PLAYERS;

    for (int i = 0; i < gs->num_players; i++) {
        Player *p = &gs->players[i];
        p->action_seq = NULL;
        p->action_seq_len = 0;
        p->action_seq_pos = 0;
        p->effect_type = EFFECT_NONE;
        p->effect_timer = 0;
        p->effect_source = -1;
        p->fork_timer = 0;
        p->current_action = ACTION_STILL;
        p->x = start_pos[i][0];
        p->y = start_pos[i][1];
        p->credits = INITIAL_CREDITS;
        p->cells_owned = 0;
        p->active = 1;
        p->color = player_colors[i];
        snprintf(p->name, sizeof(p->name), "%s", player_names[i]);

        if (load_player_file(p, argv[i + 1]) == 0) {
            /* text action script loaded */
        } else {
            p->lib_handle = dlopen(argv[i + 1], RTLD_NOW | RTLD_LOCAL);
            if (!p->lib_handle) {
                fprintf(stderr, "Cannot load %s: %s\n", argv[i+1], dlerror());
                free_player_actions(p);
                return -1;
            }
            p->get_action = (get_action_fn)dlsym(p->lib_handle, "get_action");
            if (!p->get_action) {
                fprintf(stderr, "No get_action in %s: %s\n", argv[i+1], dlerror());
                dlclose(p->lib_handle);
                free_player_actions(p);
                return -1;
            }
        }

        mark_cell(gs, p->x, p->y, i, 0, 0);
        gs->clones[i].active = 0;
        gs->clones[i].spawn_timer = 0;
        gs->clones[i].timer = 0;
        gs->clones[i].spawn_x = 0;
        gs->clones[i].spawn_y = 0;
    }

    /* Initialize control state */
    gs->paused = 0;
    gs->speed_factor = 1;

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
        free(gs->pixels);
        return -1;
    }

    return 0;
}

void game_update(GameState *gs)
{
    for (int i = 0; i < gs->num_players; i++) {
        Player *p = &gs->players[i];
        if (!p->active) continue;

        char action = p->action_seq_len ? script_get_action(p) : p->get_action();
        p->current_action = action;
        int cost = action_cost(action);
        if (p->fork_timer > 0) cost *= 2;
        if (cost > p->credits) {
            p->active = 0;
            continue;
        }
        p->credits -= cost;
        player_move(gs, i, action);

        Clone *clone = &gs->clones[i];
        if (clone->active) {
            player_move_clone(gs, clone, action);
            clone->timer--;
            if (clone->timer <= 0)
                clone->active = 0;
        }

        if (p->credits <= 0)
            p->active = 0;
    }

    for (int i = 0; i < gs->bomb_count; i++) {
        gs->bombs[i].timer--;
        if (gs->bombs[i].timer <= 0) {
            explode_bomb(gs, i);
            gs->bombs[i] = gs->bombs[gs->bomb_count - 1];
            gs->bomb_count--;
            i--;
        }
    }

    for (int i = 0; i < gs->num_players; i++) {
        Clone *clone = &gs->clones[i];
        if (clone->spawn_timer > 0) {
            clone->spawn_timer--;
            if (clone->spawn_timer == 0) {
                clone->active = 1;
                clone->timer = FORK_DURATION;
                clone->x = clone->spawn_x;
                clone->y = clone->spawn_y;
            }
        }
    }

    int all_done = 1;
    for (int i = 0; i < gs->num_players; i++) {
        Player *p = &gs->players[i];
        if (p->effect_timer > 0) {
            p->effect_timer--;
            if (p->effect_timer == 0)
                p->effect_type = EFFECT_NONE;
        }
        if (p->fork_timer > 0)
            p->fork_timer--;
        if (p->active)
            all_done = 0;
    }

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
                if (event.key.keysym.sym == SDLK_SPACE) {
                    gs->paused = !gs->paused;
                }
                if (event.key.keysym.sym == SDLK_r) {
                    return 2;
                }
                if (event.key.keysym.sym == SDLK_DOWN) {
                    if (gs->speed_factor > 1) gs->speed_factor--;
                }
                if (event.key.keysym.sym == SDLK_UP) {
                    if (gs->speed_factor < 10) gs->speed_factor++;
                }
                break;
        }
    }
    return 1;
}

int should_restart(GameState *gs)
{
    return gs->game_over;
}

void game_cleanup(GameState *gs)
{
    for (int i = 0; i < gs->num_players; i++) {
        if (gs->players[i].lib_handle)
            dlclose(gs->players[i].lib_handle);
        free_player_actions(&gs->players[i]);
    }

    if (gs->texture) SDL_DestroyTexture(gs->texture);
    if (gs->renderer) SDL_DestroyRenderer(gs->renderer);
    if (gs->window) SDL_DestroyWindow(gs->window);
    if (gs->pixels) free(gs->pixels);
    SDL_Quit();
}
