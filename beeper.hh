/*
 * Beeper Sound Library
 *
 * author: Thomas Foster
 * created: 04/2022
 * description: Library for emulating IBM PC Speaker (Beeper) sound.
 *
 */

#ifndef beeper_h
#define beeper_h

extern bool beeper_on; // Set to false to mute all sound.

void InitBeeper(void);

/// Range: 1-15. Default: 8.
void SetVolume(unsigned value);

/// Play frequency 800 Hz for 0.2 seconds.
void Beep(void);

/// Play a frequency for given duration. Interrupts any currently playing
/// sound. To play several frequencies successively, use `QueueSound`.
void Sound(unsigned frequency, unsigned milliseconds);

/// Queue a frequency, which will be played immediately. May be called
/// successively to queue multiple frequencies.
void QueueSound(unsigned frequency, unsigned milliseconds);

/// Play sound from a BASIC-style music string.
void Play(const char * string, ...);

/// Stop any sound that is currently playing.
void StopSound(void);

#endif /* beeper_h */
