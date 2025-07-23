#pragma once

state_t* main_menu_items[] = {
  &auto_burn,
  &manual_burn,
  &settings_menu,
  &calibration_menu,
  nullptr,
};

struct main_menu_t : public menu_t {

  main_menu_t() : menu_t(F("Main Menu"), main_menu_items) {};

  virtual void loop() override {
    menu_t::loop();

    // (backing up to) the main menu always stops movement 
    valve.stop();
  }
};

main_menu_t main_menu;
