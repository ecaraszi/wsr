#pragma once

template <typename T, uint8_t capacity> struct queue_t {
  static_assert(capacity <= 32);

  T _queue[capacity];
  uint8_t _head = 0;
  uint8_t _tail = 0;
  uint8_t _size = 0;

  bool empty() { return _size == 0; }

  bool full() { return _size == capacity; }

  T head() { return _queue[(_head + capacity - 1) % capacity]; }

  T tail() { return _queue[_tail]; }

  const T &operator[](int8_t signed_index) const {
    uint8_t r;
    if (signed_index < 0) {
      r = _head;
      while (signed_index < 0) {
        r = (r + capacity - 1) % capacity;
        signed_index++;
      }
    } else {
      r = _tail;
      while (signed_index > 0) {
        r = (r + 1) % capacity;
        signed_index--;
      }
    }
    return _queue[r];
  }

  uint8_t size() { return _size; }

  void pop() {
    if (_size > 0) {
      _tail = (_tail + 1) % capacity;
      _size--;
    }
  }

  void push(T t) {
    _queue[_head] = t;
    _head = (_head + 1) % capacity;
    if (_size < capacity) {
      _size++;
    } else {
      _tail = (_tail + 1) % capacity;
    }
  }

  void clear() { _head = _tail = _size = 0; }

  T mean_value() {
    uint8_t tail = _tail;
    int32_t accum = 0;
    for (uint8_t i = 0; i < _size; i++) {
      accum += _queue[tail];
      tail = (tail + 1) % capacity;
    }
    return T(accum / int32_t(_size));
  }
};