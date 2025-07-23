#pragma once

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

struct neopixel_strip_t {
  struct setup_t {
    int pin;
    int length;
  } setup;
  Adafruit_NeoPixel strip;

  bool lit;

  neopixel_strip_t(setup_t setup_)
      : setup(setup_), strip(setup_.length, setup_.pin, NEO_GRBW + NEO_KHZ800) {
  }

  void once() {
    strip.begin();
    for (int i = 0; i < setup.length; i++) {
      setColor(i, 0, 0, 0, 0);
    }
    strip.show();
    lit = false;
  }

  void loop() {
    if (lcd.backlight_on) {
      off();
    }
  }

  void off() {
    if (lit) {
      for (int i = 0; i < setup.length; i++) {
        setColor(i, 0, 0, 0, 0);
      }
      strip.show();
      lit = false;
    }
  }

  void yellow() {
    if (!lit) {
      for (int i = 0; i < setup.length; i++) {
        if (i & 1) {
          setColor(i, 50, 50, 0, 0);
        } else {
          setColor(i, 200, 200, 50, 0);
        }
      }
      strip.show();
      lit = true;
    }
  }

  void setColor(uint n, uint r, uint g, uint b, uint w) {
    strip.setPixelColor(n, r, g, b, w);
  }
};