//
//  game.cc
//  NetTest2
//
//  Created by Thomas Foster on 3/18/25.
//

// TODO
// - Reduce frequency of red, rainbow rings, and yellow rings

// BUGS
// - When a player bumps into another player by wrapping around.

#include "game.hh"

#include "beeper.hh"
#include "buffer.hh"
#include "net.hh"
#include "packet.hh"
#include "random.hh"

#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL.h>

#define HUD_LINE_HEIGHT (CHAR_HEIGHT + 2)
#define HUD_LINE(n) (HUD_LINE_HEIGHT * ((n) - 1))

// -----------------------------------------------------------------------------
// Constants

static const char _tile_map[MAP_SIZE][MAP_SIZE + 1 /* = null term. */] = {
    "XVXXXXX...XXVVX...XXXXXXX",
    "XsSSSSS...SXVsS...SSSSSSX",
    "V..........SsX..........X",
    "X..A.........S.......C..X",
    "X.aSc....XGW........gSi.X",
    "X..b...XGXGWG..XVX...h..X",
    "X.....0XWSWWG..SsV2.....X",
    "S....XXXWWWG.....sXX....S",
    ".....VSSGGG.......SX.....",
    ".....XT...........TX.....",
    "X....S.......W.....S....X",
    "XX.......G.GWW.........XX",
    "XX.......GGWWWW........XX",
    "XS......GGGoWW.........XX",
    "X........GYGGGG........SX",
    "S....X...GGGYG.....X....S",
    ".....XT....GG.....TX.....",
    ".....XXX.........VXS.....",
    "X....SSX........GVS.....X",
    "X.....3XXX.....VXX1.....X",
    "X..D...SSS.....sSS...B..X",
    "X.jSl.......LL......dSf.X",
    "X..k.......LLLL......e..X",
    "X..........XXXLL........X",
    "XXXXXXX...XXXXX...XXXXXXX",
};

static const Color _player_colors[MAX_PLAYERS] = {
    BRIGHT_WHITE,
    BRIGHT_MAGENTA,
    BRIGHT_GREEN,
    BRIGHT_CYAN,
};

static const char * _sounds[] = {
    [S_REMOVE_FROM_SOCKET]  = "l32 c f d f+ a",
    [S_PLACE_IN_SOCKET]     = "l32 a f+ d f c",
    [S_ATTACK]              = "t160 l32 o1 c c+ a",
    [S_BUMP]                = "t160 l32 o1 c c+",
    // TODO: different sound for each ring appear
    [S_RING_SPAWN]          = "t160 l32 o5 d a",
    [S_RING_COLLECT]        = "t160 l32 o5 e a b",
    [S_DISPOSER_PLACE]      = "t160 l32 o6 c l16 d-",
    [S_RING_DISPOSED]       = "t160 o2 l32 b f+ b- f a- e-",
    [S_TELEPORT]            = "t200 o1 l32 f+ > e > e- > d > c+ > c",
    [S_REGEN]               = "t160 o2 l32 f+ a+ g+ > c",
    [S_MATCH_OVER]          = "t160 o4 l16 e c e g e g > c8"
};

