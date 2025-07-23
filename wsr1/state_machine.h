// is one polymorphism a monomorphism?
// would it be cooler in 2024 to assign lambdas to a struct in the global initializer? 
#pragma once

struct state_t {
  smart_string_pointer_t name;
  virtual void start() {}
  virtual void loop() {}
  virtual void refresh(char* line_text) {
    stpncpy(line_text, name, lcd.max_str);
  }
  bool started = false;
  bool done = false;
};

struct error_state_t : public state_t {
  virtual void loop() override {
    lcd.print(0).clear();
    lcd.print(1).s_centered(F("INTERNAL ERROR"));
    lcd.print(2).s_centered(name);
    lcd.print(3).clear();
  }
};

struct state_machine_t {
  stack_t<state_t *, 8> _stack;
  error_state_t state_error;

  state_machine_t() {}

  void push_state(state_t *s) {
    if (_stack.full()) {
      return error(F("full state_machine stack"));
    }

    s->started = false;
    s->done = false;
    _stack.push(s);
  }

  void queue_state(state_t *s) {
    if (_stack.full()) {
      return error(F("full state_machine stack"));
    }

    state_t *t = _stack.top();
    _stack.pop();
    push_state(s);
    _stack.push(t);
  }

  void loop() {
    if (_stack.empty()) {
      error(F("empty state_machine stack"));
    }

    state_t *t = _stack.top();
    if (!_stack.top()->started) {
      t->start();
      t->started = true;
    } else {
      t->loop();
    }

    t = _stack.top();
    bool back = false;
    if (t->done) {
      back = true;
    }
    if (button_back.pushed()) { // in this software, the back button always works
      back = true;
    }
    if (back && _stack.size() > 1 && t != &state_error) {
      _stack.pop();
    }
  }

  void error(smart_string_pointer_t str) {
    state_error.name = str;
    push_state(&state_error);
  }
};

state_machine_t state_machine;
