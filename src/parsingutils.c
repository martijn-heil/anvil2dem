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

// All coordinates in this file are cartesian unless specified otherwise.

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>

#include "parseregion.h"
#include "utils.h"
#include "constants.h"
#include "conversions.h"

struct auxdata
{
  uint8_t *outbuf;
  size_t size;
};

void output_point_func(long long x, long long y, uint8_t height, void *aux)
{
  struct auxdata *auxd = (struct auxdata *) aux;
  uint8_t *outbuf = auxd->outbuf;
  size_t size = auxd->size;

  struct lli_xy result = region_coords(x, y);
  long long region_x = result.x;
  long long region_y = result.y;

  // region origin, topleft
  struct lli_xy origin = region_origin_topleft(region_x, region_y);

  long long ydiff = origin.y - y;
  long long xdiff = x - origin.x;

  long long row = ydiff + 1;
  long long column = xdiff + 1;
  size_t index = rowcol_to_index(row, column, 512);

  if(index >= size) // Guard against buffer overflow
  {
    // TODO less cryptic error message
    fprintf(stderr, "Calculated index %zu exceeds image buffer size %zu.\n", index, size);
    exit(EXIT_FAILURE);
  }
  outbuf[index] = height;
}

/*
 * outbuf must be at least of size REGION_SIZE.
 */
void region2dem(uint8_t *outbuf, const uint8_t *inbuf, size_t inbuf_size, is_ground_func_t is_ground_func,
    long long *out_region_x,
    long long *out_region_y)
{
  assert(outbuf != NULL);
  assert(out_region_x != NULL);
  assert(out_region_y != NULL);
  assert(inbuf != NULL);
  assert(is_ground_func != NULL);

  // These will get continuously updated as they are passed to parse_region()
  long long maxx = LLONG_MIN;
  long long minx = LLONG_MAX;
  long long maxy = LLONG_MIN;
  long long miny = LLONG_MAX;
  struct auxdata aux = { outbuf, REGION_SIZE };

  parse_region(inbuf, inbuf_size,
      &maxx,
      &minx,
      &maxy,
      &miny,
      output_point_func,
      &aux,
      is_ground_func);

  struct lli_xy result = region_coords(minx, miny);
  *out_region_x = result.x;
  *out_region_y = result.y;
}


#define BUF_SIZE 52428800ULL
static uint8_t buf[BUF_SIZE];

void regionfile2dem(uint8_t *outbuf, const char *filepath, is_ground_func_t is_ground_func,
    long long *out_region_x,
    long long *out_region_y)
{
  fprintf(stderr, "handling %s\n", filepath);
  FILE *fp = fopen(filepath, "r");

  if(fp == NULL)
  {
    fprintf(stderr, "Could not open file '%s'. (%s)", filepath, strerror(errno));
    exit(EXIT_FAILURE);
  }

  // important not to swap 1 and BUF_SIZE around.because otherwise if(filesize < 4096) would become bugged
  size_t filesize = fread(buf, 1, BUF_SIZE, fp);
  if(filesize == 0)
  {
    fprintf(stderr, "Could not read from file. (%s)\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  if(filesize < 4096)
  {
    fprintf(stderr, "region file is not at least 4096 bytes. This could indicate a corrupt region file.\n");
    exit(EXIT_FAILURE);
  }

  region2dem(outbuf, buf, filesize, is_ground_func, out_region_x, out_region_y);
}