static const Glyph _glyphs[] = {
    // TODO: rings?

    ['.'] = { { 0x00, 0xB0 }, VERY_DARK_GRAY, BLACK }, // Empty
    ['G'] = { { 0xB0, 0xB0 }, DARK_GREEN, BLACK }, // Grass
    ['X'] = { { 0xB0, 0xB1 }, WHITE, GRAY }, // Wall
    ['S'] = { { 0xB1, 0xB1 }, DARK_GRAY, VERY_DARK_GRAY }, // Wall side
    ['V'] = { { 0xB1, 0xB2 }, GRAY, GREEN }, // Vines
    ['s'] = { { 0xB1, 0xB1 }, DARK_GREEN, VERY_DARK_GRAY }, // Vine side
    ['T'] = { { 0x07, 0x07 }, BRIGHT_WHITE, MAGENTA }, // Teleporter
    ['W'] = { { 0x00, '~'  }, BRIGHT_CYAN, BLUE }, // Water
    ['L'] = { { 0x00, '~'  }, YELLOW, RED }, // Lava
    ['Y'] = { { 0x05, 0x06 }, GREEN, BLACK }, // Tree
    ['R'] = { { 0x09, 0x09 }, DARK_GRAY, BLACK }, // Empty Socket
    ['A'] = { { 0x07, 0x07 }, _player_colors[0], GRAY }, // Player 1 Totem
    ['B'] = { { 0x07, 0x07 }, _player_colors[1], GRAY }, // Player 2 Totem
    ['C'] = { { 0x07, 0x07 }, _player_colors[2], GRAY }, // Player 3 Totem
    ['D'] = { { 0x07, 0x07 }, _player_colors[3], GRAY }, // Player 4 Totem
    ['a'] = { { 0x09, 0x09 }, DARK_GRAY, BLACK }, // Ring Socket
    ['b'] = { { 0x09, 0x09 }, DARK_GRAY, BLACK }, // Ring Socket
    ['c'] = { { 0x09, 0x09 }, DARK_GRAY, BLACK }, // Ring Socket
    ['d'] = { { 0x09, 0x09 }, DARK_GRAY, BLACK }, // Ring Socket
    ['e'] = { { 0x09, 0x09 }, DARK_GRAY, BLACK }, // Ring Socket
    ['f'] = { { 0x09, 0x09 }, DARK_GRAY, BLACK }, // Ring Socket
    ['g'] = { { 0x09, 0x09 }, DARK_GRAY, BLACK }, // Ring Socket
    ['h'] = { { 0x09, 0x09 }, DARK_GRAY, BLACK }, // Ring Socket
    ['i'] = { { 0x09, 0x09 }, DARK_GRAY, BLACK }, // Ring Socket
    ['j'] = { { 0x09, 0x09 }, DARK_GRAY, BLACK }, // Ring Socket
    ['k'] = { { 0x09, 0x09 }, DARK_GRAY, BLACK }, // Ring Socket
    ['l'] = { { 0x09, 0x09 }, DARK_GRAY, BLACK }, // Ring Socket
    ['0'] = { { 0xFE, 0xFE }, _player_colors[0], DARK_GRAY }, // Player Start
    ['1'] = { { 0xFE, 0xFE }, _player_colors[1], DARK_GRAY }, // Player Start
    ['2'] = { { 0xFE, 0xFE }, _player_colors[2], DARK_GRAY }, // Player Start
    ['3'] = { { 0xFE, 0xFE }, _player_colors[3], DARK_GRAY }, // Player Start
    ['o'] = { { 0x0A, 0x0A }, BRIGHT_RED, RED }, // Ring Disposer
};

// -----------------------------------------------------------------------------
// Prototypes

static void UpdateGame(Action action, float dt);
static void RenderGame(void);
static bool DoGameInput(void);
static void RenderMatchOver(void);
void TryMovePlayer(Player * player, int x, int y);
void DisposeRing(void * data);
void UpdatePoints(void * data);
void SpawnRing(void * data);

// -----------------------------------------------------------------------------
// Public Data

bool is_running_g = true;
Session session_g;
int nplayers_g = 1;

// -----------------------------------------------------------------------------
// Private Data

#define             NUM_SOCKETS_PER_PLAYER 3
#define             NUM_SOCKETS ((NUM_SOCKETS_PER_PLAYER) * (MAX_PLAYERS))
#define             MAX_RINGS 4 // Number of rings that can appear at once
#define             MAX_POINTS 100
static GameState    _curr_state;
static Player       _players[MAX_PLAYERS];
static int          _player_idx; // Which player[] we are.
static Action       _curr_action; // Current player action from input.
static u8           _sockets[NUM_SOCKETS]; // Corresponds to tiles 'a' to 'l'
static Ring         _rings[MAX_RINGS]; // Rings on the board
static u8           _nrings; // Number of rings in the rings array
static u8           _disposal; // Type of ring inside.
static enum Sound   _curr_sound; // Which sound to output at end of frame.

static Timer        _ring_timer = InitTimer(3.0f, 15.0f, SpawnRing);
static Timer        _dispose_timer = InitTimer(0, 0, DisposeRing);
static Timer        _point_timer = InitTimer(5.0f, 5.0f, UpdatePoints);
static Timer        _key_timer = InitTimer(0.0f, 0.0f, NULL);

static const GameStateHandler _state_handlers[] = {
    [GS_PLAY] = {
        .do_input = DoGameInput,
        .update = UpdateGame,
        .render = RenderGame,
    },
    [GS_MATCH_OVER] = {
        .do_input = NULL,
        .update = NULL,
        .render = RenderMatchOver
    }
};

// Net
static Socket _connections[MAX_PLAYERS]; // Server connections.
static Socket _client;
static Buffer _net_buf;

// -----------------------------------------------------------------------------
#pragma mark - Misc Functions

