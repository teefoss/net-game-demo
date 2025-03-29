//
//  main.cc
//  NetTest2
//
//  Created by Thomas Foster on 2/9/25.
//

#include "beeper.hh"
#include "game.hh"
#include "net.hh"
#include "video.hh"

static const char * program_name;

static int ArgumentError(const char * message)
{
    puts(message);
    printf("usage: %s -s [port] [player count (1-4)]\n", program_name);
    printf("usage: %s -c [IP] [port]\n", program_name);
    
    return EXIT_FAILURE;
}

int main(int argc, char ** argv)
{
    program_name = argv[0];
    const char * port = NULL;
    const char * ip = NULL;

    // Parse args.
    if ( argc == 1 ) {
        return ArgumentError("Bad arguments.");
#if 0
        nplayers_g = 2; // Single player testing
        session_g = SN_SINGLE_PLAYER;
#endif
    } else if ( argc == 4 ) {
        if ( strcmp(argv[1], "-s") == 0 ) {
            session_g = SN_SERVER;
            port = argv[2];
            nplayers_g = atoi(argv[3]);
            if ( nplayers_g == 0 ) {
                return ArgumentError("Invalid player count (expected 1-4)\n");
            }
        } else if ( strcmp(argv[1], "-c") == 0 ) {
            session_g = SN_CLIENT;
            ip = argv[2];
            port = argv[3];
        } else {
            return ArgumentError("Expected -s or -c");
        }
    } else {
        return ArgumentError("Bad arguments");
    }

    InitVideo(GAME_WIDTH, GAME_HEIGHT, SCALE);
    SDL_Delay(1000); // This stops weird errors from happening
    InitBeeper();

    if ( !InitGame(ip, port) ) {
        return EXIT_FAILURE;
    }

    u64 frequency = SDL_GetPerformanceFrequency();
    u64 last = SDL_GetPerformanceCounter();
    float target_frame_sec = 1.0f / 60.0f;

    while ( is_running_g ) {
        u64 now = SDL_GetPerformanceCounter();
        float dt = ((float)(now - last) / (float)frequency);

        if ( dt > 0.05f ) {
            dt = 0.05f;
        } else if ( dt < target_frame_sec ) {
            SDL_Delay(1);
            continue;
        }

        dt = target_frame_sec;
        last = now;

        DoFrame(dt);
    }

    return 0;
}
