#ifndef AUDIOCONFIGMEGAAVR_H
#define AUDIOCONFIGMEGAAVR_H

#if not IS_MEGAAVR()
#error This header should be included for Arduino megaAVR, only
#endif

#define AUDIO_CHANNEL_1_PIN 9

#define STANDARD_PWM_RESOLUTION 488

#define AUDIO_BITS 9
#define AUDIO_BITS_PER_CHANNEL AUDIO_BITS

#define AUDIO_BIAS ((uint16_t) 244)

#endif        //  #ifndef AUDIOCONFIGMEGAAVR_H

