#pragma once

struct valve_actuator_t {

  // WSR2 Approximate Numerology:
  // Total travel of the/my wood stove air valve is 2" / 5cm / 50mm.
  // 50mm / 8mm T8 nut travel per rotation is 6 full rotations.
  // 200 full steps per rotation stepper motor, 1200 full steps total travel.

  // See stepper_tmc5160.h for step hz, microstepping, and RPM.

  static constexpr int total_travel_fullsteps = 1200;

  int open_steps;
  int closed_steps;
  int percent;

  enum internal_state {
    // other code is moving the stepper
    DISCONNECTED = 0xF,
    // valve_actuator_t is moving the stepper
    ADJUST = 0,
    FIND_OPEN = 1,    // also "the open direction of steps"
    FIND_CLOSED = -1, // also "the closed direction of steps"
  };
  int state;

  void once() { reset(); }

  void reset() {
    open_steps = (total_travel_fullsteps / 2) * FIND_OPEN;
    closed_steps = (total_travel_fullsteps / 2) * FIND_CLOSED;
    percent = 50;
    state = DISCONNECTED;
  }

  void normalize_steps() {
    // move the range so closed_steps is zero
    long total_steps = open_steps - closed_steps;
    long p = percent;
    constexpr long cent = 100;
    long steps = (total_steps * p) / cent;

    closed_steps = 0;
    open_steps = total_steps;
    stepper.stop_at(steps);
  }

  void disconnect() {
    state = DISCONNECTED;
    stepper.stop();

    // wherever we stopped,
    // read back the steps into the percent.
    long total_steps = open_steps - closed_steps;
    long half_a_percent = total_steps / 200;
    long steps = stepper.fullsteps();
    long base = closed_steps;
    constexpr long cent = 100;

    long pc = ((steps + half_a_percent - base) * cent) / total_steps;
    percent = pc;
  }

  void loop() {
    if (state == ADJUST) {
      int total_steps = open_steps - closed_steps;
      int target_steps =
          closed_steps +
          ((int32_t(total_steps) * int32_t(percent)) / int32_t(100));
      stepper.move_to(target_steps);
    }

    if (state != DISCONNECTED && stepper.sg_stall_recovered) {
      if (state == ADJUST) {
        error(F("v_a aj stall"));
        stepper.sg_stall_recovered = false;
      }

      auto set_limit = [&]() {
        normalize_steps();
        state = ADJUST;
        stepper.sg_stall_recovered = false;
      };
      if (state == FIND_OPEN) {
        open_steps = stepper.fullsteps();
        set_limit();
      }
      if (state == FIND_CLOSED) {
        closed_steps = stepper.fullsteps();
        set_limit();
      }
    }
  }

  void set_percent_air(int pct) {
    if (state == DISCONNECTED) {
      // UI that calls this is active, QED.
      state = ADJUST;
    }

    if (percent != pct && state == ADJUST) {
      auto move_toward_limit = [&]() {
        stepper.sg_stall_recovered = false;
        stepper.move_delta(state /* dir */ * total_travel_fullsteps *
                           2 /* far */);
      };
      if (pct <= 0) {
        state = FIND_CLOSED;
        move_toward_limit();
      } else if (pct >= 100) {
        state = FIND_OPEN;
        move_toward_limit();
      }

      percent = max(0, min(100, pct));
    }
  }
};

valve_actuator_t valve;
