#pragma once

struct manual_burn_t : public state_t {

  manual_burn_t() { name = F("Manual Burn"); }

  virtual void loop() override {
    lcd.print(0).s_centered(name);
    lcd.print(1)
        .s(F("EGT: "))
        .i(k_thermo.temp_f, 3)
        .s(F("  Air: "))
        .i(valve.percent, 4)
        .s(F("%"));
    valve.print();

    valve.set_percent_air(valve.percent + knob.diff);
  }

};

manual_burn_t manual_burn;
