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

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <limits.h>
#include <errno.h>

#include "utils.h"
#include "parseregion.h"


#define BUF_SIZE 52428800ULL
static uint8_t buf[BUF_SIZE];

/*
 * The image buffer gets allocated with (filecount * 512*512) bytes, that is to say,
 * it has one byte of space for each block in all regions that are submitted to the program to be
 * processed. Upon allocation, the image buffer gets initialized with all-zero values.
 *
 * The image buffer gets filled gradually when reading chunks from region files, this does not happen in linear order,
 * as the way the block columns are sorted in chunks does not allow for that.
 *
 * Chunks that have not been generated are skipped, so as a result, it could occur that the buffer's geographical bounds
 * are larger than the geographical bounds of actually-inserted height data. When writing to the GeoTIFF only all bytes
 * within the geographical bounds of the actually-inserted height data are written to the GeoTIFF. To achieve this,
 * a max and min for the cartesian x and y of actually-inserted height data is kept track of in the variables below.
 * These max and min are incremented on a by-chunk basis, not on individual block columns. If a chunk gets parsed that is
 * currently outside the geographical bounds kept track of by these variables, they are incremented so that the chunk has
 * expanded them.
 *
 * 0 is treated as nodata
 */
static uint8_t *image_buf;

long long region_x;
long long region_z;

static void output_point(long long cartesian_x, long long cartesian_y, uint8_t height)
{
  long long origin_cartesian_y = 0 - (region_z * 32 * 16);
  long long origin_cartesian_x = region_x * 32 * 16;

  long long row = cartesian_x - origin_cartesian_x + 1;
  long long column = origin_cartesian_y - cartesian_y + 1;
  image_buf[rowcol_to_index(row, column)] = height;
}

uint8_t *parse_world(const char *region_file_paths[], size_t n, is_ground_func_t is_ground_func,
    long long *max_cartesian_x,
    long long *min_cartesian_x,
    long long *max_cartesian_y,
    long long *min_cartesian_y)
{

  image_buf = calloc(512*512, n);
  if(image_buf == NULL)
  {
    fprintf(stderr, "Could not allocate memory. (%s)", strerror(errno));
    exit(EXIT_FAILURE);
  }

  for(size_t i = 0; i < n; i++)
  {
    const char *path = region_file_paths[i];

    int result = sscanf(path, "r.%lli.%lli.mca",
        &region_x, &region_z);
    if(result == EOF || result < 2)
    {
      fprintf(stderr, "Failed to parse region file name to obtain region coordinates.\n");
      exit(EXIT_FAILURE);
    }

    FILE *fp = fopen(path, "r");

    if(fp == NULL)
    {
      fprintf(stderr, "Could not open file '%s'. (%s)", path, strerror(errno));
      exit(EXIT_FAILURE);
    }

    size_t size = fread(buf, 1, BUF_SIZE, fp);
    if(size == 0)
    {
      fprintf(stderr, "Could not read from file. (%s)\n", strerror(errno));
      exit(EXIT_FAILURE);
    }

    if(size < 4096)
    {
      fprintf(stderr, "region file is not at least 4096 bytes. This could indicate a corrupt region file.\n");
      exit(EXIT_FAILURE);
    }

    parse_region(buf, size,
        max_cartesian_x,
        min_cartesian_x,
        max_cartesian_y,
        min_cartesian_y,
        &output_point,
        is_ground_func);

    fclose(fp);
  }

  return image_buf;
}
