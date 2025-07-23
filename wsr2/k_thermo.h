// k-type thermocouple
// https://www.analog.com/media/en/technical-documentation/data-sheets/MAX6675.pdf
#pragma once

struct k_thermo_t {
  struct pins_t {
    int cs;
    int sck;
    int so;
  } pins;

  int temp_f = 0;
  queue_t<int, 4> samples;
  uint read_ms = 0;
  int stale_minutes = 0;

  // rev 1 of the hardware crosstalked with the stepper.
  // rev 2 has shielded stepper cable, I still suspect this
  bool disconnected = false;

  static constexpr int read_period_ms = 4000; // 4 seconds

  explicit k_thermo_t(pins_t pins_) : pins(pins_) {}

  void once() {
    pinMode(pins.cs, OUTPUT);
    pinMode(pins.sck, OUTPUT);
    pinMode(pins.so, INPUT);

    digitalWrite(pins.cs, HIGH); // chip not selected
  }

  void loop() {
    stale_minutes = min(99, stale_minutes + clock.elapsed_min);

    if (disconnected) {
      return;
    }

    if (clock.since_ms(read_ms, read_period_ms)) {
      int r = read();

      if (r < 0) {
        error(F("k_t disconnect"));
        return;
      }

      if (r == 0) {
        error(F("k_t zero"));
        return;
      }

      // discard implausible values, degrees F
      if (r < 10) {
        error(F("k_t low"));
        return;
      }
      if (r > 800) {
        error(F("k_t high"));
        return;
      }

      samples.push(r);
      if (samples.full()) {
        temp_f = samples.mean_value();
        stale_minutes = 0;
      }
    }
  }

  int read() {
    // digitalWrite(LED_BUILTIN, HIGH);

    uint v = 0;

    noInterrupts();

    digitalWrite(pins.cs, LOW);
    delayMicroseconds(10);

    for (int i = 15; i >= 0; i--) {
      digitalWrite(pins.sck, LOW);
      delayMicroseconds(10);
      if (digitalRead(pins.so)) {
        v |= (uint(1) << i);
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
    v &= uint(0xFFF);

    if (v == 0) {
      // this was probably the loose connection in wsr1
      return 0;
    }

    // the value is at 0.25 C resolution
    v /= 4;

    // convert to Farenheit, in America, in integers
    int32_t f = v;
    f *= int32_t(9);
    f /= int32_t(5);
    f += 32;

    return f;
  }
};

// Wsr1 only closed the air valve.

// If wsr2 is going to open the air valve,
// EGT measurement is critical,

// The thermocouple happened to be the
// part of wsr1 that failed first, although
// it was likely a badly crimped wire
// that could have been any wire.

// Therefore, wsr2 has dual thermocouples
// and dual max6675s, for corroborating
// testimony.

struct k_thermo_dual_t {
  k_thermo_t a;
  k_thermo_t b;
  int temp_f = 0;
  int stale_minutes = 0;

  k_thermo_dual_t(k_thermo_t::pins_t a_, k_thermo_t::pins_t b_) : a(a_), b(b_) {
    // bias the read clock on b so they alternate
    b.read_ms += k_thermo_t::read_period_ms / 2;
  }

  void once() {
    a.once();
    b.once();
  }

  void set_disconnect(bool d) {
    a.disconnected = d;
    b.disconnected = d;
  }

  void loop() {
    a.loop();
    b.loop();

    if (a.temp_f != 0 && b.temp_f != 0) {
      int diff = a.temp_f - b.temp_f;
      if (diff < 0) {
        diff = -diff;
      }
      if (diff >= 20) {
        temp_f = max(a.temp_f, b.temp_f);
        error(F("k_t diff 20"));
      } else {
        temp_f = (a.temp_f + b.temp_f) / 2;
      }
    }

    stale_minutes = a.stale_minutes + b.stale_minutes;
  }
};