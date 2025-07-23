
#include <Arduino.h>

#include "basic_types.h"

#include "clock.h"
#include "print.h"
#include "queue.h"
#include "stack.h"

#include "button.h"
#include "knob.h"

knob_t knob({
    .clk = 3, // reversed for direction
    .dt = 2,
});
button_t button_knob({.pin = 9});
button_t button_back({.pin = 8});

button_t button_wood({.pin = 7});
button_t button_yes({.pin = 6});

button_t button_ex1({.pin = 5});
button_t button_ex2({.pin = 4});

#include "lcd.h"

lcd_t lcd({
    // I2C SDA SCL and
    .backlight_pwm_out = 10,
    .ambient_light_pwm_in = A0,
});

#include "log.h"
#include "error.h"

#include "state_machine.h"
#include "menu.h"
#include "readout.h"

#include "k_thermo.h"

k_thermo_dual_t k_thermo(
    // pins a
    {
        .cs = 22,
        .sck = 23,
        .so = 24,
    },
    // pins b
    {
        .cs = PIN_A12,
        .sck = PIN_A11,
        .so = PIN_A10,
    });

#include "neopixel_strip.h"
neopixel_strip_t led_strip({
    .pin = A6,
    .length = 9,
});

#include "stepper_tmc5160.h"
stepper_tmc5160_t stepper({
    .dir = 43,
    .step = 42,
    .mosi = 51,
    .miso = 50,
    .sck = 52,
    .cs = 53,
    .diag0 = 48,
    .diag1 = 47,
    .en = 49,
});

/*
#include "stepper_tmc2209.h"
stepper_tmc2209_t stepper({
    .dir = 31,
    .step = 30,
    .ms1 = 39,
    .ms2 = 37,
    .rx = 18,
    .tx = 19,
    .index = 41,
    .diag = 40,
    .en = 38,
});
*/

#include "valve_actuator.h"
#include "message_queue.h"
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
  button_ex1.once();
  button_ex2.once();

  lcd.once();
  k_thermo.once();

  led_strip.once();

  stepper.once();
  valve.once();

  state_machine.push_state(main_menu);
}

void loop() {
  clock.loop();

  /*
  // new arduino who dis 
  if ((clock.seconds & 3) <= 2) {
    digitalWrite(LED_BUILTIN, LOW);
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
  }
  */

  knob.loop();
  button_knob.loop();
  button_back.loop();
  button_wood.loop();
  button_yes.loop();
  button_ex1.loop();
  button_ex2.loop();

  lcd.loop();
  k_thermo.loop();

  led_strip.loop();

  stepper.loop();
  valve.loop();

  state_machine.loop();
}
