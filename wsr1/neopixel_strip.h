#pragma once

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

struct neopixel_strip_t {
  struct setup_t {
    int8_t pin;
    int8_t length;
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
        }
      }
      strip.show();
      lit = true;
    }
  }

  void setColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    strip.setPixelColor(n, r, g, b, w);
  }
};