#pragma once

struct clock_t {
  uint32_t _ms = 0;
  uint elapsed_ms = 0;
  uint elapsed_sec = 0;
  uint elapsed_min = 0;

  uint _second_ms = 0;
  uint seconds = 0;
  uint minutes = 0;

  // filter out slow frames if they occur
  // conform to general code expectations of a framerate
  static constexpr uint max_elapsed_ms = 66;

  void once() {}

  void loop() {
    uint32_t now = millis();

    if (now < _ms) {
      // overflow
      // I think it's this
      // elapsed_ms = (uint32_t(0xFFFFFFFF) - _ms) + now;
      // This is simpler, the discarded time interval is trivial,
      // especially considering max_elapsed_ms.
      elapsed_ms = min(max_elapsed_ms, /* estimate */ now);
    } else {
      elapsed_ms = min(max_elapsed_ms, uint(now - _ms));
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
        // if your wood stove has a burn time of > 18 hours,
        // then you'll need 32 bits.
        minutes = min(minutes + 1, 18u * 60u * 60u /* 64800 */);
        seconds -= 60;
        elapsed_min = 1;
      }
    }
  }


  [[nodiscard]] bool since(uint& var, uint elapsed, uint limit) {
    var = min(var + elapsed, limit);
    if (var >= limit) {
      var = 0;
      return true;
    } else {
      return false;
    }
  }

  [[nodiscard]] bool since_ms(uint& var, uint limit) {
    return since(var, elapsed_ms, limit);
  }

  [[nodiscard]] bool since_sec(uint& var, uint limit) {
    return since(var, elapsed_sec, limit);
  }

  [[nodiscard]] bool since_min(uint& var, uint limit) {
    return since(var, elapsed_min, limit);
  }
};

clock_t clock;