static void QuitGame(void)
{
    if ( _client.is_init ) {
        CloseSocket(&_client);
    }

    for ( int i = 0; i < nplayers_g; i++ ) {
        if ( _connections[i].is_init ) {
            CloseSocket(&_connections[i]);
        }
    }
}

constexpr SDL_Rect GetMapRect(void)
{
    constexpr int map_pix_size = MAP_SIZE * TILE_SIZE;
    constexpr SDL_Rect map_rect = {
        .x = (GAME_WIDTH - map_pix_size) / 2,
        .y = (GAME_HEIGHT - map_pix_size) / 2,
        .w = map_pix_size,
        .h = map_pix_size,
    };

    return map_rect;
}

void GetHUDRects(SDL_Rect rects[])
{
    const SDL_Rect map_rect = GetMapRect();

    const int hud_lines = 3;
    const int hud_w = CHAR_WIDTH * MAX_PLAYER_HEALTH;
    const int hud_h = hud_lines * HUD_LINE_HEIGHT;
    const int hud_x1 = map_rect.x - (hud_w + CHAR_WIDTH);
    const int hud_x2 = map_rect.x + map_rect.w + CHAR_WIDTH;
    const int hud_y1 = map_rect.y + CHAR_HEIGHT;
    const int hud_y2 = map_rect.y + map_rect.h - (hud_h + CHAR_HEIGHT);

    rects[0] = { hud_x1, hud_y1, hud_w, hud_h };
    rects[1] = { hud_x2, hud_y2, hud_w, hud_h };
    rects[2] = { hud_x2, hud_y1, hud_w, hud_h };
    rects[3] = { hud_x1, hud_y2, hud_w, hud_h };
}

static bool DoGameInput(void)
{
    _curr_action = A_NONE;

    if ( _key_timer.sec == 0.0f ) {
        const bool * keys = SDL_GetKeyboardState(NULL);

        if ( keys[SDL_SCANCODE_A] ) {
            _curr_action |= A_MOVE_LEFT;
        }

        if ( keys[SDL_SCANCODE_D] ) {
            _curr_action |= A_MOVE_RIGHT;
        }

        if ( keys[SDL_SCANCODE_W] ) {
            _curr_action |= A_MOVE_UP;
        }

        if ( keys[SDL_SCANCODE_S] ) {
            _curr_action |= A_MOVE_DOWN;
        }

        if ( _curr_action != A_NONE ) {
            if ( _players[_player_idx].held == RING_RAINBOW ) {
                _key_timer.sec = 0.0f;
            } else {
                _key_timer.sec = 0.25f;
            }
        }
    }

    return _curr_action != A_NONE;
}

static Color RingColor(u8 ring_type)
{
    switch ( ring_type ) {
        case RING_NONE: return GRAY;

        case RING_BLUE: return BRIGHT_BLUE;
        case RING_GREEN: return BRIGHT_GREEN;
        case RING_CYAN: return BRIGHT_CYAN;
        case RING_RED: return BRIGHT_RED;
        case RING_MAGENTA: return BRIGHT_MAGENTA;
        case RING_YELLOW: return YELLOW;
        case RING_WHITE: return BRIGHT_WHITE;
        case RING_RAINBOW: return (Color)Rand(BRIGHT_BLUE, BRIGHT_WHITE);

        case NUM_RING_TYPES:
        default:
            ERROR("RingColor: Bad ring type (%d)\n", ring_type);
            break;
    }
}

static int RingValue(u8 ring_type, int player_index)
{
    if ( ring_type == RING_RAINBOW ) {
        return 3;
    } else if ( RingColor(ring_type) == _player_colors[player_index] ) {
        return 2;
    } else if ( ring_type == RING_NONE ) {
        return 0;
    } else {
        return 1;
    }
}

static bool Unoccupied(int x, int y)
{
    for ( int i = 0; i < nplayers_g; i++ ) {
        Player * p = &_players[i];
        if ( p->x == x && p->y == y ) {
            return false;
        }
    }

    for ( int i = 0; i < _nrings; i++ ) {
        Ring * r = &_rings[i];
        if ( r->x == x && r->y == y ) {
            return false;
        }
    }

    return _tile_map[y][x] == '.' || _tile_map[y][x] == 'G';
}

