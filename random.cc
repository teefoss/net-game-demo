#include "misc.hh"
#include <time.h>
#include <stdlib.h>

static u32 next = 1;

void SeedRand(u32 seed)
{
    next = seed;
}

void Randomize(void)
{
    next = (u32)time(NULL);
}

// https://lemire.me/blog/

static inline u32 Wyhash32(void)
{
    uint64_t tmp;
    uint32_t m1, m2;

    next += 0xE120FC15;
    tmp  = (uint64_t)next * 0x4A39B70D;
    m1   = (uint32_t)(( tmp >> 32) ^ tmp );
    tmp  = (uint64_t)m1 * 0x12FAD5C9;
    m2   = (uint32_t)( (tmp >> 32) ^ tmp );

    return m2;
}

u32 Rand32(void)
{
    return Wyhash32();
}

/// Returns a psuedo random interger between `min` and `max`, inclusive.
u32 Rand(u32 min, u32 max)
{
    return Wyhash32() % (max - min + 1) + min;
}

static inline float _RandomFloat(void)
{
    return (float)((double)Wyhash32() / (double)0xFFFFFFFF);
}

float RandF(float min, float max)
{
    return _RandomFloat() * (max - min) + min;
}

bool Chance(float percent)
{
    return _RandomFloat() < percent;
}
