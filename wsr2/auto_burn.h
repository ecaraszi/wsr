#pragma once

struct burn_history_t {
  // burn_history_t provides a line of characters
  // that summarizes the exhaust gas temperature
  // over the past (20 chars * 15 period_min) = 5 hours.
  //
  // A character is the 100s place degrees F EGT.
  //
  // Adding wood inserts a space.
  //
  // For example if you added wood three hours ago,
  // when the stove was down to coals, it might say,
  // "2211111 234433322221"
  static constexpr uint period_min = 15;
  queue_t<int, period_min> egt_reads;
  queue_t<char, lcd.columns> lcd_chars;
  uint minutes = 0;

  void loop(int egt) {
    // every minute, remark the egt
    if (clock.elapsed_min) {
      egt_reads.push(egt);
    }

    // every period_min, summarize into a character
    if (clock.since_min(minutes, period_min)) {
      lcd_chars.push('0' + min(9, (egt_reads.mean_value() / 100)));
    }
  }

  void space(int egt) {
    if (lcd_chars.head() != ' ') {
      // can't remember why I flush egt_reads with egt in wsr1
      for (uint i = 0; i < period_min; i++) {
        egt_reads.push(egt);
      }
      lcd_chars.push(' ');
      minutes = 0;
    }
  }

  void print(int line) {
    char text[lcd.max_str];
    for (uint i = 0; i < lcd_chars.size(); i++) {
      text[i] = lcd_chars[i];
    }
    text[lcd_chars.size()] = 0;
    lcd.print(line).s_right(text);
  }
};

struct auto_burn_t : public state_t {

  // low temperature to notify for wood
  int notify_egt = 180;
  // after adding wood, reach this temperatue
  // for this time
  int burn_egt = 325;
  int burn_egt_min = 0;
  static constexpr int burn_egt_min_target = 30;
  // above this temperature, start closing down
  int hot_egt = 375;
  // stove is too hot, close the valve
  int close_egt = 450;
  // lcd line 3
  burn_history_t history;
  // the past minute of egt
  queue_t<int, 60> egt_samples;
  // last time the air valve was touched
  int adjust_minute = 0;
  // duration of previous wood cycle
  int previous_wood_minutes = 0;
  // turn on notification under notify_egt
  bool enable_notify = false;

  auto_burn_t() {
    name = F("Auto Burn");
    add_wood();
  }

  void add_wood() {
    // time itself resets on more wood
    previous_wood_minutes = clock.minutes;
    clock.minutes = clock.seconds = 0;
    history.space(k_thermo.temp_f);
    adjust_minute = clock.minutes;
    burn_egt_min = 0;
    enable_notify = false;
    log().s(F("add wood")).end();
  }

  void change_air_percent(int change_percent,
                          smart_string_pointer_t robot_reason) {
    int clipped = max(0, min(100, valve.percent + change_percent));
    if (clipped != valve.percent) {
      valve.set_percent_air(clipped);
      adjust_minute = clock.minutes;
      if (robot_reason.p.bits != 0) {
        enable_notify = true; // robot is driving, so call human for wood
        log()
            .s(robot_reason)
            .s(F(" "))
            .i(change_percent)
            .s(F(" "))
            .i(valve.percent)
            .end();
      }
    }
  }

  void print_history() {
    history.print(2);
    lcd.print(3)
        .s(F("p ")) // previous wood cycle time
        .i_time(previous_wood_minutes)
        .s(F(" c ")) // current wood cycle time
        .i_time(clock.minutes)
        .s(F(" -")) // time since last air adjustment
        .i_time(clock.minutes - adjust_minute);
  }

  struct algorithm_info_t {
    int delta_long;
    int delta_short;
  };

