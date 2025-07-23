#pragma once

struct trigger_t : public state_t {
  int16_t hold_ms;

  virtual void start() override {
    hold_ms = 1000;
  }

  void tick() {
    hold_ms -= clock.elapsed_ms;
    done = hold_ms < 0;
  }

  virtual void loop() override {
    lcd.print(0).clear();
    lcd.print(1).s_centered(name);
    lcd.print(2).clear();
    lcd.print(3).clear();

    tick();
  }

};