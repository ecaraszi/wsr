#pragma once

// clang-format off
state_t* main_menu_items[] = {
  &auto_burn,
  &manual_burn,
  &settings_menu,
  &calibration_menu,
  &logs_menu,
  &errors_menu,
  nullptr,
};
// clang-format on

struct main_menu_t : public menu_t {

  main_menu_t() : menu_t(F("Main Menu"), main_menu_items){};

  virtual void loop() override {
    menu_t::loop();

    // User Story:  back back back stop stop what.
    // Backing up to the main menu always stops movement.
    // Or wsr2 has a big red switch on the bottom right corner.
    // Should also close the valve here if calibrated?
    valve.disconnect();

    // At the main menu, the wood button goes directly to auto_burn
    if (state_machine._stack.size() == 1 && button_wood.pushed()) {
      state_machine.push_state(auto_burn);
    }
  }
};

main_menu_t main_menu;