  algorithm_info_t the_algorithm(int egt) {
    //
    // The Algorithm
    // Is All New Draft & Theory For Hardware Revision 2
    // Not Even Tested Once
    // Look Again In December 2025 or January 2026
    // When It Be Tested A Few Hundred Times
    //

    // Imagine if I was standing there driving the stove
    // With Full Attention, and All The Time In The World

    // Like some kind of machine
    // "Yes, I know. A different kind of machine." -- Peter Watts

    if (clock.elapsed_sec) {
      egt_samples.push(egt);
    }

    if (egt >= burn_egt) {
      burn_egt_min += clock.elapsed_min;
    }

    // is the EGT going up or down and how fast
    // short term (10s) or long term (1m)
    // in tenths of a degree per second ("human fixed point")
    int delta_long = 0;
    int delta_short = 0;
    if (egt_samples.full()) {
      // linear and regression enough for me
      delta_long = ((egt_samples.head() - egt_samples.tail()) * 10) / 60;
      delta_short = ((egt_samples.head() - egt_samples[-10]) * 10) / 60;
    }
    algorithm_info_t info = {
        .delta_long = delta_long,
        .delta_short = delta_short,
    };

    // between 200 degrees (idle coals) and 375 degree (hot burn) is +175.
    // in five minutes, that's fast, 0.58 degrees/second.
    // in twenty minutes, that's slow, 0.14 degrees/second.
    constexpr int delta_fast = 5;
    constexpr int delta_slow = 2;

    uint minutes_since_adjust = clock.minutes - adjust_minute;
    bool act_normal = minutes_since_adjust >= 5;
    bool act_urgent = minutes_since_adjust >= 2;

    int small_change = max(3, valve.percent / 10);
    int large_change = max(7, valve.percent / 4);

    // temperature is low
    if (egt < burn_egt && burn_egt_min < burn_egt_min_target) {

      // add air
      if (delta_long < delta_slow && act_normal) {
        change_air_percent(large_change, F("ab l a"));
      }

      // trim air
      if (delta_short >= delta_fast && act_urgent) {
        change_air_percent(-small_change, F("ab l t"));
      }
    }

    // temperature is OK
    if (egt >= burn_egt && egt < hot_egt) {

      // trim air
      if (delta_long >= delta_slow && act_normal) {
        change_air_percent(-small_change, F("ab n lo"));
      }

      // trim air
      if (delta_short >= delta_fast && act_urgent) {
        change_air_percent(-small_change, F("ab n st"));
      }
    }

    // temperature is hot
    if (egt >= hot_egt && act_urgent) {

      // trim air
      if (delta_long > delta_fast || delta_short > delta_fast) {
        change_air_percent(-large_change, F("ab h lg"));
      }

      // trim air
      if (delta_long > 0 || delta_short > 0) {
        change_air_percent(-small_change, F("ab h sm"));
      }
    }

    //
    // End Algorithm
    //

    // predict the EGT in 5m
    // int predict_egt = egt + (int32_t(delta_egt * 300 /* seconds */) /
    //                          int32_t(egt_samples.capacity()));

    return info;
  }

  virtual void loop() override {
    // exhaust gas temperature is need to know
    if (k_thermo.stale_minutes >= 5) {
      error(F("k_t stale 5m"));
      // close the valve
      change_air_percent(-valve.percent, F("ab k_t s5"));
    }

    // always allow knob adjustment
    if (knob.diff != 0) {
      change_air_percent(knob.diff, smart_string_pointer_t());
      led_strip.off();
    }

    // human added wood
    if (button_wood.pushed()) {
      if (history.lcd_chars.head() != ' ') {
        add_wood();
      }
      led_strip.off();
    }

    int egt = k_thermo.temp_f;

    // need wood
    if (egt < notify_egt && enable_notify &&
        burn_egt_min >= burn_egt_min_target) {
      log().s(F("notify wood")).end();
      led_strip.yellow();
      enable_notify = false;
    }

    algorithm_info_t info = the_algorithm(egt);

    history.loop(egt);

    // quick access to logs menu
    if (button_yes.pushed()) {
      state_machine.push_state(logs_menu);
    }

    lcd.print(0).s_centered(name);
    lcd.print(1)
        .s(F("T"))
        .i(egt, 4)
        .s(F(" "))
        .i(info.delta_long)
        .s(F(" "))
        .i(info.delta_short)
        .s(F("   A"))
        .i(valve.percent, 4)
        .s(F("%"));
    print_history();
  }
};

auto_burn_t auto_burn;

//
// The WSR1 Algorithm
//

// int notify_egt = 180;
// int start_egt = 300;
// int burn_egt = 325;
// int hot_egt = 375;
// int close_egt = 425;

// always try to close down the valve
// don't touch it constantly because the A4988 stepper is loud

// bring the stove to the state of:
// - egt in the 300s
// - valve closed
// - burning on secondary air
// - accumulating charcoal

// // slow burn
// if (minutes_since_adjust >= 10) {
//   if (egt >= burn_egt) {
//     set_air_percent(valve.percent - max(3, valve.percent / 3), false);
//   }
// }

// // fast burn
// if (minutes_since_adjust >= 3 && minutes_since_adjust < 10) {
//   if (egt >= hot_egt) {
//     set_air_percent(valve.percent - max(7, valve.percent / 2), false);
//   }
// }

// // hot start
// if (minutes_since_adjust >= 3) {
//   if (egt >= start_egt && valve.percent > start_percent && adjust_human) {
//     set_air_percent(start_percent, false);
//   }
// }

// // hot limit
// if (egt >= close_egt) {
//   set_air_percent(0, false);
// }

//
// End Algorithm
//