RingType GetRandomRingType(void)
{
    int weights[] = {
        [RING_BLUE]     = 10,
        [RING_GREEN]    = 10,
        [RING_CYAN]     = 10,
        [RING_MAGENTA]  = 10,
        [RING_YELLOW]   = 10,
        [RING_WHITE]    = 10,
        [RING_RED]      = 5,
        [RING_RAINBOW]  = 2,
    };

    int total = 0;
    for ( int i = 1; i < NUM_RING_TYPES; i++ ) {
        total += weights[i];
    }

    int value = Rand(0, total - 1);

    int weight = 0;
    for ( int i = 1; 1 < NUM_RING_TYPES; i++ ) {
        weight += weights[i];
        if ( value < weight ) {
            return (RingType)i;
        }
    }
}

Ranking GetRanking(void)
{
    Ranking ranking = { 0 };

    // Find the highest points value.
    int max_pts = _players[0].pts;

    for ( int i = 1; i < nplayers_g; i++ ) {
        if ( _players[i].pts > max_pts ) {
            max_pts = _players[i].pts;
        }
    }

    // Assign first place.
    for ( int i = 0; i < nplayers_g; i++ ) {
        if ( _players[i].pts == max_pts ) {
            ranking.is_in_first[i] = true;
            ranking.in_first_indices[ranking.num_in_first++] = i;
        }
    }

    if ( ranking.num_in_first == nplayers_g ) {
        // All players are tied, remove in first status
        ranking.num_in_first = 0;
        memset(ranking.is_in_first, 0, sizeof(ranking.is_in_first));
    }

    return ranking;
}

#pragma mark - Draw Functions

void DrawTile(char ch, int tile_x, int tile_y)
{
    int i = WangHash(tile_x, tile_y) & 1;

    Glyph tile = _glyphs[ch];
    Color fg = tile.fg;
    Color bg = tile.bg;

    // Choose socket color
    if ( ch >= 'a' && ch <= 'l' ) {
        RingType ring_type = (RingType)_sockets[ch - 'a'];
        if ( ring_type ) {
            fg = RingColor(ring_type);
        }
    }

    if ( ch == 'o' && _disposal ) {
        bg = RingColor(_disposal);
    }

    DrawChar(tile_x * TILE_SIZE, tile_y * TILE_SIZE, 219, bg);
    DrawChar(tile_x * TILE_SIZE, tile_y * TILE_SIZE, tile.ch[i], fg);
}

void DrawPlayer(int i)
{
    Player * p  = &_players[i];

    int x = (p->x * TILE_SIZE) + p->offx;
    int y = (p->y * TILE_SIZE) + p->offy;

    Color fg;
    Color bg = BLACK;
    char ch = 2;

    switch ( p->held ) {
        case RING_RAINBOW:
            fg = (Color)Rand(BRIGHT_BLUE, BRIGHT_WHITE);
            break;
        case RING_RED:
            fg = (SDL_GetTicks() / 100) & 1 ? BRIGHT_RED : RED;
            break;
        case RING_YELLOW:
            if ( i == _player_idx ) {
                fg = DARK_GRAY;
            } else {
                fg = VERY_DARK_GRAY;
            }
            bg = TRANSPARENT;
            ch = 1;
            break;
        default:
            fg = _player_colors[i];
            break;
    }

    if ( bg != TRANSPARENT ) {
        DrawChar(x, y, 219, bg);
    }

    DrawChar(x, y, ch, fg);
}

