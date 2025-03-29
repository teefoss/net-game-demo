#include "misc.hh"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <SDL3/SDL.h>

[[noreturn]]
void _Error(const char * file,
            int line,
            const char * function,
            const char * format, ...)
{
    fprintf(stderr, "%s:%d: fatal error in %s: ", file, line, function);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "\n");
//    exit(EXIT_FAILURE);
    abort();
}

float Lerp(float a, float b, float w)
{
    return (1.0f - w) * a + w * b;
}

float DistanceSquared(int ax, int ay, int bx, int by)
{
    float dx = bx - ax;
    float dy = by - ay;
    return dx * dx + dy * dy;
}

void Shuffle(int * values, int count)
{
    for ( int i = count - 1; i > 0; i-- ) {
        int j = rand() % (i + 1);
        int temp = values[i];
        values[i] = values[j];
        values[j] = temp;
    }
}

unsigned int WangHash(unsigned x, unsigned y)
{
    unsigned seed = x * 73856093 ^ y * 19349663;
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

Timer InitTimer(float sec, float reset_sec, TimerCallback callback, void * data)
{
    return (Timer){
        .sec = sec,
        .reset_sec = reset_sec,
        .callback = callback,
        .data = data
    };
}

bool RunTimer(Timer * t, float dt)
{
    if ( t->sec > 0.0f ) {

        t->sec -= dt;
        if ( t->sec <= 0.0f ) {

            if ( t->callback ) {
                t->callback(t->data);
            }

            t->sec = t->reset_sec > 0.0f ? t->reset_sec : 0.0f;
            return true;
        }
    }

    return false;
}
