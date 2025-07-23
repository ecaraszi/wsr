#pragma once

struct burn_history_t {
  static constexpr uint8_t period_min = 15;
  queue_t<int16_t, period_min> egt_reads;
  queue_t<char, lcd.columns> lcd_chars;
  uint8_t minutes = 0;

  void loop(int16_t egt) {
    if (clock.elapsed_min) {
      egt_reads.push(egt);
    }

    minutes += clock.elapsed_min;
    if (minutes >= period_min) {
      minutes = 0;
      lcd_chars.push('0' + (egt_reads.mean_value() / 100));
    }
  }

  void space(int16_t egt) {
    if (lcd_chars.head() != ' ') {
      for (uint8_t i = 0; i < period_min; i++) {
        egt_reads.push(egt);
      }
      lcd_chars.push(' ');
      minutes = 0;
    }
  }

  void print(int8_t line) {
    char text[lcd.max_str];
    for (uint8_t i = 0; i < lcd_chars.size(); i++) {
      text[i] = lcd_chars[i];
    }
    text[lcd_chars.size()] = 0;
    lcd.print(line).s_right(text);
  }
};

struct auto_burn_t : public state_t {

  int16_t notify_egt = 180;
  int16_t start_egt = 300;
  int16_t burn_egt = 325;
  int16_t hot_egt = 375;
  int16_t close_egt = 400;
  uint16_t adjust_minute = 0;
  int16_t adjust_egt = 0;
  int16_t adjust_percent = 0;
  int16_t start_percent = 30;
  bool adjust_human = false;
  bool enable_notify = false;

  burn_history_t history;

  auto_burn_t() {
    name = F("Auto Burn");
    new_cycle();
  }

  void set_air_percent(int8_t percent, bool adjust_human_) {
    int8_t clipped = max(0, min(100, percent));
    if (clipped != valve.percent) {
      valve.set_percent_air(clipped);
      adjust_minute = clock.minutes;
      adjust_egt = k_thermo.temp_f;
      adjust_percent = clipped;
      adjust_human = adjust_human_;
      if (!adjust_human) {
        enable_notify = true;
      }
    }
  }

  void new_cycle() {
    clock.minutes = clock.seconds = 0;
    adjust_minute = clock.minutes;
    enable_notify = false;
    history.space(k_thermo.temp_f);
  }

  void print_history() {
    history.print(2);

    // time since wood and latest air adjustment
    lcd.print(3)
        .s(F("t "))
        .i_time(clock.minutes)
        .s(F("  "))
        .i(adjust_egt)
        .s(F("/"))
        .i(adjust_percent)
        .s(F("/"))
        .i_time(clock.minutes - adjust_minute);
  }

  virtual void loop() override {
    int16_t egt = k_thermo.temp_f;
    uint16_t minutes_since_adjust = clock.minutes - adjust_minute;

    // exhaust gas temperature is need to know
    if (k_thermo.minutes_since_read > 3) {
      set_air_percent(0, false);
    }

    //
    // The Algorithm
    //

    // always try to close down the valve

    // bring the stove to the state of:
    // - egt in the 300s
    // - valve closed
    // - burning on secondary air
    // - accumulating charcoal

    // slow burn
    if (minutes_since_adjust >= 10) {
      if (egt >= burn_egt) {
        set_air_percent(valve.percent - max(3, valve.percent / 3), false);
      }
    }

    // fast burn
    if (minutes_since_adjust >= 3 && minutes_since_adjust < 10) {
      if (egt >= hot_egt) {
        set_air_percent(valve.percent - max(7, valve.percent / 2), false);
      }
    }

    // hot start
    if (minutes_since_adjust >= 3) {
      if (egt >= start_egt && valve.percent > start_percent && adjust_human) {
        set_air_percent(start_percent, false);
      }
    }

    // hot limit
    if (egt >= close_egt) {
      set_air_percent(0, false);
    }

    //
    // End Algorithm
    //

    // need wood
    if (egt < notify_egt && enable_notify) {
      led_strip.yellow();
      enable_notify = false;
    }

    // always allow knob adjustment
    if (knob.diff != 0) {
      set_air_percent(valve.percent + knob.diff, true);
    }

    // human added wood, human adds air
    if (button_wood.pushed()) {
      if (history.lcd_chars.head() == ' ') {
        if (valve.percent < start_percent) {
          set_air_percent(start_percent, true);
        } else {
          set_air_percent(valve.percent + 30, true);
        }
      } else {
        new_cycle();
      }
    }

    history.loop(egt);

    lcd.print(0).s_centered(name);
    lcd.print(1)
        .s(F("EGT:"))
        .i(egt, 4)
        .s(F("   Air:"))
        .i(valve.percent, 4)
        .s(F("%"));
    print_history();
  }
};

auto_burn_t auto_burn;