void RenderGame(void)
{
    static SDL_Rect map_rect = GetMapRect();
    SDL_Rect hud_rects[MAX_PLAYERS];
    GetHUDRects(hud_rects);

    ClearWindow(20, 20, 20);

    // Player HUD

    Ranking ranking = GetRanking();

    // Render HUD for each player
    for ( int i = 0; i < nplayers_g; i++ ) {

#if 0
        float progress = (float)point_timer_i.sec / point_timer_i.reset_sec;
        int len = hud_rects[i].w * progress;
        DrawHorizontalLine(hud_rects[i].x,
                           hud_rects[i].y - 2,
                           len,
                           player_colors_i[i]);
#endif

        SetViewport(&hud_rects[i]);

        // Health
        for ( int j = 0; j < MAX_PLAYER_HEALTH; j++ ) {
            Color fg = j < _players[i].health ? _player_colors[i] : GRAY;
            DrawChar(j * CHAR_WIDTH, HUD_LINE(1), 3, fg);
        }

        // Held Ring
//        if ( players_i[i].held ) {
            DrawChar(0, HUD_LINE(2), 0x09, RingColor(_players[i].held));
//        }

        // Socket Points
        u8 * sock = &_sockets[i * NUM_SOCKETS_PER_PLAYER];
        DrawText(0, HUD_LINE(2), RingColor(*sock), "  %d  ",
                 RingValue(*sock, i));
        sock++;
        DrawText(0, HUD_LINE(2), RingColor(*sock), "   %d ",
                 RingValue(*sock, i));
        sock++;
        DrawText(0, HUD_LINE(2), RingColor(*sock), "    %d",
                 RingValue(*sock, i));

        // First Place Marker
        if ( ranking.is_in_first[i] ) {
            Color fg;
            if ( ranking.num_in_first == 1 ) {
                fg = YELLOW;
            } else {
                fg = BRIGHT_BLUE;
            }

            DrawChar(0, HUD_LINE(3), 224, fg);
        }

        // Points
        DrawText(0, HUD_LINE(3), _player_colors[i], "  %3d", _players[i].pts);
    }

    SetViewport(&map_rect);

    // Tile map
    for ( int y = 0; y < MAP_SIZE; y++ ) {
        for ( int x = 0; x < MAP_SIZE; x++ ) {
            DrawTile(_tile_map[y][x], x, y);
        }
    }

    // Rings
    for ( int i = 0; i < _nrings; i++ ) {
        DrawChar(_rings[i].x * TILE_SIZE,
                 _rings[i].y * TILE_SIZE,
                 0x09,
                 RingColor(_rings[i].type));
    }

    // Players
    for ( int i = 0; i < nplayers_g; i++ ) {
        DrawPlayer(i);
    }

    SetViewport(NULL);
    RefreshWindow();
}

void RenderMatchOver(void)
{
    ClearWindow(20, 20, 20);

    int winner_idx = -1;
    int max_pts = 0;
    for ( int p = 0; p < nplayers_g; p++ ) {
        if ( _players[p].pts > max_pts ) {
            max_pts = _players[p].pts;
            winner_idx = p;
        }
    }

    int y = 32;
    DrawCenteredText(y, _player_colors[winner_idx], "%c Wins!", 2);
    y += 32;

    for ( int p = 0; p < nplayers_g; p++ ) {
        DrawCenteredText(y, _player_colors[p], "%d. %c %3d points",
                         p + 1, 2, _players[p].pts);
        y += CHAR_HEIGHT * 1.5;
    }

    RefreshWindow();
}

#pragma mark - Update Functions

bool CollideWithPlayer(Player * self, int x, int y, int dx, int dy)
{
    for ( int i = 0; i < nplayers_g; i++ ) {

        Player * hit = &_players[i];

        if ( hit != self && x == hit->x && y == hit->y ) {
            // Collision:

            // Set up bump animation.
            self->offx = dx * TILE_SIZE * 0.5;
            self->offy = dy * TILE_SIZE * 0.5;

            // Do damage.
            if ( self->held == RING_RED ) {
                hit->health = 0;
            } else if ( self->held
                       && RingColor(self->held) == _player_colors[_player_idx] ) {
                hit->health -= 2;
            } else if ( self->held == RING_RAINBOW ) {
                hit->health -= 2;
            } else {
                hit->health--;
            }

            if ( hit->health <= 0 ) {
                // TODO: killed
            }

            printf("health: %d\n", hit->health);
            _curr_sound = S_ATTACK;

            // Move the hit player.
            int pdx = hit->x - self->x;
            int pdy = hit->y - self->y;
            TryMovePlayer(hit, hit->x + pdx, hit->y + pdy);

            return true;
        }
    }

    return false;
}

void TeleportPlayer(Player * player, int from_x, int from_y)
{
    for ( int y = 0; y < MAP_SIZE; y++ ) {
        for ( int x = 0; x < MAP_SIZE; x++ ) {
            if ( _tile_map[y][x] == 'T'
                && x != from_x
                && y != from_y )
            {
                player->x = x;
                player->y = y;
                _curr_sound = S_TELEPORT;
                return;
            }
        }
    }
}

