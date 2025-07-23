#pragma once 

struct int_readout_t : public state_t {
  int16_t* pvalue;
  int16_t undo;
  int16_t increment;

  int_readout_t(smart_string_pointer_t name_, int16_t* pvalue_) {
    name = name_;
    pvalue = pvalue_;
  }

  virtual void start() override {
    undo = *pvalue;
    increment = 1;
  }

  virtual void loop() override {
    *pvalue = *pvalue + (knob.diff * increment);

    lcd.print(0).s(name).i_right(*pvalue);
    lcd.print(1).clear();
    lcd.print(2).s_centered(F("Adjust by"));
    lcd.print(3).s(F("No")).i(increment, 9).s_right(F("Yes"));

    if (button_back.pushed()) {
      *pvalue = undo;
      done = true;
    }

    if (button_yes.pushed() || button_knob.pushed()) {
      done = true;
    }

    if (button_wood.pushed()) {
      increment *= 10;
      if (increment > 100) {
        increment = 1;
      }
    }
  }

  virtual void refresh(char* line_text) override {
    print_head_t p;
    p.start(line_text, lcd.columns);
    p.s(name).s(": ").i(*pvalue);
    p.null_terminate();
  }
};

