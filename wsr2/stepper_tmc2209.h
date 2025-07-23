// mostly draft, this is as far as I got before realizing 
// the PCB UART wiring wasn't as single wire as it should have been
#pragma once

#include <TMCStepper.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcpp" // doesn't actually work, hmm
#define USE_TIMER_1 true
#define USE_TIMER_2 false
#define USE_TIMER_3 false
#define USE_TIMER_4 false
#define USE_TIMER_5 false
#include <TimerInterrupt.h>
#pragma GCC diagnostic pop

struct stepper_tmc2209_t;
void step_interrupt(stepper_tmc2209_t *s);

struct stepper_tmc2209_t {
  struct pins_t {
    int dir;
    int step;
    int ms1;
    int ms2;
    int rx;
    int tx;
    int index;
    int diag;
    int en;
  } pins;

  // "110mOhm" from
  // https://biqu.equipment/products/bigtreetech-tmc2209-stepper-motor-driver-for-3d-printer-board-vs-tmc2208
  static constexpr float r_sense = 0.11f;

  // because ms1 and ms2 are set low
  static constexpr uint driver_address = 0b00;

  // sleeping stops interrupts
  // used to be sleeping was quiet for A4988 motor controller
  // TMC2209 is quiet itself
  bool sleeping = true;
  int idle_ms = 0;

  // interrupt heart rate
  static constexpr int timer_ms = 2;

  // steps
  static constexpr uint microsteps = 2;

  volatile int dir = 0;
  volatile int parity = 0;
  volatile int steps = 0;
  int target = 0; // target steps

  // stallguard
  static constexpr uint stall_value = 10;
  uint stallguard_read_ms = 0;
  uint stallguard_result = 0;
  uint cs_rms = 0;
  uint cs_actual = 0;

  TMC2209Stepper driver;

  stepper_tmc2209_t(pins_t pins_)
      : pins(pins_), driver(pins.rx, pins.tx /*&Serial1*/, r_sense, driver_address) {
    sleep();
  }

  void once() {
    pinMode(pins.dir, OUTPUT);
    pinMode(pins.step, OUTPUT);

    pinMode(pins.ms1, OUTPUT);
    pinMode(pins.ms2, OUTPUT);
    digitalWrite(pins.ms1, LOW);
    digitalWrite(pins.ms2, LOW);

    pinMode(pins.en, OUTPUT);
    digitalWrite(pins.en, LOW);

    //Serial1.begin(115200);

    driver.begin();

    driver.toff(4);
    driver.blank_time(24);
    driver.rms_current(800);
    driver.microsteps(microsteps);

    // stealthChop
    // driver.pwm_autoscale(true);

    // stallguard
    driver.TCOOLTHRS(0xFFFFF); // 20bit max
    driver.semin(5);
    driver.semax(2);
    driver.sedn(0b01);
    driver.SGTHRS(stall_value);

    sleeping = true;

    ITimer1.init();
  }

  void sleep() {
    ITimer1.detachInterrupt();
    // k_thermo.sleeping = false;
    // // load_sensor.sensor.power_down();
    sleeping = true;
  }

  void wake() {
    dir = 0;
    parity = 0;
    // k_thermo.sleeping = true;
    // // load_sensor.sensor.power_up();
    ITimer1.attachInterruptInterval(timer_ms, step_interrupt, this);
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

    if (clock.since_ms(stallguard_read_ms, 200 /* 5x second */)) {
      stallguard_result = driver.SG_RESULT();
      cs_actual = driver.cs_actual();
      cs_rms = driver.cs2rms(cs_actual);
    }
  }

  void move_to(int target_) { target = target_; }
  void stop() { target = steps; }
  bool idle() { return target == steps; }

  void print() {
    lcd.print(2)
        .s(F("S "))
        .i(steps, 7)
        .s(F(" T "))
        .i(target, 7);
    lcd.print(3)
        .s(F("SG "))
        .i(stallguard_result)
        .s(F(" C "))
        .i(cs_actual)
        .s(F(" R "))
        .i(cs_rms);
  }
};

void step_interrupt(stepper_tmc2209_t *ps) {
  stepper_tmc2209_t &s = *ps;
  int steps = s.target - s.steps;
  int dir = 0;

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
