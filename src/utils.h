/*
  nin-anvil - Generate a DEM from .mca files.
  Copyright (C) 2017-2019  Martijn Heil

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as published
  by the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef NIN_ANVIL_UTILS_H
#define NIN_ANVIL_UTILS_H

#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Assumes that row and column start at 1, not at 0
#define rowcol_to_index(row, col, column_count) ((row-1) * column_count + col - 1)

#ifdef __GNUC__
  #define unused_ __attribute((unused))
#else
  #define unused_
#endif

static inline void strupper(char *out, const char *in) {
  size_t i = 0;
  while(*in != '\0') {
    out[i] = toupper(*in);
    i++;
    in++;
  }
}

static inline bool streq(const char *str1, const char *str2) {
  return strcmp(str1, str2) == 0;
}

static inline bool string_starts_with(const char *subject, const char *prefix) {
  return strncmp(subject, prefix, strlen(prefix)) == 0;
}

#endif
