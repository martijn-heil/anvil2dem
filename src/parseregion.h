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

#ifndef NIN_ANVIL_PARSEREGION
#define NIN_ANVIL_PARSEREGION

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


typedef void (*output_point_func_t)(long long cartesian_x, long long cartesian_y, uint8_t height, void *aux);


/*
 * Whether a block type should be ignored or not when calculating block column height
 * This is useful for example when you want to exclude leaves and logs (trees) from the resulting DEM.
 */
typedef bool (*is_ground_func_t)(uint8_t block_id);


// buf size should be at least 4096.
// 'size' is the amount of available bytes in buf, thus it should be at least 4096.

// the cartesian output bounds should already be initialized when passed to parse_region
// if they yet have no meaningful content, you can initialize them to LLONG_MAX and LLONG_MIN respectably.
void parse_region(const uint8_t *buf, const size_t size,
    long long *out_max_cartesian_x,
    long long *out_min_cartesian_x,
    long long *out_max_cartesian_y,
    long long *out_min_cartesian_y,
  output_point_func_t output_point_func,
    void *aux,
    is_ground_func_t is_ground_func);

#endif
