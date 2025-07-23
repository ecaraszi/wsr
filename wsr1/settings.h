#pragma once

int_readout_t readout_notify_egt(F("Notify EGT"), &auto_burn.notify_egt);
int_readout_t readout_start_egt(F("Start EGT"), &auto_burn.start_egt);
int_readout_t readout_burn_egt(F("Burn EGT"), &auto_burn.burn_egt);
int_readout_t readout_hot_egt(F("Hot EGT"), &auto_burn.hot_egt);
int_readout_t readout_close_egt(F("Close EGT"), &auto_burn.close_egt);

int_readout_t readout_start_percent(F("Start percent"), &auto_burn.start_percent);

int_readout_t readout_calibration_open(F("Open steps"), &valve.open_steps);
int_readout_t readout_calibration_closed(F("Closed steps"),
                                         &valve.closed_steps);

int_readout_t readout_force_neutral(F("Neutral force"),
                                    &load_sensor.neutral_force);
int_readout_t readout_force_stop(F("Stop force"), &load_sensor.stop_force);

int_readout_t backlight_off_secs(F("LCD timeout"), (int16_t*)(&lcd.backlight_off_sec));

state_t *settings_items[] = {
    &backlight_off_secs,
    &readout_notify_egt,
    &readout_start_egt,
    &readout_burn_egt,
    &readout_hot_egt,
    &readout_close_egt,
    &readout_start_percent,
    &readout_calibration_open,
    &readout_calibration_closed,
    &readout_force_neutral,
    &readout_force_stop,
    nullptr,
};

menu_t settings_menu(F("Settings"), settings_items);
