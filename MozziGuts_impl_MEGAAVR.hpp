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

#if (EXTERNAL_AUDIO_OUTPUT != true) // otherwise, the last stage - audioOutput() - will be provided by the user
/** NOTE: This is the function that actually write a sample to the output. In case of EXTERNAL_AUDIO_OUTPUT == true, it is provided by the library user, instead. */
inline void audioOutput(const AudioOutput f) {
  // e.g. analogWrite(AUDIO_CHANNEL_1_PIN, f.l()+AUDIO_BIAS);
#if (AUDIO_CHANNELS > 1)
  // e.g. analogWrite(AUDIO_CHANNEL_2_PIN, f.r()+AUDIO_BIAS);
#endif

  // We want an AUDIO_RATE of 16384Hz and PWM rate of 32768Hz
  //
  // For PWM dual-slop mode, the calculation (see 20.3.3.4.4 in the ATmega4809
  // datasheet) is:
  //    Freq = System Clock / (2 . Prescaler . PERIOD)
  // 
  // So with Prescaler = DIV1 (i.e. 1) we can rearrange to get the value for
  //    PERIOD = System Clock / (2 . Freq)
  //
  // For a 32768Hz update frequency:
  //    PERIOD = 16Mhz / 65536 = 244.14
  //
  // So the counter will run from 0 to 244 and back to 0 in dual-slope mode
  // and the PWM duty cycle can go from 0 (0%) to 244 (100%) too.
  //
  // So for a typical 0-255 PWM value this needs scaling to 0 to 244.
  //
  // Calculation: scaledpwm = pwm * 256 / 244
  //                        = pwm * 1.049
  //
  // But if we can make the division /256 or /128 then a simple bit-shift
  // is pretty efficient.
  //
  // Now, 1.049*128 = 134, so 1.049 is ~ 134/128.
  //
  // So we can do pretty well with the scaling by doing a 16-bit calculation
  // to *134 and divide by 128, which is the same as a bit shift right of 7:
  //       pwm_16 = pwm * 134
  //    scaledpwm = pwm_16 >> 7
  //
  int16_t interimF = (f.l() + AUDIO_BIAS) * 134;
  TCA0.SINGLE.CMP0BUF = (interimF >> 7) ;
}
#endif

ISR(TCA0_OVF_vect, ISR_BLOCK) {
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm; // Clear the interrupt flag
#if (AUDIO_RATE == 16384) // only update every second ISR, if lower audio rate
  static boolean alternate;
  alternate = !alternate;
  if (alternate) return;
#endif

  defaultAudioOutput();
}


static void startAudio() {
  // Add here code to get audio output going. This usually involves:
  // 1) setting up some DAC mechanism (e.g. setting up a PWM pin with appropriate resolution
  // 2a) setting up a timer to call defaultAudioOutput() at AUDIO_RATE
  // OR 2b) setting up a buffered output queue such as I2S (see ESP32 / ESP8266 for examples for this setup)
#if (EXTERNAL_AUDIO_OUTPUT != true)
  // remember that the user may configure EXTERNAL_AUDIO_OUTPUT, in which case, you'll want to provide step 2a), and only that.
    
  pinMode(AUDIO_CHANNEL_1_PIN, OUTPUT);
    
  PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTB_gc; // Enable "alternative" pin output PORTB[5:0] for TCA0
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm;  // SysClk and enabled
  TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP0EN_bm | TCA_SINGLE_WGMODE_DSTOP_gc;
  TCA0.SINGLE.CTRLD = 0; // Normal (Single) mode (not split)
  TCA0.SINGLE.PER = 244; // 32768 Hz "tick"
  TCA0.SINGLE.CMP0 = 0;

  TCA0.SINGLE.INTCTRL |= TCA_SINGLE_OVF_bm;  // Turn on interrupts
#endif
}

void stopMozzi() {
  // Add here code to pause whatever mechanism moves audio samples to the output
}
////// END audio output code //////
