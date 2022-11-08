/*
 * MozziGuts.cpp
 *
 * Copyright 2012 Tim Barrass.
 *
 * This file is part of Mozzi.
 *
 * Mozzi by Tim Barrass is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License.
 *
 */

/** README! 
 * This file is meant to be used as a template when adding support for a new platform. Please read these instructions, first.
 *
 *  Files involved:
 *  1. Modify hardware_defines.h, adding a macro to detect your target platform
 *  2. Modify MozziGuts.cpp to include MozziGuts_impl_YOURPLATFORM.hpp
 *  3. Modify MozziGuts.h to include AudioConfigYOURPLATFORM.h
 *  4. Copy this file to MozziGuts_impl_YOURPLATFORM.hpp and adjust as necessary
 *  (If your platform is very similar to an existing port, it may instead be better to modify the existing MozziGuts_impl_XYZ.hpp/AudioConfigYOURPLATFORM.h,
 *  instead of steps 2-3.).
 *  Some platforms may need small modifications to other files as well, e.g. mozzi_pgmspace.h
 *
 *  How to implement MozziGuts_impl_YOURPLATFORM.hpp:
 *  - Follow the NOTEs provided in this file
 *  - Read the doc at the top of AudioOutput.h for a better understanding of the basic audio output framework
 *  - Take a peek at existing implementations for other hardware (e.g. TEENSY3/4 is rather complete while also simple at the time of this writing)
 *  - Wait for more documentation to arrive
 *  - Ask when in doubt
 *  - Don't forget to provide a PR when done (it does not have to be perfect; e.g. many ports skip analog input, initially)
 */

// The main point of this check is to document, what platform & variants this implementation file is for.
#if !(IS_MEGAAVR())
#  error "Wrong implementation included for this platform"
#endif
// Add platform specific includes and declarations, here


////// BEGIN analog input code ////////

/** NOTE: This section deals with implementing (fast) asynchronous analog reads, which form the backbone of mozziAnalogRead(), but also of USE_AUDIO_INPUT (if enabled).
 *  This template provides empty/dummy implementations to allow you to skip over this section, initially. Once you have an implementation, be sure to enable the
 *  #define, below: */
//#define MOZZI_FAST_ANALOG_IMPLEMENTED

// Insert here code to read the result of the latest asynchronous conversion, when it is finished.
// You can also provide this as a function returning unsigned int, should it be more complex on your platform
#define getADCReading() 0

/** NOTE: On "pins" vs. "channels" vs. "indices"
 *  "Pin" is the pin number as would usually be specified by the user in mozziAnalogRead().
 *  "Channel" is an internal ADC channel number corresponding to that pin. On many platforms this is simply the same as the pin number, on others it differs.
 *      In other words, this is an internal representation of "pin".
 *  "Index" is the index of the reading for a certain pin/channel in the array of analog_readings, ranging from 0 to NUM_ANALOG_PINS. This, again may be the
 *      same as "channel" (e.g. on AVR), however, on platforms where ADC-capable "channels" are not numbered sequentially starting from 0, the channel needs
 *      to be converted to a suitable index.
 *
 *  In summary, the semantics are roughly
 *      mozziAnalogRead(pin) -> _ADCimplementation_(channel) -> analog_readings[index]
 *  Implement adcPinToChannelNum() and channelNumToIndex() to perform the appropriate mapping.
 */
// NOTE: Theoretically, adcPinToChannelNum is public API for historical reasons, thus cannot be replaced by a define
#define channelNumToIndex(channel) channel
uint8_t adcPinToChannelNum(uint8_t pin) {
  return pin;
}

/** NOTE: Code needed to trigger a conversion on a new channel */
void adcStartConversion(uint8_t channel) {
#warning Fast analog read not implemented on this platform
}

/** NOTE: Code needed to trigger a subsequent conversion on the latest channel. If your platform has no special code for it, you should store the channel from
 *  adcStartConversion(), and simply call adcStartConversion(previous_channel), here. */
