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

#ifndef NIN_ANVIL_PARSEREGION
#define NIN_ANVIL_PARSEREGION

#include <stdint.h>

#include "parseworld.h"

typedef void (*output_point_func_t)(long long cartesian_x, long long cartesian_y, uint8_t height);


// buf size should be at least 4096.
// 'size' is the amount of available bytes in buf, thus it should be at least 4096.
void parse_region(const uint8_t *buf, const size_t size,
    long long *out_max_cartesian_x,
    long long *out_min_cartesian_x,
    long long *out_max_cartesian_y,
    long long *out_min_cartesian_y,
    output_point_func_t output_point_func,
    is_ground_func_t is_ground_func);

#endif
