// normally open switch to ground, pulled up by the arduino
#pragma once

struct button_t {
  struct pins_t {
    int8_t pin;
  } pins;
  bool _pushed = false;
  bool _double_pushed = false;
  int16_t down_ms = 0;
  int16_t up_ms = 0;

  explicit button_t(pins_t pins_) : pins(pins_) {}

  void once() { pinMode(pins.pin, INPUT_PULLUP); }

  void loop() {
    bool down = digitalRead(pins.pin) == 0;
    bool change_down = down && down_ms == 0;
    bool change_up = !down && up_ms == 0;

    auto tick = [](int16_t &count_ms, bool change) {
      if (change) {
        count_ms = max(1, /* estimate */ clock.elapsed_ms / 2);
      } else {
        count_ms = min(10000, count_ms + clock.elapsed_ms);
      }
    };

    _pushed = false;
    _double_pushed = false;

    bool down_not_jitter = up_ms > 100;
    if (down && change_down && down_not_jitter) {
      _pushed = true;
      if (up_ms < 500) {
        _double_pushed = true;
      }
    }

    if (down) {
      tick(down_ms, change_down);
      up_ms = 0;
    } else {
      tick(up_ms, change_up);
      down_ms = 0;
    }
  }

  bool pushed() {
    bool p = _pushed;
    _pushed = false;
    return p;
  }

  bool double_pushed() {
    bool p = _double_pushed;
    _double_pushed = false;
    return p;
  }
};