void startSecondADCReadOnCurrentChannel() {
#warning Fast analog read not implemented on this platform
}

/** NOTE: Code needed to set up faster than usual analog reads, e.g. specifying the number of CPU cycles that the ADC waits for the result to stabilize.
 *  This particular function is not super important, so may be ok to leave empty, at least, if the ADC is fast enough by default. */
void setupFastAnalogRead(int8_t speed) {
#warning Fast analog read not implemented on this platform
}

/** NOTE: Code needed to initialize the ADC for asynchronous reads. Typically involves setting up an interrupt handler for when conversion is done, and
 *  possibly calibration. */
void setupMozziADC(int8_t speed) {
#warning Fast analog read not implemented on this platform
}

/* NOTE: Most platforms call a specific function/ISR when conversion is complete. Provide this function, here.
 * From inside its body, simply call advanceADCStep(). E.g.:
void stm32_adc_eoc_handler() {
  advanceADCStep();
}
*/
////// END analog input code ////////

////// BEGIN audio output code //////
/* NOTE: Some platforms rely on control returning from loop() every so often. However, updateAudio() may take too long (it tries to completely fill the output buffer,
 * which of course is being drained at the same time, theoretically it may not return at all). If you set this define, it will be called once per audio frame to keep things
 * running smoothly. */
//#define LOOP_YIELD yield();

// Required for Timer B use only
#define CCMPL_TOP 243

#if (EXTERNAL_AUDIO_OUTPUT != true) // otherwise, the last stage - audioOutput() - will be provided by the user
/** NOTE: This is the function that actually write a sample to the output. In case of EXTERNAL_AUDIO_OUTPUT == true, it is provided by the library user, instead. */
inline void audioOutput(const AudioOutput f) {
  // e.g. analogWrite(AUDIO_CHANNEL_1_PIN, f.l()+AUDIO_BIAS);
#if (AUDIO_CHANNELS > 1)
  // e.g. analogWrite(AUDIO_CHANNEL_2_PIN, f.r()+AUDIO_BIAS);
#endif

#if (AUDIO_CHANNEL_1_PIN==9)
  // We want an AUDIO_RATE of 16384Hz and PWM rate of 32768Hz
  //
  // For PWM single-slop mode, the calculation (see 20.3.3.4.3 in the ATmega4809
  // datasheet) is:
  //    Freq = System Clock / (Prescaler . PERIOD)
  // 
  // So with Prescaler = DIV1 (i.e. 1) we can rearrange to get the value for
  //    PERIOD = System Clock / Freq
  //
  // For a 32768Hz update frequency:
  //    PERIOD = 16Mhz / 32768 = 488.28
  //
  // So the counter will run from 0 to 488 in single-slope mode
  // and the PWM duty cycle can go from 0 (0%) to 488 (100%) too.
  //
  // So we can use the pseudo "9-bit" mode which means PWM goes 0 to 488.
  // Then use the PWM value directly with no further scaling required.
  TCA0.SINGLE.CMP0BUF = f.l() + AUDIO_BIAS;
#elif (AUDIO_CHANNEL_1_PIN==3)
  // We want an AUDIO_RATE of 16384Hz but PWM rate is 65536Hz
  //
  // For 8-bit PWM mode, the calculation (see 21.3.3.1.8 in the ATmega4809
  // datasheet) is:
  //    Freq = System Clock / (Prescaler . (PERIOD + 1))
  // 
  // So with Prescaler = DIV1 (i.e. 1) we can rearrange to get the value for
  //    PERIOD + 1 = System Clock / Freq
  //
  // For a 32768Hz update frequency:
  //    PERIOD + 1 = 16Mhz / 32768 = 488 which is too high.
  //
  // So for a 65536Hz update frequency:
  //    PERIOD + 1 = 16MHz / 65536 = 244
  //    PERIOD     = 243
  //
  // To maintain an AUDIO_RATE of 16384, we need to only perform
  // audio updates for 1 in 4 interrupts.
  //
  // So the counter will run from 0 to 243 and the PWM duty cycle
  //  can go from 0 (0%) to 243 (100%) too.
  //
  // So for a typical 0-255 PWM value this needs scaling to 0 to 244.
  //
  // Calculation: scaledpwm = pwm * 244 / 256
  //                        = (pwm * 244) >> 8
  //
  uint16_t interimF = (f.l() + AUDIO_BIAS) * 244;
  TCB1.CCMPL = CCMPL_TOP;
  TCB1.CCMPH = (interimF >> 8);
#endif
}
#endif

