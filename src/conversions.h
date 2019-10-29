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

#ifndef NIN_ANVIL_CONVERSIONS_H
#define NIN_ANVIL_CONVERSIONS_H

#include "constants.h"

struct lli_xy
{
  long long x;
  long long y;
};

static inline struct lli_xy get_cartesian_region_coords(long long cartesian_x, long long cartesian_y)
{
  long long cartesian_region_x = cartesian_x / REGION_WIDTH;
  if(cartesian_x < 0 && (cartesian_x % REGION_WIDTH) != 0) cartesian_region_x--;

  long long cartesian_region_y = cartesian_y / REGION_HEIGHT;
  if(cartesian_y < 0 && (cartesian_y % REGION_HEIGHT) != 0) cartesian_region_y--;

  struct lli_xy ret = { .x = cartesian_region_x, .y = cartesian_region_y };
  return ret;
}

#endif