void TryMovePlayer(Player * player, int try_x, int try_y)
{
    int dx = try_x - player->x;
    int dy = try_y - player->y;

    // Wrap position
    if ( try_x < 0 ) try_x += MAP_SIZE;
    if ( try_y < 0 ) try_y += MAP_SIZE;
    if ( try_x >= MAP_SIZE ) try_x -= MAP_SIZE;
    if ( try_y >= MAP_SIZE ) try_y -= MAP_SIZE;

    switch ( _tile_map[try_y][try_x] ) {
        case '.': // Empty
        case '0': case '1': case '2': case '3': // Player spawn platforms
        case 'a': case 'b': case 'c': // Player 1 Ring sockets
        case 'd': case 'e': case 'f': // Player 2 Ring sockets
        case 'g': case 'h': case 'i': // Player 3 Ring sockets
        case 'j': case 'k': case 'l': // Player 4 Ring sockets
        case 'G': // Grass
        case 'o': // Ring Disposer
        case 'T': // Teleporter
        {
            // No tile collision.
            char tile = _tile_map[try_y][try_x];

            // TODO: refactor
            // if ( !CollideWithPlayer ) {
            //      StepOntoEmptyTile(type)
            // }

            // Check for a player:
            if ( CollideWithPlayer(player, try_x, try_y, dx, dy) ) {
                return;
            }

            // No collision with a player, move and check for pick-ups:

            player->x = try_x;
            player->y = try_y;
            player->offx = -dx * TILE_SIZE; // Step animation
            player->offy = -dy * TILE_SIZE;

            // Check if the player stepped onto a ring.
            if ( !player->held ) {
                for ( int i = 0; i < _nrings; i++ ) {
                    if ( player->x == _rings[i].x && player->y == _rings[i].y ) {
                        player->held = _rings[i].type; // Pick it up.
                        _rings[i] = _rings[--_nrings]; // Remove from board.
                        _curr_sound = S_RING_COLLECT;
                    }
                }
            }

            // Stepped onto a socket.
            if ( tile >= 'a' && tile <= 'l' ) {

                u8 * socket = &_sockets[_tile_map[try_y][try_x] - 'a'];

                if ( !(*socket) && player->held ) {
                    // Place a held ring into the empty socket.
                    *socket = player->held;
                    player->held = 0;
                    _curr_sound = S_PLACE_IN_SOCKET;
                } else if ( *socket && !player->held ) {
                    // Pick up the item in the socket.
                    player->held = *socket;
                    *socket = 0;
                    _curr_sound = S_REMOVE_FROM_SOCKET;
                }
            }

            // Stepped onto the ring disposer.
            if ( tile == 'o' ) {
                if ( player->held && !_disposal ) {
                    _disposal = player->held;
                    player->held = RING_NONE;
                    _dispose_timer.sec = 5.0f;
                    _curr_sound = S_DISPOSER_PLACE;
                } else if ( !player->held && _disposal ) {
                    player->held = _disposal;
                    _disposal = RING_NONE;
                    _dispose_timer.sec = 0.0f;
                    _curr_sound = S_RING_COLLECT;
                }
            }

            break;
        }
        case 'W':
            // Blocking and no bump animation: do nothing.
            break;
        default:
            // Bump into:
            player->offx = dx * TILE_SIZE * 0.5;
            player->offy = dy * TILE_SIZE * 0.5;
            _curr_sound = S_BUMP;
            break;
    }
}

void UpdatePlayer(Player * player, Action action)
{
    if ( player->offx || player->offy ) {
        player->offx -= SIGN(player->offx);
        player->offy -= SIGN(player->offy);

        if ( player->offx == 0 && player->offy == 0 ) {
            // Arrive, check if on teleporter.
            if ( _tile_map[player->y][player->x] == 'T' ) {
                TeleportPlayer(player, player->x, player->x);
            }
        }
    } else if ( action != A_NONE ) {

        int dx = 0;
        int dy = 0;

        if ( action & A_MOVE_UP ) dy--;
        if ( action & A_MOVE_DOWN) dy++;
        if ( action & A_MOVE_LEFT) dx--;
        if ( action & A_MOVE_RIGHT) dx++;

        TryMovePlayer(player, player->x + dx, player->y + dy);
    }
}

void SpawnRing(void * data)
{
    if ( _nrings >= MAX_RINGS ) {
        return;
    }

    // Find free map spots
    u8 free_x[MAP_SIZE * MAP_SIZE];
    u8 free_y[MAP_SIZE * MAP_SIZE];
    int npts = 0;

    for ( int y = 0; y < MAP_SIZE; y++ ) {
        for ( int x = 0; x < MAP_SIZE; x++ ) {
            if ( Unoccupied(x, y) ) {
                free_x[npts] = x;
                free_y[npts] = y;
                npts++;
            }
        }
    }

    // Select a random free spot
    int rand_i = Rand(0, npts - 1);

    _rings[_nrings].x = free_x[rand_i];
    _rings[_nrings].y = free_y[rand_i];
    _rings[_nrings].type = GetRandomRingType();
    _nrings++;

    _curr_sound = S_RING_SPAWN;
}

