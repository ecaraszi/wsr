// or circular buffer
#pragma once

template <typename T, uint _capacity> struct queue_t {
  T _queue[_capacity];
  uint _head = 0;
  uint _tail = 0;
  uint _size = 0;

  bool empty() { return _size == 0; }

  bool full() { return _size == _capacity; }

  T head() { return _queue[(_head + _capacity - 1) % _capacity]; }

  T tail() { return _queue[_tail]; }

  uint size() { return _size; }

  uint capacity() { return _capacity; }

  // reading [0, size) displays the queue past to present
  // reading [-1, -size] queries the past  
  const T &operator[](int signed_index) const {
    uint r;
    if (signed_index < 0) {
      r = _head;
      while (signed_index < 0) {
        r = (r + _capacity - 1) % _capacity;
        signed_index++;
      }
    } else {
      r = _tail;
      while (signed_index > 0) {
        r = (r + 1) % _capacity;
        signed_index--;
      }
    }
    return _queue[r];
  }

  void pop() {
    if (_size > 0) {
      _tail = (_tail + 1) % _capacity;
      _size--;
    }
  }

  void push(T t) {
    _queue[_head] = t;
    _head = (_head + 1) % _capacity;
    if (_size < _capacity) {
      _size++;
    } else {
      _tail = (_tail + 1) % _capacity;
    }
  }

  void clear() { _head = _tail = _size = 0; }

  T mean_value() {
    uint tail = _tail;
    int32_t accum = 0;
    for (uint i = 0; i < _size; i++) {
      accum += _queue[tail];
      tail = (tail + 1) % _capacity;
    }
    return T(accum / int32_t(_size));
  }
};