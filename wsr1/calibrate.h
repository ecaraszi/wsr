#pragma once

struct sense_limit_t : public state_t {
  int8_t percent;

  sense_limit_t(smart_string_pointer_t name_, int8_t percent_) {
    name = name_;
    percent = percent_;
  }

  virtual void start() override { valve.set_percent_air(percent); }

  virtual void loop() override {
    lcd.print(0).s_centered(name);
    lcd.print(1).clear();
    valve.print();

    lcd.idle_sec = 0;

    if (valve.mode == valve_actuator_t::ADJUST) {
      done = true;
    }
  }
};

sense_limit_t sense_open(F("Sense Open"), 100);
sense_limit_t sense_closed(F("Sense Closed"), 0);

struct reset_all_t : public trigger_t {

  reset_all_t() { name = F("Reset All"); }

  virtual void start() override {
    stepper.reset();
    valve.reset();
    trigger_t::start();
  }
};

reset_all_t reset_all;

struct tare_t : public trigger_t {
  tare_t() { name = F("Tare Load Sensor"); }

  virtual void start() override {
    load_sensor.samples.clear();
    trigger_t::start();
  }

  virtual void loop() override {
    lcd.print(0).s_centered(name);
    lcd.print(1).i_centered(load_sensor.samples.size());
    valve.print();

    lcd.idle_sec = 0;

    if (load_sensor.samples.full()) {
      load_sensor.set_tare();
      trigger_t::tick();
    }
  }
};

tare_t tare;

struct calibrate_all_t : public state_t {
  calibrate_all_t() { name = F("Calibrate All"); }

  virtual void loop() override {
    lcd.print(0).s_centered(F("Put it in Neutral"));
    lcd.print(1).clear();
    lcd.print(2).clear();
    lcd.print(3).s(F("Back")).s_right(F("It is in N"));

    lcd.idle_sec = 0;

    if (button_yes.pushed()) {
      state_machine.queue_state(&sense_closed);
      state_machine.queue_state(&sense_open);
      state_machine.queue_state(&tare);
      state_machine.queue_state(&reset_all);
      done = true;
    }
  }
};

calibrate_all_t calibrate_all;

// clang-format off
state_t *calibration_items[] = {
    &calibrate_all,
    &reset_all,
    &tare,
    &sense_closed,
    &sense_open,
    nullptr,
};
// clang-format on

menu_t calibration_menu(F("Calibration"), calibration_items);