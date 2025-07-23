// These Trinamic folks have it dialed,
// they know how to integrate a circuit and liven up some motor coils.
// Extra thanks to the datasheet and the TMCStepper library.
#pragma once

#include <TMCStepper.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcpp" // doesn't actually work, hmm
#define USE_TIMER_1 true
#define USE_TIMER_2 false
#define USE_TIMER_3 false
#define USE_TIMER_4 false
#define USE_TIMER_5 false
#include <TimerInterrupt.h>
#pragma GCC diagnostic pop

class TMC5160Stepper_local : public TMC5160Stepper {
public:
  TMC5160Stepper_local(uint pinCS, float RS) : TMC5160Stepper(pinCS, RS) {}

  // TMCStepper is missing R+WC for motor stall?
  // or this does nothing?
  // read() and write() are protected
  void rwc_RAMP_STAT() {
    RAMP_STAT_t r{0};
    r.sr = read(RAMP_STAT_t::address);
    write(RAMP_STAT_t::address, r.sr);
  }
};

namespace TMC5160_n_local {
// or earlier?  read more datasheets 
// TMC2130 struct is missing stealth, s2vsa & b.
// clang-format off
struct DRV_STATUS_t {
  constexpr static uint address = 0x6F;
  union {
    uint32_t sr;
    struct {
      uint sg_result : 10;
      uint            : 2;
      bool s2vsa : 1;
      bool s2vsb : 1;
      bool stealth : 1;
      bool fsactive : 1;
      uint cs_actual : 5,
                        : 3;
      bool  stallGuard : 1,
            ot : 1,
            otpw : 1,
            s2ga : 1,
            s2gb : 1,
            ola : 1,
            olb : 1,
            stst : 1;
    };
  };
};
// clang-format on
} // namespace TMC5160_n_local

struct stepper_tmc5160_t;
void step_interrupt(stepper_tmc5160_t *s);

struct stepper_tmc5160_t {
  struct pins_t {
    int dir;
    int step;
    int mosi;
    int miso;
    int sck;
    int cs;
    int diag0;
    int diag1;
    int en;
  } pins;

  // From refs/BIGTREETECH TMC5160-V1.0 manual.pdf
  static constexpr float r_sense = 0.075f;

  volatile int dir = 0;
  volatile int parity = 0;
  volatile long steps = 0;
  long target = 0; // target steps

  // Above this class e.g. valve_actuator_t operates
  // in full steps.
  static constexpr long microsteps = 8;

  enum internal_state {
    SLEEP = 'Z',
    IDLE = 'I',
    MOVE = 'M',
    STALL = 'S',
  };
  uint state = SLEEP;

  // Sleeping stops interrupts.
  // Used to be sleeping was quiet for A4988 driver.
  // TMC5160 is quiet itself.
  // And, who needs a hundreds-hz no-op interrupt.
  uint idle_ms = 0;
  static constexpr long interrupt_hz = 1000;

  // The stepper motor has
  static constexpr long fullsteps_per_rotation = 200;

  // RPM is on the stallguard tuning chart, see driver.sgt().
  static_assert(37 == (interrupt_hz * 60L /* seconds per minute */) /
                          (fullsteps_per_rotation * microsteps));

  // driver status
  GSTAT_t gstat;
  TMC5160_n_local::DRV_STATUS_t drv_status;
  uint rms_actual = 0;
  uint status_read_ms = 0;
  uint tstep = 0;

  // stallguard
  bool sg_diag1_active = false;
  bool sg_stall_recovered = false;
  uint sg_stall_cooldown_ms = 0;
  long sg_steps = 0;
  int sg_dir = 0;

  TMC5160Stepper_local driver;

  stepper_tmc5160_t(pins_t pins_) : pins(pins_), driver(pins.cs, r_sense) {}

