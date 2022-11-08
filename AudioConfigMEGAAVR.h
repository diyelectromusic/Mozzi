#ifndef AUDIOCONFIGMEGAAVR_H
#define AUDIOCONFIGMEGAAVR_H

#if not IS_MEGAAVR()
#error This header should be included for Arduino megaAVR, only
#endif

// Code will support audio output on the following PWM pins:
//   9 (Mozzi default) WARNING: This will mess with millis(), delay(), micros().  See below.
//   3 Lower resolution, but leaves system timing untouched.
#define AUDIO_CHANNEL_1_PIN 3

// Define the two timer/audio configurations
#if (AUDIO_CHANNEL_1_PIN==9)

// Configuration for D9 (Timer A)
#define STANDARD_PWM_RESOLUTION 488
#define AUDIO_BITS      8
#define AUDIO_BITS_NEAR 9
#define AUDIO_BITS_PER_CHANNEL AUDIO_BITS
#define AUDIO_BIAS ((uint16_t) 244)

// Factor to apply to any call to millis() or delay() etc
// Examples:
//   unsigned long realmillis = MOZZI_MILLIS_SCALING * millis();
// Or:
//   for (int i=0; i<MOZZI_MILLIS_SCALING; i++) {
//     delay (reqdmillis);
//   }
//
#define MOZZI_MILLIS_SCALING 64

#elif (AUDIO_CHANNEL_1_PIN==3)

// Configuration for D3 (Timer B)
#define STANDARD_PWM_RESOLUTION 256
#define AUDIO_BITS      8
#define AUDIO_BITS_NEAR 8
#define AUDIO_BITS_PER_CHANNEL AUDIO_BITS
#define AUDIO_BIAS ((uint16_t) 128)

// Factor to apply to any call to millis() or delay() etc
#define MOZZI_MILLIS_SCALING 1

#else
#  error "MegaAVR only supports audio output on D3 or D9."
#endif

#endif        //  #ifndef AUDIOCONFIGMEGAAVR_H