void DisposeRing(void * data)
{
    _curr_sound = S_RING_DISPOSED;
    _disposal = RING_NONE;
}

void UpdatePoints(void * data)
{
    // Update points based on rings in sockets.
    bool match_over = false;

    for ( int s = 0; s < NUM_SOCKETS; s++ ) {
        if ( _sockets[s] ) {
            int player_index = s / NUM_SOCKETS_PER_PLAYER;
            int points = RingValue(_sockets[s], player_index);
            _players[player_index].pts += points;
            if ( _players[player_index].pts > MAX_POINTS ) {
                match_over = true;
            }
        }
    }

    Ranking ranking = GetRanking();
    if ( ranking.num_in_first == 1 && match_over ) {
        _curr_state = GS_MATCH_OVER;
        _curr_sound = S_MATCH_OVER;
        return;
    }

    // Increase health for those standing on their spawn platform
    for ( int p = 0; p < nplayers_g; p++ ) {
        int spawn_x = -1;
        int spawn_y = -1;
        bool scan = true;

        for ( int y = 0; y < MAP_SIZE && scan; y++ ) {
            for ( int x = 0; x < MAP_SIZE && scan; x++ ) {
                if ( _tile_map[y][x] == p + '0' ) {
                    spawn_x = x;
                    spawn_y = y;
                    scan = false;
                }
            }
        }

        if ( spawn_x == -1 || spawn_y == -1 ) {
            ERROR("Programmer effed up big");
        }

        if ( _players[p].x == spawn_x && _players[p].y == spawn_y ) {
            if ( _players[p].health < MAX_PLAYER_HEALTH ) {
                _players[p].health++;
                _curr_sound = S_REGEN;
            }
        }
    }
}

void ServerUpdate(Action action, float dt)
{
    Action actions[MAX_PLAYERS] = { [0] = action };
    for ( int i = 1; i < nplayers_g; i++ ) {
        actions[i] = A_NONE;
    }

    // Read client actions.
    for ( int i = 1; i < nplayers_g; i++ ) {
        BufferClear(&_net_buf);
        if ( _connections[i].is_init ) {
            if ( PacketRead(&_connections[i], &_net_buf) ) {
                BufferRead(&_net_buf, &actions[i], sizeof(Action));
            }
        }
    }

    // Update Game
    RunTimer(&_dispose_timer, dt);
    RunTimer(&_ring_timer, dt);
    RunTimer(&_point_timer, dt);

    for ( int i = 0; i < nplayers_g; i++ ) {
        UpdatePlayer(&_players[i], actions[i]);
    }

    // Serialize and send game state.

    BufferClear(&_net_buf);
    BufferWrite(&_net_buf, _players, sizeof(_players));
    BufferWrite(&_net_buf, &_nrings, sizeof(_nrings));
    BufferWrite(&_net_buf, _rings, sizeof(_rings));
    BufferWrite(&_net_buf, &_curr_sound, sizeof(_curr_sound));
    BufferWrite(&_net_buf, _sockets, sizeof(_sockets));
    BufferWrite(&_net_buf, &_disposal, sizeof(_disposal));

    for ( int i = 1; i < nplayers_g; i++ ) {
        if ( _connections[i].is_init ) {
            if ( !PacketWrite(&_connections[i], &_net_buf) ) {
                fprintf(stderr, "ServerUpdate: packet write failed\n");
            }
        }
    }
}

void ClientUpdate(Action action)
{
    Player * player = &_players[_player_idx];

    // Send action, if any.
    if ( action != A_NONE && player->offx == 0 && player->offy == 0 ) {
        BufferClear(&_net_buf);
        BufferWrite(&_net_buf, &action, sizeof(action));
        if ( !PacketWrite(&_client, &_net_buf) ) {
            fprintf(stderr, "ClientUpdate: packetwrite failed\n");
        }
    }

    // Receive game state.

    BufferClear(&_net_buf);
    if ( PacketRead(&_client, &_net_buf) ) {
        // Deserialize:
        BufferRead(&_net_buf, _players, sizeof(_players));
        BufferRead(&_net_buf, &_nrings, sizeof(_nrings));
        BufferRead(&_net_buf, _rings, sizeof(_rings));
        BufferRead(&_net_buf, &_curr_sound, sizeof(_curr_sound));
        BufferRead(&_net_buf, _sockets, sizeof(_sockets));
        BufferRead(&_net_buf, &_disposal, sizeof(_disposal));
    }
}