  void once() {
    pinMode(pins.dir, OUTPUT);
    pinMode(pins.step, OUTPUT);
    digitalWrite(pins.dir, dir);
    digitalWrite(pins.step, parity);

    pinMode(pins.en, OUTPUT);
    digitalWrite(pins.en, LOW);

    pinMode(pins.diag0, INPUT);
    pinMode(pins.diag1, INPUT);

    SPI.begin();

    driver.begin();

    // driver enable non-zero and
    // duration of slow decay phase is (24 + 32 * toff) clocks, [0, 15]
    driver.toff(4);

    // tbl register, 24 (clocks) is 0b01
    driver.blank_time(24);

    driver.rms_current(250); // mA

    driver.dedge(true);
    driver.microsteps(microsteps);

    // two settings above default to reduce ringing maybe?
    driver.filt_isense(0b10);

    // freewheeling is fine
    driver.ihold(0);
    driver.freewheel(0b01);

    // TSTEP for 1000hz interrupt at 8 microsteps reads ~378
    // 8 microsteps per fullstep is 32 1/256 microsteps
    // 378 clocks * 32 1/256 microsteps = 12k clocks in 1ms
    // 12k * 1000hz = 12mhz clock, sounds right ?

    // Effectively with these settings, coolstep/stallguard
    // are always on and stealthchop is always off ?

    // Could have one setup for calibration, and another
    // for calibrated ends-and-middle sections, but the
    // wood stove air valve doesn't need to move that fast?

    // stealthchop quiet mode
    driver.en_pwm_mode(true);
    driver.pwm_autoscale(true);
    // if TSTEP >= TPWMTHRS, stealthchop is enabled
    driver.TPWMTHRS(0);

    // stallguard2 (sg2) and coolstep

    // if TCOOLTHRS >= TSTEP >= THIGH, coolstep enabled over stealthchop
    driver.TCOOLTHRS(0xFFFFF); // 20bit max

    // if TSTEP <= THIGH, coolstep and stealthchop disabled
    driver.THIGH(0); // min

    // sg2 filter, 0 high resolution, 1 filtered across 4 fullsteps.
    // End stops do not work as well with filtering on.
    // With filtering on, the backoff in STALL state would have to
    // be increased?
    driver.sfilt(0);

    // sg_result values are ~350-500 range while free moving

    // if sg_result < (semin * 32), high load, increase motor current
    // 5 * 32 = 160, from example
    // 1 * 32 = 32, almost disabled, no need to push through what? load
    driver.semin(1);
    // current stepup speed, 0b00 is least stepup
    // not sure about this setting, leave default?
    // driver.seup(0b00);

    // if sg_result >= ((semin+semax+1) * 32)), low load, decrease motor current
    // (5+2+1) * 32 = 256, from example
    // (1+4+1) * 32 = 192
    driver.semax(4);
    // current stepdown speed, 0b01 "for each 8 sg2 values", of 32,8,2,1,
    // "medium" also not sure about this setting
    driver.sedn(0b01);

    // Set effective range of stallguard.
    // Page 89 of the datasheet.
    // Experiment with various settings along with current & RPM.
    // [-64, 63], lower is more sensitive.
    // 4 seems to be the limit before too sensitive.
    driver.sgt(4);

    // set diag1 pin high when stalled i.e. sg_result is zero.
    // don't set diag1 for other reasons
    driver.diag1_stall(true);
    driver.diag1_index(false);
    driver.diag1_onstate(false);
    driver.diag1_steps_skipped(false);
    driver.diag1_poscomp_pushpull(true);

    // stop when stalled
    driver.sg_stop(true);

    // clear existing errors
    driver.GSTAT(
        0b111); // argument is ignored, 0b111 hardcoded in TMCStepper for R+WC
    driver.rwc_RAMP_STAT(); // not sure this does anything

    ITimer1.init();
  }

  void _idle() {
    idle_ms = 0;
    state = IDLE;
  }

  void _sleep() {
    ITimer1.detachInterrupt();
    state = SLEEP;
    k_thermo.set_disconnect(false);
  }

  void _wake() {
    k_thermo.set_disconnect(true);
    ITimer1.attachInterrupt(float(interrupt_hz), step_interrupt, this);
    _idle();
  }

  void loop() {
    bool read_status = false;

    if (state == SLEEP) {
      if (steps != target) {
        _wake();
      }
    }

    if (state == IDLE) {
      if (steps != target) {
        state = MOVE;
      } else {
        idle_ms = min(30000, idle_ms + clock.elapsed_ms);
        if (idle_ms > 2000) {
          _sleep();
        }
      }
    }

    if (state == MOVE) {
      if (steps != target) {
        // interrupt does the work
      } else {
        _idle();
      }
    }

    if (state == STALL) {
      read_status = true;
      if (steps == target) {
        if (digitalRead(pins.diag1) || drv_status.stallGuard) {
          // back off a few full steps until the stall clears
          // 200 fullsteps per 8mm T8 nut travel per rotation
          // 12 steps is 0.5mm, not much air valve, some clearance
          target -= microsteps * 12 * sg_dir;
          reset_stall();
        } else if (clock.since_ms(sg_stall_cooldown_ms, 250)) {
          // clear
          sg_stall_recovered = true;
          sg_steps = steps;
          _idle();
        }
      }
    }

    if (state != SLEEP) {
      read_status |= clock.since_ms(status_read_ms, 333 /* 3x per second */);
    }

    if (read_status) {
      gstat = {0};
      gstat.sr = driver.GSTAT();
      drv_status = {0};
      drv_status.sr = driver.DRV_STATUS();
      rms_actual = driver.cs2rms(drv_status.cs_actual);
      tstep = driver.TSTEP();
      sg_diag1_active = digitalRead(pins.diag1);
    }
  }

