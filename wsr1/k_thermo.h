// k-type thermocouple with max6675
#pragma once

struct k_thermo_t {
  struct pins_t {
    int8_t sck;
    int8_t cs;
    int8_t so;
  } pins;

  queue_t<int16_t, 4> samples;
  static constexpr int8_t period_sec = 4;
  int16_t temp_f = 0;
  int8_t wait_sec = 0;

  int8_t minutes_since_read = 0;

  // rev 1 of the hardware crosstalks with the stepper,
  // so don't read while stepper is awake.
  bool sleeping = false;

  explicit k_thermo_t(pins_t pins_) : pins(pins_) {}

  void once() {
    pinMode(pins.cs, OUTPUT);
    pinMode(pins.sck, OUTPUT);
    pinMode(pins.so, INPUT);

    digitalWrite(pins.cs, HIGH);
  }

  void loop() {
    bool b = false;
    if (sleeping) {
      wait_sec = period_sec;
    } else {
      wait_sec -= clock.elapsed_sec;
      if (wait_sec <= 0) {
        wait_sec = period_sec;
        b = true;
      }
    }

    minutes_since_read = min(99, minutes_since_read + clock.elapsed_min);

    if (b) {
      int16_t r = read();
      if (r >= 0) {
        samples.push(r);
        temp_f = samples.mean_value();
        minutes_since_read = 0;
      }
    }
  }

  int16_t read() {
    // digitalWrite(LED_BUILTIN, HIGH);

    // https://www.analog.com/media/en/technical-documentation/data-sheets/MAX6675.pdf
    uint16_t v = 0;

    noInterrupts();

    digitalWrite(pins.cs, LOW);
    delayMicroseconds(10);

    for (int i = 15; i >= 0; i--) {
      digitalWrite(pins.sck, LOW);
      delayMicroseconds(10);
      if (digitalRead(pins.so)) {
        v |= (uint16_t(1) << i);
      }
      digitalWrite(pins.sck, HIGH);
      delayMicroseconds(10);
    }

    digitalWrite(pins.cs, HIGH);

    interrupts();

    // digitalWrite(LED_BUILTIN, LOW);

    if (v & 0x4) {
      // thermocouple disconnected
      return -1;
    }

    // shift away the status bits
    v >>= 3;

    // mask down to the 12-bit value
    v &= uint16_t(0xFFF);

    if (v == 0) {
      // probably max6675 disconnected
      return -1;
    }

    // the value is at 0.25 C resolution
    v /= 4;

    // convert to Farenheit, in America, in integers
    int32_t fix = v;
    fix *= int32_t(9);
    fix /= int32_t(5);
    fix += 32;

    return fix;
  }
};