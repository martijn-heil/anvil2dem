/*
  anvil2dem - Generate a DEM from .mca files.
  Copyright (C) 2017-2020  Martijn Heil

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

#include <stdbool.h>

#include "constants.h"

// Assumes that row and column start at 1, not at 0
#define rowcol_to_index(row, col, column_count) ((row-1) * column_count + col - 1)

struct lli_xy
{
  long long x;
  long long y;
};

struct lli_bounds
{
  long long maxx;
  long long minx;
  long long maxy;
  long long miny;
};

static inline bool lli_xy_equals(struct lli_xy first, struct lli_xy second)
{
  return first.x == second.x && first.y == second.y;
}

static inline bool lli_bounds_equals(struct lli_bounds first, struct lli_bounds second)
{
  return first.minx == second.minx &&
    first.maxx == second.maxx &&
    first.miny == second.miny &&
    first.maxy == second.maxy;
}

static inline struct lli_xy region_coords(long long x, long long y);
static inline struct lli_xy region_origin_topleft(long long region_x, long long region_y);
static inline struct lli_bounds region_bounds(long long region_x, long long region_y);


static inline struct lli_xy region_coords(long long x, long long y)
{
  long long region_x = x / REGION_WIDTH;
  if(x < 0 && (x % REGION_WIDTH) != 0) region_x--;

  long long region_y = y / REGION_HEIGHT;
  if(y < 0 && (y % REGION_HEIGHT) != 0) region_y--;

  struct lli_xy ret = { .x = region_x, .y = region_y };
  return ret;
}

static inline struct lli_xy region_origin_topleft(long long region_x, long long region_y)
{
  struct lli_bounds bounds = region_bounds(region_x, region_y);
  struct lli_xy ret = { .x = bounds.minx, .y = bounds.maxy };
  return ret;
}

static inline struct lli_bounds region_bounds(long long region_x, long long region_y)
{
  long long minx = region_x * 32 * 16;
  long long miny = region_y * 32 * 16;

  long long maxx = region_x * 32 * 16 + REGION_WIDTH - 1;
  long long maxy = region_y * 32 * 16 + REGION_HEIGHT - 1;

  struct lli_bounds bounds = {
    .maxx = maxx,
    .minx = minx,
    .maxy = maxy,
    .miny = miny,
  };
  return bounds;
}

#endif
