// SunFounder IIC I2C/TWI Serial 2004 20x4 LCD display module
#pragma once

#include <Wire.h>

// https://github.com/johnrickman/LiquidCrystal_I2C/blob/master/LiquidCrystal_I2C.h
#include <LiquidCrystal_I2C.h>

struct lcd_t {
  struct pins_t {
    int backlight_pwm_out;
    int ambient_light_pwm_in;
  } pins;

  static constexpr int address = 0x27;
  static constexpr int columns = 20;
  static constexpr int rows = 4;
  static constexpr int max_str =
      32; // columns, plus some lucky padding for itoa()

  LiquidCrystal_I2C _lcd;

  char _text[rows][max_str];
  print_head_t _heads[rows];

  uint idle_sec = 0;

  static constexpr uint ambient_read_period_ms = 5000;
  uint ambient_read_ms = 0;
  queue_t<uint, 16> ambient_samples; // 5 seconds * 16 samples == 80 seconds
  uint ambient_low = 0;
  uint ambient_high = 15;
  uint ambient = 0;

  uint backlight_off_sec = 120;
  uint backlight_low_percent = 4;
  uint backlight_high_percent = 40;
  bool backlight_on = true;

  lcd_t(pins_t pins_) : pins(pins_), _lcd(address, columns, rows) {}

  void once() {
    _lcd.init();
    _lcd.clear();
    _lcd.backlight();

    pinMode(pins.backlight_pwm_out, OUTPUT);
    pinMode(pins.ambient_light_pwm_in, INPUT);
    analogWrite(pins.backlight_pwm_out, 128);  // boot at 50% brightness

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

  void keep_lit() {
    idle_sec = 0;
  }

  void loop_backlight() {

    // backlight idle off / on entirely
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

    // backlight intensity based on ambient light sensor
    if (clock.since_ms(ambient_read_ms, ambient_read_period_ms)) {
      ambient = analogRead(pins.ambient_light_pwm_in);
      ambient_samples.push(ambient);

      uint backlight_percent =
          backlight_low_percent +
          (((backlight_high_percent - backlight_low_percent) *
            (ambient_samples.mean_value() - ambient_low)) /
           (ambient_high - ambient_low));
      analogWrite(pins.backlight_pwm_out,
                  (backlight_percent * uint(255)) / 100);
    }
  }

  void loop() {
    for (int r = 0; r < rows; r++) {
      _heads[r].fill_with_space();
      if (_heads[r].dirty) {
        // digitalWrite(LED_BUILTIN, HIGH);
        _lcd.setCursor(0, r);
        _lcd.print(_text[r]);
        _heads[r].dirty = false;
        // digitalWrite(LED_BUILTIN, LOW);
      }
    }

    loop_backlight();
  }

  print_head_t &print(int row) {
    _heads[row].start(_text[row], columns);
    return _heads[row];
  }
};
