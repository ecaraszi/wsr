#pragma once

static_assert(sizeof(char) == 1);
static_assert(sizeof(bool) == 1);

static_assert(sizeof(int) == 2);
static_assert(sizeof(const char*) == 2);
static_assert(sizeof(PGM_P) == 2);
using uint = uint16_t;

static_assert(sizeof(long) == 4);