  void move_to(int target_fullsteps) {
    if (state != STALL) {
      target = long(target_fullsteps) * microsteps;
    }
  }
  void move_delta(int delta_fullsteps) {
    if (state != STALL) {
      target += long(delta_fullsteps) * microsteps;
    }
  }
  void stop() { target = steps; }
  void stop_at(int target_fullsteps) {
    steps = target = long(target_fullsteps) * microsteps;
  }
  int fullsteps() { return steps / microsteps; }
  void reset_stall() {
    sg_stall_cooldown_ms = 0;
    sg_stall_recovered = false;
  }
  bool at_rest() { return state == IDLE || state == SLEEP; }

  void print() {
    lcd.print(2)
        .s(F("S "))
        .i(steps / microsteps, 4)
        .s(F(" SG "))
        .i(drv_status.sg_result, 3)
        .s(F(" C "))
        .i(rms_actual, 3);

    bool unusual_drv_status =
        drv_status.ola | drv_status.olb | drv_status.s2gb | drv_status.s2ga |
        drv_status.otpw | drv_status.ot | drv_status.s2vsb | drv_status.s2vsa;
    // status[] = drv_status.ola ? '1' : '0';
    // status[] = drv_status.olb ? '1' : '0';
    // status[] = drv_status.s2gb ? '1' : '0';
    // status[] = drv_status.s2ga ? '1' : '0';
    // status[] = drv_status.otpw ? '1' : '0';
    // status[] = drv_status.ot ? '1' : '0';
    // status[] = drv_status.s2vsb ? '1' : '0';
    // status[] = drv_status.s2vsa ? '1' : '0';

    char status[21];
    status[0] = gstat.reset ? '1' : '0';
    status[1] = gstat.drv_err ? '1' : '0';
    status[2] = gstat.uv_cp ? '1' : '0';
    status[3] = unusual_drv_status ? '1' : '0';
    status[4] = ' ';

    status[5] = drv_status.stallGuard ? '1' : '0';
    status[6] = sg_diag1_active ? '1' : '0';
    status[7] = ' ';

    status[8] = drv_status.stst ? '1' : '0';
    status[9] = drv_status.stealth ? '1' : '0';
    status[10] = drv_status.fsactive ? '1' : '0';
    status[11] = ' ';

    status[12] = state;
    status[13] = 0;

    lcd.print(3).s(status).s(F(" ")).i(tstep);
  }
};

void step_interrupt(stepper_tmc5160_t *ps) {
  stepper_tmc5160_t &s = *ps;
  int steps = s.target - s.steps;
  int dir = 0;

  if (steps < 0) {
    dir = -1;
    steps = -steps;
  } else if (steps > 0) {
    dir = 1;
  }

  if (s.state == stepper_tmc5160_t::internal_state::MOVE &&
      digitalRead(s.pins.diag1)) {
    s.state = stepper_tmc5160_t::internal_state::STALL;
    s.sg_steps = s.target = s.steps;
    s.sg_dir = dir;
    s.reset_stall();
    // clean up in loop()
    return;
  }

  if (dir != s.dir) {
    if (dir < 0) {
      digitalWrite(s.pins.dir, LOW);
    } else if (dir > 0) {
      digitalWrite(s.pins.dir, HIGH);
    }
    s.dir = dir;

    // allow next-interrupt delay for dir to take effect
    return;
  }

  // n.b. driver.dedge(true)
  if (s.dir != 0) {
    if (s.parity == 1) {
      digitalWrite(s.pins.step, LOW);
      s.steps += s.dir;
      s.parity = 0;
    } else {
      digitalWrite(s.pins.step, HIGH);
      s.steps += s.dir;
      s.parity = 1;
    }
  }
}
