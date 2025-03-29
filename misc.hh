//
//  misc.hh
//  Realm
//
//  Created by Thomas Foster on 3/10/25.
//

#ifndef misc_h
#define misc_h

#include <stdint.h>

#define DEG2RAD(deg)        ((deg) * (M_PI / 180.0))
#define RAD2DEG(rad)        ((rad) * (180.0 / M_PI))
#define COMPARE(x, y)       (((x) > (y)) - ((x) < (y)))
#define SIGN(x)             COMPARE(x, 0)
#define CLAMP(x, min, max)  (x < min ? min : x > max ? max : x)
#define MAP(x, a, b, c, d)  (((x) - (a)) * ((d) - (c)) / ((b) - (a)) + (c))

#define MY_DEBUG 1
#if MY_DEBUG
#define ERROR(...) _Error(__FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define ERROR
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef void (* TimerCallback)(void *);

struct Timer {
    float sec;
    float reset_sec;
    TimerCallback callback;
    void * data;
};

[[noreturn]] void _Error(const char * file,
                         int line,
                         const char * func,
                         const char * msg, ...);

float       Lerp(float a, float b, float w);
float       DistanceSquared(int ax, int ay, int bx, int by);
void        Shuffle(int * values, int count);
unsigned    WangHash(unsigned x, unsigned y);
Timer       InitTimer(float sec,
                      float reset_sec,
                      TimerCallback callback,
                      void * data = nullptr);
bool        RunTimer(Timer * timer, float dt);

template <typename T>
constexpr const T& min(const T& a, const T& b) {
    return (b < a) ? b : a;
}

template <typename T>
constexpr const T& max(const T& a, const T& b) {
    return (a < b) ? b : a;
}

#endif /* misc_h */
