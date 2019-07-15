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

#ifndef NIN_ANVIL_PARSEWORLD_H
#define NIN_ANVIL_PARSEWORLD_H

/*
 * Whether a block type should be ignored or not when calculating block column height
 * This is useful for example when you want to exclude leaves and logs (trees) from the resulting DEM.
 */
typedef bool (*is_ground_func_t)(uint8_t block_id);

uint8_t *parse_world(const char *region_file_paths[], size_t n, is_ground_func_t is_ground_func,
    long long *out_image_buf_origin_cartesian_x,
    long long *out_image_buf_origin_cartesian_y,
    unsigned long long *out_image_buf_width,
    unsigned long long *out_image_buf_height,
    long long *out_max_cartesian_x,
    long long *out_min_cartesian_x,
    long long *out_max_cartesian_y,
    long long *out_min_cartesian_y);

#endif