void UpdateGame(Action action, float dt)
{
    RunTimer(&_key_timer, dt);

    if ( session_g == SN_CLIENT ) {
        ClientUpdate(action);
    } else {
        ServerUpdate(action, dt);
    }
}

#pragma mark - Init Functions

bool InitServer(void)
{
    SetWindowTitle("Server");
    SetWindowPosition(0, 0);

    Socket server = CreateServer("5555");
    if ( !server.is_init ) {
        fprintf(stderr, "CreateServer failed: %s", GetNetError());
        return false;
    }

    // Wait for all clients to connect before starting
    for ( int i = 1; i < nplayers_g; i++ ) {
        printf("Waiting for player %d to connect...\n", i + 1);

        while ( !_connections[i].is_init ) {
            if ( !AcceptConnection(&server, &_connections[i]) ) {
                fprintf(stderr, "AcceptConnection failed: %s\n", GetNetError());
                return false;
            }
        }

        printf("Player %d connected.\n", i + i);
    }

    CloseSocket(&server);

    // Send clients the number of players and their index
    for ( int i = 1; i < nplayers_g; i++ ) {
        NetWriteAll(&_connections[i], &i, sizeof(i));
        NetWriteAll(&_connections[i], &nplayers_g, sizeof(nplayers_g));
    }

    return true;
}

void InitClient(const char * ip, const char * port)
{
    SetWindowTitle("Client");
    SetWindowPosition(_player_idx * (GAME_WIDTH / 2) * SCALE, 0);

    _client = CreateClient(ip, port);

    if ( !_client.is_init ) {
        fprintf(stderr, "CreateClient failed: %s", GetNetError());
        exit(1);
    }

    // Wait for the server to assign our player_index.
    if ( !NetReadAll(&_client, &_player_idx, sizeof(_player_idx)) ) {
        fprintf(stderr, "Could not read player index: %s\n", GetNetError());
        exit(1);
    }

    // Wait for the server to send how many players there are.
    if ( !NetReadAll(&_client, &nplayers_g, sizeof(nplayers_g)) ) {
        fprintf(stderr, "Could not read number of players: %s\n", GetNetError());
        exit(1);
    }

    printf("Connected as player %d\n", _player_idx);
}

bool InitGame(const char * ip, const char * port)
{
    atexit(QuitGame);
    Randomize();
    BufferInit(&_net_buf, 1024);

    if ( session_g == SN_SERVER ) {
        InitNetwork("log_server.txt");
        if ( !InitServer() ) {
            fprintf(stderr, "InitServer failed: %s\n", GetNetError());
            return false;
        }
    } else if ( session_g == SN_CLIENT) {
        InitNetwork("log_client.txt");
        InitClient(ip, port);
    }

    // Find player spawn spots.
    int spawn_x[MAX_PLAYERS];
    int spawn_y[MAX_PLAYERS];

    for ( int y = 0; y < MAP_SIZE; y++ ) {
        for ( int x = 0; x < MAP_SIZE; x++ ) {
            char t = _tile_map[y][x];
            if ( t >= '0' && t <= '3' ) {
                spawn_x[t - '0'] = x;
                spawn_y[t - '0'] = y;
            }
        }
    }

    // Init players
    for ( int i = 0; i < nplayers_g; i++ ) {
        _players[i].x = spawn_x[i];
        _players[i].y = spawn_y[i];
        _players[i].health = MAX_PLAYER_HEALTH;
    }

    _curr_state = GS_PLAY;

    return true;
}

#pragma mark -

void DoFrame(float dt)
{
    SDL_Event event;
    while ( SDL_PollEvent(&event) ) {
        switch ( event.type ) {
            case SDL_EVENT_QUIT:
                is_running_g = false;
                return;
            case SDL_EVENT_KEY_DOWN:
                switch ( event.key.key ) {
                    case SDLK_BACKSLASH:
                        ToggleFullscreen();
                        break;
                    default:
                        break;
                }
                break;
        }
    }

    if ( _state_handlers[_curr_state].do_input) {
        _state_handlers[_curr_state].do_input();
    }

    if ( _state_handlers[_curr_state].update ) {
        _state_handlers[_curr_state].update(_curr_action, dt);
    }

    _state_handlers[_curr_state].render();

    if ( _curr_sound ) {
        Play(_sounds[_curr_sound]);
        _curr_sound = S_NONE; // Don't wait for the server to reset the sound.
    }
}
