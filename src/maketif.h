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

#ifndef NIN_ANVIL_MAKETIF_H
#define NIN_ANVIL_MAKETIF_H


void maketif(
    const char *filepath,
    const void *buf,
    const long long buf_origin_cartesian_x,
    const long long buf_origin_cartesian_y,
    const unsigned long long buf_width,
    const unsigned long long buf_height,
    const long long max_cartesian_x,
    const long long min_cartesian_x,
    const long long max_cartesian_y,
    const long long min_cartesian_y);
#endif
