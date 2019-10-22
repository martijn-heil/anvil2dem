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
#include <assert.h>

#include <libgen.h> // for basename()

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
static size_t image_buf_size;
static unsigned long long image_buf_width; // in blocks
static unsigned long long image_buf_height; // in blocks

long long region_x;
long long region_z;

static void output_point(long long cartesian_x, long long cartesian_y, uint8_t height)
{
  long long origin_cartesian_y = 0 - (region_z * 32 * 16);
  long long origin_cartesian_x = region_x * 32 * 16;

  long long row = cartesian_x - origin_cartesian_x + 1;
  long long column = origin_cartesian_y - cartesian_y + 1;
  size_t index = rowcol_to_index(row, column, image_buf_width);
  if(index >= image_buf_size) // Guard against buffer overflow
  {
    // TODO less cryptic error message
    // TODO when the regions are non-sequential, this condition can occur as well
    fprintf(stderr, "Calculated index exceeds image buffer size.\n");
    exit(EXIT_FAILURE);
  }
  image_buf[index] = height;
}

uint8_t *parse_world(const char *region_file_paths[], size_t n, is_ground_func_t is_ground_func,
    long long *out_image_buf_origin_cartesian_x,
    long long *out_image_buf_origin_cartesian_y,
    unsigned long long *out_image_buf_width,
    unsigned long long *out_image_buf_height,
    long long *max_cartesian_x,
    long long *min_cartesian_x,
    long long *max_cartesian_y,
    long long *min_cartesian_y)
{
  assert(max_cartesian_x != NULL);
  assert(min_cartesian_x != NULL);
  assert(max_cartesian_y != NULL);
  assert(min_cartesian_y != NULL);
  assert(out_image_buf_width != NULL);
  assert(out_image_buf_height != NULL);
  assert(out_image_buf_origin_cartesian_x != NULL);
  assert(out_image_buf_origin_cartesian_y != NULL);
  assert(is_ground_func != NULL);
  assert(n != 0);
  assert(region_file_paths != NULL);

  printf("in parse_world(n: %zu)\n", n);

  long long max_region_x = LLONG_MIN;
  long long min_region_x = LLONG_MAX;
  long long max_region_z = LLONG_MIN;
  long long min_region_z = LLONG_MAX;
  for(size_t i = 0; i < n; i++)
  {
    const char *path = region_file_paths[i];
    long long tmp_region_x;
    long long tmp_region_z;

    int result = sscanf(path, "r.%lli.%lli.mca", &tmp_region_x, &tmp_region_z);
    if(result == EOF || result < 2)
    {
      fprintf(stderr, "Failed to parse region file name to obtain region coordinates.\n");
      exit(EXIT_FAILURE);
    }

    if(tmp_region_x > max_region_x) max_region_x = tmp_region_x;
    if(tmp_region_x < min_region_x) min_region_x = tmp_region_x;
    if(tmp_region_z > max_region_z) max_region_z = tmp_region_z;
    if(tmp_region_z < min_region_z) min_region_z = tmp_region_z;
  }
  // Integer rounding is on purpose
  *out_image_buf_origin_cartesian_x = min_region_x * 32 * 16;
  *out_image_buf_origin_cartesian_y = 0 - (min_region_z * 32 * 16);

  unsigned long long width = max_region_x - min_region_x + 1;
  unsigned long long height = max_region_z - min_region_z + 1;
  image_buf_width = width * 32 * 16;
  image_buf_height = height * 32 * 16;

  *out_image_buf_width = image_buf_width;
  *out_image_buf_height = image_buf_height;

  image_buf_size = width * height * 512*512;
  image_buf = calloc(image_buf_size, 1);
  if(image_buf == NULL)
  {
    fprintf(stderr, "Could not allocate %zu bytes of memory. (%s)", image_buf_size, strerror(errno));
    exit(EXIT_FAILURE);
  }

  for(size_t i = 0; i < n; i++)
  {
    const char *path = region_file_paths[i];

    // basename may modify the contents of it's argument 'path', so we need to make a copy.
    char copy_of_path[strlen(path) + 1];
    strcpy(copy_of_path, path);
    const char *filename = basename(copy_of_path);

    int result = sscanf(filename, "r.%lli.%lli.mca", &region_x, &region_z);
    if(result == EOF || result < 2)
    {
      fprintf(stderr, "Failed to parse region file name to obtain region coordinates.\n");
      free(image_buf);
      exit(EXIT_FAILURE);
    }

    FILE *fp = fopen(path, "r");

    if(fp == NULL)
    {
      fprintf(stderr, "Could not open file '%s'. (%s)", path, strerror(errno));
      free(image_buf);
      exit(EXIT_FAILURE);
    }

    size_t size = fread(buf, 1, BUF_SIZE, fp);
    if(size == 0)
    {
      fprintf(stderr, "Could not read from file. (%s)\n", strerror(errno));
      free(image_buf);
      exit(EXIT_FAILURE);
    }

    if(size < 4096)
    {
      fprintf(stderr, "region file is not at least 4096 bytes. This could indicate a corrupt region file.\n");
      free(image_buf);
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
