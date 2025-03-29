//
//  game.hh
//  NetTest2
//
//  Created by Thomas Foster on 3/18/25.
//

#ifndef game_hh
#define game_hh

#include "misc.hh"
#include "video.hh"

#define GAME_WIDTH 320
#define GAME_HEIGHT 200
#define SCALE 3

#define TILE_SIZE (CHAR_WIDTH)
#define MAP_SIZE 25
#define MAX_PLAYERS 4
#define MAX_PLAYER_HEALTH 5

typedef u8 Action;
#define A_NONE         0x00
#define A_MOVE_UP      0x01
#define A_MOVE_DOWN    0x02
#define A_MOVE_LEFT    0x04
#define A_MOVE_RIGHT   0x08
#define A_WRITE_TEST   0x10

enum GameState {
    GS_PLAY,
    GS_MATCH_OVER,
};

// When players are carrying their own color ring, they do more damage
enum RingType {
    RING_NONE,

    RING_BLUE,
    RING_GREEN,
    RING_CYAN,
    RING_RED, // Pauses point generation [Carrying: Insta kill others]
    RING_MAGENTA,
    RING_YELLOW, // [Carrying: invisibility/invulnerability]
    RING_RAINBOW, // Extra bonus, any player [Carrying: increased movement speed]
    RING_WHITE,
    NUM_RING_TYPES,
};

enum Session {
    SN_SINGLE_PLAYER,
    SN_SERVER,
    SN_CLIENT
};

// TODO: order based on priority and write SetSound() that only sets a sound
// if the priority is higher than the current sound.
enum Sound {
    S_NONE,
    S_REMOVE_FROM_SOCKET,
    S_PLACE_IN_SOCKET,
    S_ATTACK,
    S_BUMP,
    S_RING_SPAWN,
    S_RING_COLLECT,
    S_DISPOSER_PLACE,
    S_RING_DISPOSED,
    S_TELEPORT,
    S_REGEN,
    S_MATCH_OVER,
};

struct GameStateHandler {
    bool (* do_input)(void);
    void (* update)(Action, float);
    void (* render)(void);
};

// Appearance of tile or actor
struct Glyph {
    u8 ch[2];
    Color fg;
    Color bg;
};

struct Player {
    s8 x;
    s8 y;
    s8 offx; // Horizontal draw offset in pixels
    s8 offy; // Vertical draw offset in pixels
    s8 health;
    s8 held; // RingType
    s8 pts;
    s8 unused;
};

struct Ranking {
    int num_in_first;
    int in_first_indices[MAX_PLAYERS];
    bool is_in_first[MAX_PLAYERS];
};

struct Ring {
    u8 x;
    u8 y;
    u8 type;
    u8 unused;
};

extern bool is_running_g;
extern Session session_g;
extern int nplayers_g;

bool InitGame(const char * ip, const char * port);
bool InitServer(void);
void InitClient(const char * ip, const char * port);
void DoFrame(float dt);

#endif /* game_hh */
