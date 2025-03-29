#include "beeper.hh"

#include <stdlib.h>
#include <stdarg.h>
#include <SDL3/SDL.h>

static const SDL_AudioSpec spec = {
    .format = SDL_AUDIO_S8,
    .freq = 44100,
    .channels = 1,
};
static SDL_AudioStream * stream;
static uint8_t volume = 8;

bool beeper_on = true;

static double NoteNumberToFrequency(int note_num)
{
    static const int frequencies[] = { // in octave 6
        4186, // C
        4435, // C#
        4699, // D
        4978, // D#
        5274, // E
        5588, // F
        5920, // F#
        6272, // G
        6645, // G#
        7040, // A
        7459, // A#
        7902, // B
    };

    int octave = (note_num - 1) / 12;
    int note = (note_num - 1) % 12;
    int freq = frequencies[note];

    int octaves_down = 6 - octave;
    while ( octaves_down-- )
        freq /= 2;

    return (double)freq;
}

void QueueSound(unsigned frequency, unsigned milliseconds)
{
    float period = (float)spec.freq / (float)frequency;
    int len = (float)spec.freq * ((float)milliseconds / 1000.0f);

    int8_t * buf = (int8_t *)malloc(len);

    if ( buf == NULL ) {
        return;
    }

    for ( int i = 0; i < len; i++ ) {
        if ( frequency == 0 ) {
            buf[i] = 0;
        } else {
            buf[i] = (int)((float)i / period) % 2 ? volume : -volume;
        }
    }

    SDL_PutAudioStreamData(stream, buf, len);
    free(buf);
}

void InitBeeper(void)
{
    if ( SDL_WasInit(SDL_INIT_AUDIO) == 0 ) {
        int result = SDL_InitSubSystem(SDL_INIT_AUDIO);
        if ( result < 0 ) {
            fprintf(stderr,
                    "error: failed to init SDL audio subsystem: %s",
                    SDL_GetError());
        }
    }

    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                       &spec,
                                       NULL,
                                       NULL);

    SDL_ResumeAudioStreamDevice(stream);
    // TODO: shutdown sound
}

void SetVolume(unsigned value)
{
    if ( value > 15 || value <= 0 ) {
        fprintf(stderr, "bad volume, expected value in range 1-15\n");
        return;
    }

    volume = value;
}

void Sound(unsigned frequency, unsigned milliseconds)
{
    SDL_ClearAudioStream(stream);
    QueueSound(frequency, milliseconds);
}

void StopSound(void)
{
    SDL_ClearAudioStream(stream);
}

void Beep(void)
{
    Sound(800, 200);
}

// Play

// L[1,2,4,8,16,32,64] default: 4
// O[0...6] default: 4
// T[32...255] default: 120

// [A...G]([+,#,-][1,2,4,8,16,32,64][.])
// N[0...84](.)
// P[v]

// TODO: figure out how to let this play simultaneously with sounds played
// from Sound()

static void PlayError(const char * msg, int line_position)
{
    printf("Play syntax error: %s (position %d)\n.", msg, line_position);
}

static void QueueNoteNumber(int note_num, int note_ms, int silence_ms)
{
    QueueSound(NoteNumberToFrequency(note_num), note_ms);
    QueueSound(0, silence_ms);
}

#define PLAY_DEBUG 0
#define PLAY_STRING_MAX 255

