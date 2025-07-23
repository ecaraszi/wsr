// A4988 stepper motor control
// https://www.pololu.com/product/1182
#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcpp"  // doesn't actually work, hmm
#define USE_TIMER_1 true
#define USE_TIMER_2 false
#define USE_TIMER_3 false
#define USE_TIMER_4 false
#define USE_TIMER_5 false
#include <TimerInterrupt.h>
#pragma GCC diagnostic pop

struct stepper_t;
void step_interrupt(stepper_t *s);

struct stepper_t {
  struct pins_t {
    int8_t dir;
    int8_t step;
    int8_t sleep;
    int8_t reset;
    int8_t ms1;
    int8_t ms2;
    int8_t ms3;
  } pins;

  // sleeping the motor quiets noise and stops interrupts
  // stove air valve won't move itself
  bool sleeping;
  int16_t idle_ms;

  volatile int16_t steps;
  volatile int16_t dir;
  volatile int16_t parity;
  int16_t target;

  stepper_t(pins_t pins_) : pins(pins_) { reset(); }

  void reset() {
    steps = 0;
    dir = 0;
    parity = 0;
    target = 0;
    idle_ms = 0;
    sleep();
  }

  // static constexpr int16_t step_scalar = 16;
  // static constexpr int16_t step_ms = 2;
  // void set_microsteps() {
  //   // small steps are quieter
  //   digitalWrite(pins.ms1, HIGH);
  //   digitalWrite(pins.ms2, HIGH);
  //   digitalWrite(pins.ms3, HIGH);
  // }

  static constexpr int16_t step_scalar = 8;
  static constexpr int16_t step_ms = 2;
  void set_microsteps() {
    // small steps are quieter
    digitalWrite(pins.ms1, HIGH);
    digitalWrite(pins.ms2, HIGH);
    digitalWrite(pins.ms3, LOW);
  }

  // static constexpr int16_t step_scalar = 4;
  // static constexpr int16_t step_ms = 16;
  // void set_microsteps() {
  //   // small steps are quieter
  //   digitalWrite(pins.ms1, LOW);
  //   digitalWrite(pins.ms2, HIGH);
  //   digitalWrite(pins.ms3, LOW);
  // }

  void once() {
    pinMode(pins.dir, OUTPUT);
    pinMode(pins.step, OUTPUT);
    pinMode(pins.sleep, OUTPUT);
    pinMode(pins.reset, OUTPUT);
    pinMode(pins.ms1, OUTPUT);
    pinMode(pins.ms2, OUTPUT);
    pinMode(pins.ms3, OUTPUT);
    // pinMode(pins.enable, OUTPUT);

    set_microsteps();

    digitalWrite(pins.sleep, LOW);
    digitalWrite(pins.reset, HIGH);
    // digitalWrite(pins.enable, LOW);
    sleeping = true;

    ITimer1.init();
  }

  void sleep() {
    ITimer1.detachInterrupt();
    digitalWrite(pins.sleep, LOW);
    k_thermo.sleeping = false;
    // load_sensor.sensor.power_down();
    sleeping = true;
  }

  void wake() {
    digitalWrite(pins.sleep, HIGH);
    delayMicroseconds(1000);
    dir = 0;
    parity = 0;
    k_thermo.sleeping = true;
    // load_sensor.sensor.power_up();
    ITimer1.attachInterruptInterval(step_ms / 2, step_interrupt, this);
    sleeping = false;
  }

  void loop() {
    if (steps == target) {
      idle_ms = min(30000, idle_ms + clock.elapsed_ms);
      if (!sleeping && idle_ms > 2000) {
        sleep();
      }
    } else { // steps != target
      idle_ms = 0;
      if (sleeping) {
        wake();
      }
    }
  }

  void move_to(int16_t target_) { target = target_; }
  void stop() { target = steps; }
  bool idle() { return target == steps; }
};

void step_interrupt(stepper_t *ps) {
  stepper_t &s = *ps;
  int16_t steps = s.target - s.steps;
  int16_t dir = 0;

  if (steps < 0) {
    dir = -1;
    steps = -steps;
  } else if (steps > 0) {
    dir = 1;
  }

  if (dir != s.dir) {
    if (dir < 0) {
      digitalWrite(s.pins.dir, LOW);
    } else if (dir > 0) {
      digitalWrite(s.pins.dir, HIGH);
    }
    s.dir = dir;
  }

  if (s.dir != 0) {
    if (s.parity == 0) {
      digitalWrite(s.pins.step, LOW);
      s.parity = 1;
    } else {
      digitalWrite(s.pins.step, HIGH);
      s.steps += s.dir;
      s.parity = 0;
    }
  }
}

// story:
// pin 13 on the micro shared with the onboard LED,
// so 'high' was '4.90v', which to the A4988 dir line was 'low'
