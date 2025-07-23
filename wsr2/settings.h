#pragma once

namespace readouts {

int_readout_t notify_egt(F("Notify EGT"), &auto_burn.notify_egt);
int_readout_t burn_egt(F("Burn EGT"), &auto_burn.burn_egt);
int_readout_t hot_egt(F("Hot EGT"), &auto_burn.hot_egt);
int_readout_t close_egt(F("Close EGT"), &auto_burn.close_egt);

int_readout_t calibration_open(F("Open steps"), &valve.open_steps);
int_readout_t calibration_closed(F("Closed steps"), &valve.closed_steps);

int_readout_t backlight_off_secs(F("LCD timeout"),
                                 (int *)(&lcd.backlight_off_sec));
int_readout_t backlight_low_percent(F("LCD light low"),
                                    (int *)(&lcd.backlight_low_percent));
int_readout_t backlight_high_percent(F("LCD light high"),
                                     (int *)(&lcd.backlight_high_percent));
int_readout_t ambient_light_sensor(F("Ambient Light"), (int *)(&lcd.ambient));

state_t *settings_items[] = {
    &backlight_off_secs,
    &backlight_low_percent,
    &backlight_high_percent,
    &ambient_light_sensor,
    &notify_egt,
    &burn_egt,
    &hot_egt,
    &close_egt,
    &calibration_open,
    &calibration_closed,
    nullptr,
};

} // namespace readouts

menu_t settings_menu(F("Settings"), readouts::settings_items);
