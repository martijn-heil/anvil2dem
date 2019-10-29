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
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <limits.h>

#include "parseregion.h"
#include "utils.h"
#include "constants.h"
#include "conversions.h"

struct auxdata
{
  uint8_t *outbuf;
  size_t size;
};

void output_point_func(long long cartesian_x, long long cartesian_y, uint8_t height, void *aux)
{
  struct auxdata *auxd = (struct auxdata *) aux;
  uint8_t *outbuf = auxd->outbuf;
  size_t size = auxd->size;

  struct lli_xy result = get_cartesian_region_coords(cartesian_x, cartesian_y);
  long long cartesian_region_x = result.x;
  long long cartesian_region_y = result.y;

  long long cartesian_region_origin_x = cartesian_region_x * 32 * 16;
  long long cartesian_region_origin_y = cartesian_region_y * 32 * 16 + REGION_HEIGHT - 1;


  long long ydiff = cartesian_region_origin_y - cartesian_y;
  //if(ydiff < 0) ydiff = 0 - ydiff;

  long long xdiff = cartesian_x - cartesian_region_origin_x;
  //if(xdiff < 0) xdiff = 0 - xdiff;

  long long row = ydiff + 1;
  long long column = xdiff + 1;
  size_t index = rowcol_to_index(row, column, 512);

  /*if(cartesian_y > 0)
  {
    fprintf(stderr, "xdiff: %lli, ydiff: %lli\n", xdiff, ydiff);
    fprintf(stderr, "row: %lli, column: %lli\n", row, column);
    fprintf(stderr, "cartesian_region_origin_y: %lli\n", cartesian_region_origin_y);
    fprintf(stderr, "cartesian_region_origin_x: %lli\n", cartesian_region_origin_x);
    fprintf(stderr, "cartesian_x: %lli, cartesian_y: %lli\n", cartesian_x, cartesian_y);
    fprintf(stderr, "cartesian_region_x: %lli, cartesian_region_y: %lli\n", cartesian_region_x, cartesian_region_y);
  }*/

  if(index >= size) // Guard against buffer overflow
  {
    // TODO less cryptic error message
    fprintf(stderr, "Calculated index %zu exceeds image buffer size %zu.\n", index, size);
    exit(EXIT_FAILURE);
  }
  outbuf[index] = height;
}

void region2dem(uint8_t *outbuf, const uint8_t *inbuf, size_t inbuf_size, is_ground_func_t is_ground_func,
    long long *out_cartesian_region_x,
    long long *out_cartesian_region_y)
{
  // These will get continuously updated as they are passed to parse_region()
  long long max_cartesian_x = LLONG_MIN;
  long long min_cartesian_x = LLONG_MAX;
  long long max_cartesian_y = LLONG_MIN;
  long long min_cartesian_y = LLONG_MAX;
  struct auxdata aux = { outbuf, REGION_SIZE };

  parse_region(inbuf, inbuf_size,
      &max_cartesian_x,
      &min_cartesian_x,
      &max_cartesian_y,
      &min_cartesian_y,
      output_point_func,
      &aux,
      is_ground_func);

  struct lli_xy result = get_cartesian_region_coords(min_cartesian_x, min_cartesian_y);
  *out_cartesian_region_x = result.x;
  *out_cartesian_region_y = result.y;
}


#define BUF_SIZE 52428800ULL
static uint8_t buf[BUF_SIZE];

void regionfile2dem(uint8_t *outbuf, const char *filepath, is_ground_func_t is_ground_func,
    long long *out_cartesian_region_x,
    long long *out_cartesian_region_y)
{
  fprintf(stderr, "handling %s\n", filepath);
  FILE *fp = fopen(filepath, "r");

  if(fp == NULL)
  {
    fprintf(stderr, "Could not open file '%s'. (%s)", filepath, strerror(errno));
    exit(EXIT_FAILURE);
  }

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

  region2dem(outbuf, buf, filesize, is_ground_func, out_cartesian_region_x, out_cartesian_region_y);
}
