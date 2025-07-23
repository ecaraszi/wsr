#pragma once

constexpr int16_t full_rotation_steps_t8x8_rod_nema_17 =
    200 * stepper_t::step_scalar;
constexpr int16_t long_steps = full_rotation_steps_t8x8_rod_nema_17 / 15;
constexpr int16_t short_steps = full_rotation_steps_t8x8_rod_nema_17 / 40;

struct valve_actuator_t {

  int16_t open_steps;
  int16_t closed_steps;
  int8_t percent;

  enum {
    STOP,
    ADJUST,
    FIND_LIMIT,
    FIND_NEUTRAL,
  };
  int8_t mode;
  enum {
    DIR_STOP = 0,
    DIR_OPEN = -1,
    DIR_CLOSED = 1,
  };
  int8_t dir;

  void once() { reset(); }

  void reset() {
    open_steps = -1925 * stepper_t::step_scalar;
    closed_steps = 0 * stepper_t::step_scalar;
    percent = 0;
    mode = STOP;
    dir = DIR_STOP;
  }

  void normalize_steps() {
    int32_t total_steps = open_steps - closed_steps;
    closed_steps = 0;
    open_steps = total_steps;
    stepper.steps = stepper.target =
        (total_steps * int32_t(percent)) / int32_t(100);
  }

  void stop() {
    mode = STOP;
    stepper.stop();
    int32_t base_steps = closed_steps;
    int32_t total_steps = open_steps - closed_steps;
    int32_t middle_of_a_percent = total_steps / 200;
    percent = ((int32_t(stepper.steps) + middle_of_a_percent - base_steps) *
               int32_t(100)) /
              total_steps;
  }

  void loop() {
    if (mode == ADJUST) {
      int32_t base_steps = closed_steps;
      int32_t total_steps = open_steps - closed_steps;
      int32_t target_steps =
          base_steps + (total_steps * int32_t(percent)) / int32_t(100);
      stepper.move_to(target_steps);

      if (load_sensor.force > (load_sensor.stop_force * 2)) {
        stop();
      }
    }

    if (mode == FIND_LIMIT) {
      if (load_sensor.force > load_sensor.stop_force) {
        stepper.stop();
        mode = FIND_NEUTRAL;
      } else {
        if (load_sensor.force > load_sensor.neutral_force) {
          stepper.move_to(stepper.steps + (dir * short_steps));
        } else {
          stepper.move_to(stepper.steps + (dir * long_steps));
        }
      }
    }

    if (mode == FIND_NEUTRAL) {
      if (stepper.idle()) {
        if (load_sensor.force > load_sensor.neutral_force) {
          stepper.move_to(stepper.steps - (dir * short_steps));
        } else {
          if (dir == DIR_OPEN) {
            open_steps = stepper.steps;
          }
          if (dir == DIR_CLOSED) {
            closed_steps = stepper.steps;
          }
          normalize_steps();
          dir = DIR_STOP;
          mode = ADJUST;
        }
      }
    }
  }

  void set_percent_air(int8_t pct) {
    if (mode == STOP) {
      mode = ADJUST;
    }

    if (percent != pct && mode == ADJUST) {
      if (pct <= 0) { // find closed
        dir = DIR_CLOSED;
        mode = FIND_LIMIT;
      } else if (pct >= 100) { // find open
        dir = DIR_OPEN;
        mode = FIND_LIMIT;
      }
    }

    percent = max(0, min(100, pct));
  }

  void print() {
    lcd.print(2)
        .s(F("Step "))
        .i(closed_steps / stepper_t::step_scalar, 5)
        .i(stepper.steps / stepper_t::step_scalar, 5)
        .i(open_steps / stepper_t::step_scalar, 5);
    lcd.print(3)
        .s(F("Force "))
        .i(load_sensor.force * load_sensor.dir, 6)
        .i(load_sensor.samples.head(), 8);
  }
};

valve_actuator_t valve;
