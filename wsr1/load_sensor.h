// HX711 amplified load cells on a 3 inch bar of aluminium
#pragma once

#include <HX711.h>

struct load_sensor_t {
  struct pins_t {
    int8_t dt;
    int8_t sck;
  } pins;
  HX711 sensor;

  queue_t<int32_t, 4> samples;
  int32_t offset = 0;
  int8_t dir = 0;
  int16_t force = 0;

  int16_t neutral_force = 600;
  int16_t stop_force = 1200;

  explicit load_sensor_t(pins_t pins_) : pins(pins_) {}

  void once() {
    sensor.begin(pins.dt, pins.sck, /* gain 32 B, 64 A, 128 A */ 128);

    samples.clear();
    offset = 0;
    while (!samples.full()) {
      read();
    }
    set_tare();
  }

  void set_tare() { offset = samples.mean_value(); }

  void loop() {
    if (sensor.is_ready()) {
      read();
    }
  }

  void read() {
    // drop 24 bit value to 16 bits
    int32_t reading = sensor.read() >> 8;
    samples.push(reading);

    force = ((samples[-1] + samples[-2]) / 2) - offset;

    dir = 1;
    if (force < 0) {
      dir = -1;
      force = -force;
    }
  }
};
