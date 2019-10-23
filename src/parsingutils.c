#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>

#include "parseregion.h"
#include "utils.h"

struct auxdata
{
  uint8_t *outbuf;
};

void output_point_func(long long cartesian_x, long long cartesian_y, uint8_t height, void *aux)
{
  struct auxdata *auxd = (struct auxdata *) aux;
  uint8_t *outbuf = auxd->outbuf;

  // Integer rounding is on purpose
  long long cartesian_region_x = cartesian_x / 16 / 32;
  long long cartesian_region_y = cartesian_y / 16 / 32;
  long long cartesian_region_origin_x = cartesian_region_x * 32 * 16;
  long long cartesian_region_origin_y = cartesian_region_y * 32 * 16;

  long long row = cartesian_x - cartesian_region_origin_x + 1;
  long long column = cartesian_region_origin_y - cartesian_y + 1;
  size_t index = rowcol_to_index(row, column, 512);
  if(index >= 4096) // Guard against buffer overflow
  {
    // TODO less cryptic error message
    // TODO when the regions are non-sequential, this condition can occur as well
    fprintf(stderr, "Calculated index exceeds image buffer size.\n");
    exit(EXIT_FAILURE);
  }
  outbuf[index] = height;
}

void region2dem(uint8_t *outbuf, const uint8_t *inbuf, is_ground_func_t is_ground_func,
    long long *out_cartesian_region_x,
    long long *out_cartesian_region_y)
{
  // These will get continuously updated as they are passed to parse_region()
  long long max_cartesian_x = LLONG_MIN;
  long long min_cartesian_x = LLONG_MAX;
  long long max_cartesian_y = LLONG_MIN;
  long long min_cartesian_y = LLONG_MAX;
  struct auxdata aux = { outbuf };

  parse_region(inbuf, 4096,
      &max_cartesian_x,
      &min_cartesian_x,
      &max_cartesian_y,
      &min_cartesian_y,
      output_point_func,
      &aux,
      is_ground_func);

  *out_cartesian_region_x = min_cartesian_x / 16 / 32;
  *out_cartesian_region_y = min_cartesian_y / 16 / 32;
}


#define BUF_SIZE 52428800ULL
static uint8_t buf[BUF_SIZE];

void regionfile2dem(uint8_t *outbuf, const char *filepath, is_ground_func_t is_ground_func,
    long long *out_cartesian_region_x,
    long long*out_cartesian_region_y)
{
  FILE *fp = fopen(filepath, "r");

  if(fp == NULL)
  {
    fprintf(stderr, "Could not open file '%s'. (%s)", filepath, strerror(errno));
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

  region2dem(outbuf, buf, is_ground_func, out_cartesian_region_x, out_cartesian_region_y);
}