#if (AUDIO_CHANNEL_1_PIN==9)
ISR(TCA0_OVF_vect, ISR_BLOCK) {
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm; // Clear the interrupt flag

#if (AUDIO_RATE == 16384) // only update every second ISR, if lower audio rate
  static boolean alternate;
  alternate = !alternate;
  if (alternate) return;
#endif

  defaultAudioOutput();
}

#elif (AUDIO_CHANNEL_1_PIN==3)
ISR(TCB1_INT_vect, ISR_BLOCK) {
  TCB1.INTFLAGS = TCB_CAPT_bm; // Clear the interrupt flag

#if (AUDIO_RATE == 16384) // only update every fourth ISR, if lower audio rate
  static uint8_t alternate;
  alternate++;
  if (alternate < 4) {
    return;
  }
  alternate = 0;    
#elif (AUDIO_RATE == 32768) // only update every second ISR
  static boolean alternate;
  alternate = !alternate;
  if (alternate) return;
#endif

  defaultAudioOutput();
}
#endif


static void startAudio() {
  // Add here code to get audio output going. This usually involves:
  // 1) setting up some DAC mechanism (e.g. setting up a PWM pin with appropriate resolution
  // 2a) setting up a timer to call defaultAudioOutput() at AUDIO_RATE
  // OR 2b) setting up a buffered output queue such as I2S (see ESP32 / ESP8266 for examples for this setup)
#if (EXTERNAL_AUDIO_OUTPUT != true)
  // remember that the user may configure EXTERNAL_AUDIO_OUTPUT, in which case, you'll want to provide step 2a), and only that.
    
  pinMode(AUDIO_CHANNEL_1_PIN, OUTPUT);
    
#if (AUDIO_CHANNEL_1_PIN==9)
  // Configure Timer A for use with D9 (PB0)
  PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTB_gc; // Enable "alternative" pin output PORTB[5:0] for TCA0
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm;  // SysClk and enabled
  TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP0EN_bm | TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
  TCA0.SINGLE.CTRLD = 0; // Normal (Single) mode (not split)
  TCA0.SINGLE.PER = 487; // 32768 Hz "tick"
  TCA0.SINGLE.CMP0 = 0;

  TCA0.SINGLE.INTCTRL |= TCA_SINGLE_OVF_bm;  // Turn on interrupts
#elif (AUDIO_CHANNEL_1_PIN==3)
  // Configure Timer B1 for use with D3 (PF5)
  PORTMUX.TCBROUTEA |= PORTMUX_TCB1_bm; // Enable "alternative" pin output (PF5=D3) for TCB1
  TCB1.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm;  // SysClk and enabled
  TCB1.CTRLB = TCB_CNTMODE_PWM8_gc | TCB_CCMPEN_bm;    // 8-bit PWM and output enabled
  TCB1.CCMPL = CCMPL_TOP;
  TCB1.CCMPH = 0;

  TCB1.INTCTRL |= TCB_CAPT_bm;  // Turn on interrupts
#endif
#endif
}

void stopMozzi() {
  // Add here code to pause whatever mechanism moves audio samples to the output
}
////// END audio output code //////
