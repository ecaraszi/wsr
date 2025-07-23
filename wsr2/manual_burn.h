#pragma once

struct manual_burn_t : public state_t {

  manual_burn_t() { name = F("Manual Burn"); }

  virtual void loop() override {
    lcd.print(0).s_centered(name);
    lcd.print(1)
        .s(F("T "))
        .i(k_thermo.a.temp_f, 3)
        .s(F("/"))
        .i(k_thermo.b.temp_f, 3)
        .s(F("  A "))
        .i(valve.percent, 3)
        .s(F("%"));
    stepper.print();

    int clipped = max(0, min(100, valve.percent + knob.diff));
    valve.set_percent_air(clipped);
  }

};

manual_burn_t manual_burn;
