// minimal string printer
// except for the flash/ram bifurcation
// made for filling up 20 character LCD lines
// dirty bit cuts traffic across I2C
#pragma once

#define MUSTTAIL // [[gnu::musttail]] ??some day

// flash string that also works for globals
#undef F
#define F(str_lit)                                                             \
  ([]() {                                                                      \
    static const char s[] PROGMEM = str_lit;                                   \
    return reinterpret_cast<const __FlashStringHelper *>(s);                   \
  }())

struct smart_string_pointer_t {

  // arduino uno & micro address space does not use this bit afaict
  constexpr static uint high_bit = (uint(1) << 15);

  union ip_t {
    const char *inmemory;
    PGM_P inflash;
    uint bits;

    bool in_flash() { return (bits & high_bit) != 0; }
  } p;

  smart_string_pointer_t() { p.bits = 0; }
  smart_string_pointer_t(const __FlashStringHelper *fstr) {
    p.inflash = reinterpret_cast<PGM_P>(fstr);
    p.bits |= high_bit;
  }
  smart_string_pointer_t(const char *str) { p.inmemory = str; }

  bool in_flash() { return p.in_flash(); }

  const __FlashStringHelper *fstr() {
    ip_t f;
    f.bits = p.bits;
    f.bits &= ~high_bit;
    return reinterpret_cast<const __FlashStringHelper *>(f.inflash);
  }

  int length() {
    if (in_flash()) {
      return strlen_P(reinterpret_cast<PGM_P>(fstr()));
    } else {
      return strlen(p.inmemory);
    }
  }
};

static_assert(sizeof(smart_string_pointer_t) == 2);
// static_assert(std::is_trivial(smart_string_pointer_t));
static_assert(sizeof(const __FlashStringHelper *) == 2);
static_assert(sizeof(const char *) == 2);

struct print_head_t {
  char *_head;
  char *_end;
  bool dirty;

  void start(char *buf, int len) {
    _head = buf;
    _end = buf + len;
  }

  void _ac(char c) { // append character
    if ((*_head) != c) {
      dirty = true;
      *_head = c;
    }
    _head++;
  }

  print_head_t &s(smart_string_pointer_t ssp) {
    if (ssp.in_flash()) {
      MUSTTAIL return s(ssp.fstr());
    } else {
      MUSTTAIL return s(ssp.p.inmemory);
    }
  }

  print_head_t &s(const char *str) {
    while (_head < _end) {
      char c = *str;
      if (!c) {
        break;
      }
      _ac(c);
      str++;
    }
    return *this;
  }

  print_head_t &s(const __FlashStringHelper *fstr) {
    PGM_P str = reinterpret_cast<PGM_P>(fstr);
    while (_head < _end) {
      char c = pgm_read_byte(str);
      if (!c) {
        break;
      }
      _ac(c);
      str++;
    }
    return *this;
  }

  print_head_t &space(int n) {
    constexpr char c = ' ';
    for (int i = 0; i < n && _head < _end; i++) {
      _ac(c);
    }
    return *this;
  }

  void fill_with_space() {
    space(_end - _head);
    null_terminate();
  }

  void clear() { fill_with_space(); }

  void end() {
    null_terminate();
  }

  void null_terminate() {
    *_head = 0;
  }

  print_head_t &s(smart_string_pointer_t ssp, int field_width) {
    int l = ssp.length();
    int pad = max(0, field_width - l);
    MUSTTAIL return space(pad).s(ssp);
  }

  print_head_t &s_centered(smart_string_pointer_t ssp) {
    int field_width = _end - _head;
    int l = ssp.length();
    MUSTTAIL return s(ssp, l + (max(0, field_width - l) / 2));
  }

  print_head_t &s_right(smart_string_pointer_t ssp) {
    int field_width = _end - _head;
    int l = ssp.length();
    MUSTTAIL return s(ssp, l + max(0, field_width - l));
  }
  
  print_head_t &i(int n) {
    char nstr[16];
    itoa(n, nstr, 10);
    MUSTTAIL return s(nstr);
  }

  print_head_t &i(int n, int field_width) {
    char nstr[16];
    itoa(n, nstr, 10);
    MUSTTAIL return s(nstr, field_width);
  }

  print_head_t &i_centered(int n) {
    char nstr[16];
    itoa(n, nstr, 10);
    MUSTTAIL return s_centered(nstr);
  }

  print_head_t &i_right(int n) {
    char nstr[16];
    itoa(n, nstr, 10);
    MUSTTAIL return s_right(nstr);
  }

  print_head_t &i_time(int minutes) {
    int hours = minutes / 60;
    minutes -= hours * 60;
    char ostr[2] = {0, 0};
    if (minutes < 10) {
      ostr[0] = '0';
    }
    return i(hours).s(F(":")).s(ostr).i(minutes);
  }
};

// not quite posix
char *stpncpy(char *dest, smart_string_pointer_t ssrc, uint len) {
  uint l;
  if (ssrc.in_flash()) {
    PGM_P src = reinterpret_cast<PGM_P>(ssrc.fstr());
    l = min(strlen_P(src), len);
    strncpy_P(dest, src, l);
  } else {
    const char *src = ssrc.p.inmemory;
    l = min(strlen(src), len);
    strncpy(dest, src, l);
  }
  dest += l;
  *dest = 0;
  return dest;
}