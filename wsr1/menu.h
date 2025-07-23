#pragma once

struct menu_t : public state_t {
  state_t **items;

  int selected;
  int item_count;

  static constexpr int refresh_period_ms = 1000;

  menu_t(smart_string_pointer_t name_, state_t **items_) {
    name = name_;
    items = items_;
    for (item_count = 0; items[item_count]; item_count++)
      ;
  }

  virtual void start() override {
    selected = 0;
  }

  virtual void loop() override {
    char item_text[lcd.max_str];
    lcd.print(0).s_centered(name);

    selected = max(0, min(selected + knob.diff, item_count - 1));

    int pi = max(0, min(selected - 1, item_count - 3));
    for (int r = 1; r < 4; r++, pi++) {
      print_head_t &p = lcd.print(r);

      if (pi >= item_count) {
        p.clear();
        continue;
      }

      items[pi]->refresh(item_text);
      p.s(pi == selected ? "> " : "  ");
      p.s(item_text);
    }

    if (button_knob.pushed() || button_yes.pushed()) {
      state_machine.push_state(items[selected]);
    }
  }
};
