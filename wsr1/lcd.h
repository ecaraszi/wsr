// SunFounder IIC I2C/TWI Serial 2004 20x4 LCD display module
#pragma once

#include <Wire.h>

// https://github.com/johnrickman/LiquidCrystal_I2C/blob/master/LiquidCrystal_I2C.h
#include <LiquidCrystal_I2C.h>

struct lcd_t {
  static constexpr int8_t address = 0x27;
  static constexpr int8_t columns = 20;
  static constexpr int8_t rows = 4;
  static constexpr int8_t max_str =
      32; // columns, plus some lucky padding for itoa()

  LiquidCrystal_I2C _lcd;
  char _text[rows][max_str];
  print_head_t _heads[rows];
  uint16_t backlight_off_sec = 15;
  uint16_t idle_sec;
  bool backlight_on;

  lcd_t() : _lcd(address, columns, rows) {}

  void once() {
    _lcd.init();
    _lcd.clear();
    idle_sec = 0;
    _lcd.backlight();
    backlight_on = true;
    memset(_text, 0, sizeof(_text));
    memset(_heads, 0, sizeof(_heads));
  }

  bool any_input_consume() {
    bool in = knob.diff != 0 || button_knob.pushed() || button_back.pushed() ||
              button_wood.pushed() || button_yes.pushed();
    if (in) {
      knob.diff = 0;
    }
    return in;
  }

  bool any_input() {
    return knob.diff != 0 || button_knob._pushed || button_back._pushed ||
           button_wood._pushed || button_yes._pushed;
  }

  void idle_backlight() {
    if (!backlight_on) {
      if (any_input_consume()) {
        idle_sec = 0;
      }
      if (idle_sec == 0) {
        _lcd.backlight();
        backlight_on = true;
      }
    } else {
      idle_sec = min(idle_sec + clock.elapsed_sec, backlight_off_sec);
      if (any_input()) {
        idle_sec = 0;
      }
      if (idle_sec >= backlight_off_sec) {
        _lcd.noBacklight();
        backlight_on = false;
      }
    }
  }

  void loop() {
    for (int r = 0; r < rows; r++) {
      _heads[r].finish();
      if (_heads[r].dirty) {
        // digitalWrite(LED_BUILTIN, HIGH);
        _lcd.setCursor(0, r);
        _lcd.print(_text[r]);
        _heads[r].dirty = false;
        // digitalWrite(LED_BUILTIN, LOW);
      }
    }

    idle_backlight();
  }

  print_head_t &print(int row) {
    _heads[row].start(_text[row], columns);
    return _heads[row];
  }
};
