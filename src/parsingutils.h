/*
  anvil2dem - Generate a DEM from .mca files.
  Copyright (C) 2017-2021  Martijn Heil

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

#ifndef NIN_ANVIL_PARSINGUTILS_H
#define NIN_ANVIL_PARSINGUTILS_H

#include <stdint.h>
#include <stddef.h>

#include "parseregion.h" // for is_ground_func_t


//void region2dem(uint8_t *outbuf, const uint8_t *inbuf, size_t size, is_ground_func_t is_ground_func,
    //long long *out_cartesian_region_x,
    //long long *out_cartesian_region_y);

void regionfile2dem(uint8_t *outbuf, const char *filepath, is_ground_func_t is_ground_func,
    long long *out_cartesian_region_x,
    long long*out_cartesian_region_y);

#endif
