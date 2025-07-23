// a short one
#pragma once

template <typename T, uint capacity> struct stack_t {
  T _stack[capacity];
  uint _end = 0;

  bool empty() { return _end == 0; }

  bool full() { return _end == capacity; }

  uint size() { return _end; }

  T top() { return _stack[_end - 1]; }

  void pop() {
    if (_end > 0) {
      _end--;
    }
  }

  void push(T t) {
    if (_end < capacity) {
      _stack[_end] = t;
      _end++;
    }
  }

  void clear() { _end = 0; }
};
