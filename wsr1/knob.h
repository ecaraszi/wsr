// KY-040 rotary encoder
//
// interrupt handling by:
// https://www.pjrc.com/teensy/td_libs_Encoder.html
//
// with this library and my encoders, a 'detent' moves position by 4
#pragma once

#define ENCODER_USE_INTERRUPTS
#include <Encoder.h>

struct knob_t {
  struct pins_t {
    int8_t clk;
    int8_t dt;
  };

  Encoder enc;
  int16_t enc_position = 0;
  int16_t position = 0;
  int16_t diff = 0;

  explicit knob_t(pins_t pins) : enc(pins.clk, pins.dt) {}

  void once() {}

  void loop() {
    int ep = enc.read();
    diff = 0;
    if (ep != enc_position) {
      enc_position = ep;
      int np = enc_position / 4;
      diff = np - position;
      position = np;
    }
  }
};
