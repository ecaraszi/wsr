#pragma once

struct clock_t {
  uint32_t _ms = 0;
  uint16_t elapsed_ms = 0;
  uint16_t elapsed_sec = 0;
  uint16_t elapsed_min = 0;

  uint16_t _second_ms = 0;
  uint16_t seconds = 0;
  uint16_t minutes = 0;

  // filter out slow frames if they occur
  static constexpr uint16_t max_elapsed_ms = 100;

  void once() {}

  void loop() {
    uint32_t now = millis();

    if (now < _ms) {
      // overflow
      // uint32_t elapsed_untestable? = (int32_t(0xFFFFFFFF) - ms) + now;
      elapsed_ms = min(max_elapsed_ms, /* estimate */ now);
    } else {
      elapsed_ms = min(max_elapsed_ms, uint16_t(now - _ms));
    }

    _ms = now;

    elapsed_sec = 0;
    elapsed_min = 0;
    
    _second_ms += elapsed_ms;
    if (_second_ms >= 1000) {
      seconds++;
      _second_ms -= 1000;
      elapsed_sec = 1;

      if (seconds >= 60) {
        minutes++;
        seconds -= 60;
        elapsed_min = 1;
      }
    }
  }
};

clock_t clock;
