// is one polymorphism a monomorphism?
// would it be cooler in 2024 to assign lambdas to a struct in the global
// initializer?
#pragma once

struct state_t {
  smart_string_pointer_t name;
  virtual void start() {}
  virtual void loop() {}
  virtual void refresh(char *line_text) {
    stpncpy(line_text, name, lcd.max_str);
  }
  enum {
    START = 1,
    LOOP,
    DONE,
    RETURN_TO_MAIN_MENU,
    ITEM_ONLY,
  };
  int state;
};

struct state_machine_t {
  stack_t<state_t *, 16> _stack;

  state_machine_t() {}

  void push_state(state_t &s) {
    if (_stack.full()) {
      return error(F("full state_machine stack"));
    }
    s.state = state_t::START;
    _stack.push(&s);
  }

  void push_state_under(state_t &s) {
    if (_stack.full()) {
      return error(F("full state_machine stack"));
    }

    // hack?
    // when top state is pushing and will exit
    state_t *t = _stack.top();
    _stack.pop();
    push_state(s);
    _stack.push(t);
  }

  void loop() {
    if (_stack.empty()) {
      error(F("empty state_machine stack"));
    }

    // don't machine out of the main menu
    auto at_main_menu = [&]() {
      return _stack.size() == 1;
    };

    state_t &t = *_stack.top();
    switch (t.state) {

    case state_t::START:
      t.start();
      t.state = state_t::LOOP;
      break;

    case state_t::LOOP:
      t.loop();
      if (button_back.pushed() && !at_main_menu()) {
        // in this software, the back button always works
        t.state = state_t::DONE;
      }
      break;

    case state_t::DONE:
      if (!at_main_menu()) {
        _stack.pop();
      }
      break;

    case state_t::RETURN_TO_MAIN_MENU:
      while (!at_main_menu()) {
        _stack.pop();
      }
      break;
    }
  }
};

state_machine_t state_machine;
