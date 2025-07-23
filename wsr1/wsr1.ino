
#include <Arduino.h>

#include "clock.h"
#include "print.h"
#include "queue.h"
#include "stack.h"

#include "button.h"
#include "knob.h"

knob_t knob({
    .clk = 0,
    .dt = 1,
});
button_t button_knob({.pin = 17});
button_t button_back({.pin = 10});
button_t button_wood({.pin = 11});
button_t button_yes({.pin = 12});

#include "k_thermo.h"
#include "lcd.h"
#include "load_sensor.h"

k_thermo_t k_thermo({
    .sck = 9,
    .cs = 8,
    .so = 7,
});
lcd_t lcd; // I2C
load_sensor_t load_sensor({
    .dt = 15,
    .sck = 14,
});

#include "neopixel_strip.h"
neopixel_strip_t led_strip({
    .pin = 16,
    .length = 8,
});

#include "stepper.h"
stepper_t stepper({
    .dir = 6,
    .step = 18,
    .sleep = 19,
    .reset = 20,
    .ms1 = 23,
    .ms2 = 22,
    .ms3 = 21,
});

#include "valve_actuator.h"

#include "state_machine.h"

#include "menu.h"
#include "readout.h"
#include "trigger.h"

#include "auto_burn.h"
#include "calibrate.h"
#include "manual_burn.h"
#include "settings.h"

#include "main_menu.h"

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  clock.once();
  
  knob.once();
  button_knob.once();
  button_back.once();
  button_wood.once();
  button_yes.once();

  lcd.once();
  k_thermo.once();
  load_sensor.once();

  led_strip.once();

  stepper.once();
  valve.once();

  state_machine.push_state(&main_menu);
}

void loop() {
  clock.loop();
  knob.loop();
  button_knob.loop();
  button_back.loop();
  button_wood.loop();
  button_yes.loop();

  lcd.loop();
  k_thermo.loop();
  load_sensor.loop();
 
  led_strip.loop();

  stepper.loop();
  valve.loop();

  if (state_machine._stack.size() == 1 && button_wood.pushed()) {
    state_machine.push_state(&auto_burn);
  }

  state_machine.loop();
}