void Play(const char * string, ...)
{
    if ( strlen(string) > PLAY_STRING_MAX ) {
        printf("Play error: string too long (max %d)\n", PLAY_STRING_MAX);
        return;
    }

    va_list args;
    va_start(args, string);

    char buffer[PLAY_STRING_MAX + 1] = { 0 };
    vsnprintf(buffer, PLAY_STRING_MAX, string, args);
    va_end(args);

    // default settings
    int bmp = 120;
    int oct = 4;
    int len = 4;
    int background = 1;

    // A-G
    static const int note_offsets[7] = { 9, 11, 0, 2, 4, 5, 7 };

    enum {
        mode_staccato = 6,  // 6/8
        mode_normal = 7,    // 7/8
        mode_legato = 8     // 8/8
    } mode = mode_normal;

//    SDL_ClearQueuedAudio(device);
    SDL_ClearAudioStream(stream);

    // queue up whatever's in the string:

    const char * str = buffer;
    while ( *str != '\0') {
        char c = toupper(*str++);
        switch ( c ) {
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            case 'G': case 'N': case 'P':
            {
                // get note:
                int note = 0;
                switch ( c ) {
                    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                    case 'G':
                        note = 1 + (oct) * 12 + note_offsets[c - 'A'];
                        break;
                    case 'P':
                        note = 0;
                        break;
                    case 'N': {
                        int number = (int)strtol(str, (char **)&str, 10);
                        if ( number < 0 || number > 84 )
                            return PlayError("bad note number", (int)(str - string));
                        if ( number > 0 )
                            note = number;
                        break;
                    }
                    default:
                        break;
                }

                // adjust note per accidental:
                if ( c >= 'A' && c <= 'G' ) {
                    if ( *str == '+' || *str == '#' ) {
                        if ( note < 84 )
                            note++;
                        str++;
                    } else if ( *str == '-' ) {
                        if ( note > 1 )
                            note--;
                        str++;
                    }
                }

                int d = len;

                // get note value:
                if ( c != 'N' ) {
                    int number = (int)strtol(str, (char **)&str, 10);
                    if ( number < 0 || number > 64 )
                        return PlayError("bad note value", (int)(str - string));
                    if ( number > 0 )
                        d = number;
                }

                // count dots:
                int dot_count = 0;
                while ( *str == '.' ) {
                    dot_count++;
                    str++;
                }

                // adjust duration if there are dots:
                float total_ms = (60.0f / (float)bmp) * 1000.0f * (4.0f / (float)d);
                float prolongation = total_ms / 2.0f;
                while ( dot_count-- ) {
                    total_ms += prolongation;
                    prolongation /= 2;
                }

                // calculate articulation silence:
                int note_ms = total_ms * ((float)mode / 8.0f);
                int silence_ms = total_ms * ((8.0f - (float)mode) / 8.0f);

                // and finally, queue it
                QueueNoteNumber(note, note_ms, silence_ms);
                break;
            } // A-G, N, and P

            case 'T':
                bmp = (int)strtol(str, (char **)&str, 10);
                if ( bmp == 0 )
                    return PlayError("bad tempo", (int)(str - string));
                #if PLAY_DEBUG
                printf("set tempo to %d\n", bmp);
                #endif
                break;

            case 'O':
                if ( *str < '0' || *str > '6' )
                    return PlayError("bad octave", (int)(str - string));
                oct = (int)strtol(str, (char **)&str, 10);
                #if PLAY_DEBUG
                printf("set octave to %d\n", oct);
                #endif
                break;

            case 'L':
                len = (int)strtol(str, (char **)&str, 10);
                if ( len < 1 || len > 64 )
                    return PlayError("bad length", (int)(str - string));
                #if PLAY_DEBUG
                printf("set length to %d\n", len);
                #endif
                break;

            case '>':
                if ( oct < 6 )
                    oct++;
                #if PLAY_DEBUG
                printf("increase octave\n");
                #endif
                break;

            case '<':
                if ( oct > 0 )
                    oct--;
                #if PLAY_DEBUG
                printf("decrease octave\n");
                #endif
                break;

            case 'M': {
                char option = toupper(*str++);
                switch ( option ) {
                    case 'L': mode = mode_legato; break;
                    case 'N': mode = mode_normal; break;
                    case 'S': mode = mode_staccato; break;
                    case 'B': background = 1; break;
                    case 'F': background = 0; break;
                    default:
                        return PlayError("bad music option", (int)(str - string));
                        break;
                }
                break;
            }
            default:
                break;
        }
    }
}

#undef PLAY_DEBUG
