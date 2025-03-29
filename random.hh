//
//  random.hh
//  Realm
//
//  Created by Thomas Foster on 3/13/25.
//

#ifndef random_h
#define random_h

#include "misc.hh"

void SeedRand(u32 seed);
void Randomize(void);
u32 Rand32(void);
u32 Rand(u32 min, u32 max);
float RandF(float min, float max);
bool Chance(float percent);

#endif /* random_h */
