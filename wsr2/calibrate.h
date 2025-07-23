#pragma once

constexpr int read_screen_ms = 500;

void push_calibrate_all_states();

struct calibrate_all_t : public state_t {
  uint hold_ms;

  calibrate_all_t() { name = F("Calibrate All"); }

  virtual void start() override { hold_ms = 0; }

  virtual void loop() override {
    if (clock.since_ms(hold_ms, read_screen_ms)) {
      push_calibrate_all_states();
      state = state_t::DONE;
    }

    lcd.print(0).clear();
    lcd.print(1).s_centered(name);
    lcd.print(2).clear();
    lcd.print(3).clear();

    lcd.keep_lit();
  }
};

calibrate_all_t calibrate_all;

struct free_jog_t : public state_t {
  free_jog_t() { name = F("Free Jog Stepper"); }

  virtual void start() override {
    // disconnect valve actuator so the knob can jog the stepper
    valve.disconnect();
  }

  virtual void loop() override {
    lcd.print(0).s_centered(name);
    lcd.print(1).clear();
    stepper.print();

    lcd.keep_lit();
    stepper.move_delta(15 * knob.diff);
  }
};

free_jog_t free_jog;

struct record_calibration_t : public state_t {
  int steps;

  virtual void loop() override {
    steps = valve.open_steps - valve.closed_steps;
    state = state_t::DONE;
  }
};

record_calibration_t record1;
record_calibration_t record2;

struct evaluate_calibration_t : public state_t {
  evaluate_calibration_t() { name = F("Evaluate Calibration"); }

  bool is_good() {
    int diff = record1.steps - record2.steps;
    if (diff < 0) {
      diff = -diff;
    }

    // expected total travel in steps
    // see also valve_actuator_t
    constexpr int min = 1100;
    constexpr int max = 1500;

    return diff < 20 && record1.steps > min && record1.steps < max &&
           record2.steps > min && record2.steps < max;
  }

  virtual void loop() override {
    lcd.print(0).s_centered(name);

    lcd.print(1).s(F("Steps ")).i(record1.steps).s(F(", ")).i(record2.steps);

    if (is_good()) {
      lcd.print(2).s_centered(F("GOOD"));
    } else {
      lcd.print(2).s_centered(F("BAD"));
    }

    lcd.print(3).s(F("Try Again")).s_right(F("OK"));

    if (button_back.pushed()) {
      // Try Again
      state_machine.push_state_under(calibrate_all);
      state = DONE;
    }

    if (button_yes.pushed()) {
      // OK
      state = RETURN_TO_MAIN_MENU;
    }

    lcd.keep_lit();
  }
};

evaluate_calibration_t evaluate_calibration;

struct sense_limit_t : public state_t {
  int percent;
  uint hold_ms;

  sense_limit_t(smart_string_pointer_t name_, int percent_) {
    name = name_;
    percent = percent_;
  }

  virtual void start() override {
    // Special 101 or -1 values of percent will drive
    // the stepper to one limit or the other by
    // valve_actuator_t.
    valve.set_percent_air(percent);
    hold_ms = 0;
  }

  virtual void loop() override {
    if (valve.state == valve_actuator_t::ADJUST && stepper.at_rest()) {
      // presumably valve_actuator_t and the stepper went
      // through a stall cycle and back to adjust/idle.
      if (clock.since_ms(hold_ms, read_screen_ms)) {
        state = state_t::DONE;
      }
    }

    lcd.print(0).s_centered(name);
    lcd.print(1).clear();
    stepper.print();
    lcd.keep_lit();
  }
};

sense_limit_t sense_open1(F("Sense Open 1 of 2"), 101);
sense_limit_t sense_open2(F("Sense Open 2 of 2"), 101);
sense_limit_t sense_closed1(F("Sense Closed 1 of 2"), -1);
sense_limit_t sense_closed2(F("Sense Closed 2 of 2"), -1);

struct reset_all_t : public state_t {
  uint hold_ms;

  reset_all_t() { name = F("Reset All"); }

  virtual void start() override {
    stepper.stop_at(0);
    valve.reset();
    hold_ms = 0;
  }

  virtual void loop() override {
    if (stepper.at_rest()) {
      if (clock.since_ms(hold_ms, read_screen_ms)) {
        state = state_t::DONE;
      }
    }

    lcd.print(0).clear();
    lcd.print(1).s_centered(name);
    lcd.print(2).clear();
    lcd.print(3).clear();
    lcd.keep_lit();
  }
};

reset_all_t reset_all;

void push_calibrate_all_states() {
  state_machine.push_state_under(evaluate_calibration);
  state_machine.push_state_under(record2);
  state_machine.push_state_under(sense_closed2);
  state_machine.push_state_under(sense_open2);
  state_machine.push_state_under(record1);
  state_machine.push_state_under(sense_closed1);
  state_machine.push_state_under(sense_open1);
  state_machine.push_state_under(reset_all);
}

// clang-format off
state_t *calibration_items[] = {
    &calibrate_all,
    &free_jog,
    nullptr,
};
// clang-format on

menu_t calibration_menu(F("Calibration"), calibration_items);