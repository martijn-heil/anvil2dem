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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include <xtiffio.h>
#include <geotiffio.h>

#include "utils.h"
#include "maketif.h"
#include "parsingutils.h"
#include "constants.h"
#include "conversions.h"



/*
                                            North(-z)
            +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
            | 0  | 1  | 2  | 3  | 4  | 5  | 6  | 7  | 8  | 9  | 10 | 11 | 12 | 13 | 14 | 15 |
            +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
            | 16 | 17 | 18 | 19 | 20 | 21 | 22 | 23 | 24 | 25 | 26 | 27 | 28 | 29 | 30 | 31 |
            +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
            | 32 | 33 | 34 | 35 | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | And so on..
            +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
            | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
            +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
            | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
            +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
            | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
            +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
            | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
            +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
            | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
West(-x)    +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+   East(+x)
            | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
            +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
            | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
            +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
            | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
            +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
            | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
            +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
            | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
            +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
            | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
            +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
            | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
            +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
            | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  |
            +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
                                            South (+z)
*/

/*
 * About variable names:
 *
 * All coordinates are absolute unless specified otherwise.
 *
 * Row and column are always relative to their container.
 */

static uint8_t forbidden_blocks[] = {0, 18, 161, 17, 162, 8, 9};
static const size_t forbidden_blocks_size = sizeof(forbidden_blocks);

/*
 * Whether a block type should be ignored or not when calculating block column height
 * This is useful for example when you want to exclude leaves and logs (trees) from the resulting DEM.
 */
static bool is_ground(uint8_t block_id)
{
  for (size_t i = 0; i < forbidden_blocks_size; i++)
  {
    if(forbidden_blocks[i] == block_id) return false;
  }
  return true;
}

// returns -1 if none matched
static int compression_from_string(const char *str)
{
  char newstr[strlen(str) + 1];
  strupper(newstr, str);

  if     (streq(str, "NONE"))           return COMPRESSION_NONE;
  else if(streq(str, "CCITTRLE"))       return COMPRESSION_CCITTRLE;
  else if(streq(str, "CCITTFAX3"))      return COMPRESSION_CCITTFAX3;
  else if(streq(str, "CCITTFAX4"))      return COMPRESSION_CCITTFAX4;
  else if(streq(str, "LZW"))            return COMPRESSION_LZW;
  else if(streq(str, "OJPEG"))          return COMPRESSION_OJPEG;
  else if(streq(str, "JPEG"))           return COMPRESSION_JPEG;
  else if(streq(str, "NEXT"))           return COMPRESSION_NEXT;
  else if(streq(str, "CCITTRLEW"))      return COMPRESSION_CCITTRLEW;
  else if(streq(str, "PACKBITS"))       return COMPRESSION_PACKBITS;
  else if(streq(str, "THUNDERSCAN"))    return COMPRESSION_THUNDERSCAN;
  else if(streq(str, "IT8CTPAD"))       return COMPRESSION_IT8CTPAD;
  else if(streq(str, "IT8LW"))          return COMPRESSION_IT8LW;
  else if(streq(str, "IT8MP"))          return COMPRESSION_IT8MP;
  else if(streq(str, "IT8BL"))          return COMPRESSION_IT8BL;
  else if(streq(str, "PIXARFILM"))      return COMPRESSION_PIXARFILM;
  else if(streq(str, "PIXARLOG"))       return COMPRESSION_PIXARLOG;
  else if(streq(str, "DEFLATE"))        return COMPRESSION_DEFLATE;
  else if(streq(str, "ADOBE_DEFLATE"))  return COMPRESSION_ADOBE_DEFLATE;
  else if(streq(str, "DCS"))            return COMPRESSION_DCS;
  else if(streq(str, "JBIG"))           return COMPRESSION_JBIG;
  else if(streq(str, "SGILOG"))         return COMPRESSION_SGILOG;
  else if(streq(str, "SGILOG24"))       return COMPRESSION_SGILOG24;
  else if(streq(str, "JP2000"))         return COMPRESSION_JP2000;
  else return -1;
}


void print_usage(const char *prog_str)
{
  printf(
    "Usage: %s [options] region_file\n"
    "Options:\n"
    "  -h, --help                Show this usage information.\n"
    "  -v, --version             Show version information.\n"
    "  --blocks=<file>           List of blocks that should be taken into account.\n"
    "  --ignoredblocks=<file>    List of blocks that should NOT be taken into account.\n"
    "  --compression=<scheme>    TIFF compression scheme, defaults to DEFLATE.\n"
    "\n"
    "scheme is case-insensitive and can be one of the following values:\n"
    "NONE, "
    "CCITTRLE, "
    "CCITTFAX3, "
    "CCITTFAX4, "
    "LZW, "
    "OJPEG, "
    "JPEG, "
    "NEXT, "
    "CCITTRLEW, "
    "PACKBITS, "
    "THUNDERSCAN, "
    "IT8CTPAD, "
    "IT8LW, "
    "IT8MP, "
    "IT8BL, "
    "PIXARFILM, "
    "PIXARLOG, "
    "DEFLATE, "
    "ADOBE_DEFLATE, "
    "DCS, "
    "JBIG, "
    "SGILOG, "
    "SGILOG24, "
    "JP2000\n"
    ,prog_str
  );
}

void print_version(void)
{
  printf("v1.0.0-SNAPSHOT\n");
}

// Obviously, we assume that argc is never less than 0
int main(int argc, char *argv[])
{
  if(argc == 1 || (argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)))
  {
    print_usage(argv[0]);
    exit(EXIT_SUCCESS);
  }

  if(argc == 2 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0))
  {
    print_version();
    exit(EXIT_SUCCESS);
  }

  const char *opts[argc - 1];
  const char *files[argc - 1];
  size_t files_i = 0;
  size_t opts_i = 0;
  for(int i = 1; i < argc; i++)
  {
    if(string_starts_with(argv[i], "-"))
      opts[opts_i++] = argv[i];
    else
      files[files_i++] = argv[i];
  }
  size_t filecount = files_i;
  size_t optscount = opts_i;

  int compression = COMPRESSION_DEFLATE;
  // Print requested information and continue
  for(size_t i = 0; i < optscount; i++) {
    if(streq(opts[i], "--version") || streq(opts[i], "-v"))
      print_version();
    else if(streq(opts[i], "--help") || streq(opts[i], "-h"))
      print_usage(argv[0]);
    else if(strlen(opts[i]) > 14 && strncmp(opts[i], "--compression=", 14) == 0) // 14 is the length of "--compression="
    {
      const char *compression_string = opts[i] + 14;
      compression = compression_from_string(compression_string);
      if(compression == -1) // invalid type of compression specified
      {
        fprintf(stderr, "Specified invalid type of compression '%s'", compression_string);
        exit(EXIT_FAILURE);
      }
    }
  }
  if(filecount == 0) exit(EXIT_SUCCESS);


  const size_t imgbuf_size = REGION_SIZE;
  uint8_t *imgbuf = calloc(imgbuf_size, 1);
  if(imgbuf == NULL)
  {
    fprintf(stderr, "Could not allocate image buffer. (%s)", strerror(errno));
    exit(EXIT_FAILURE);
  }

  long long region_x;
  long long region_y;
  regionfile2dem_wkt("debug_output.wkt", files[0], is_ground, &region_x, &region_y);
  exit(EXIT_SUCCESS);

  /*long long region_x;
  long long region_y;
  regionfile2dem(imgbuf, files[0], is_ground, &region_x, &region_y);
  printf("main.c: cartesian region coords x: %lli, y: %lli\n", region_x, region_y);

  struct lli_xy origin = region_origin_topleft(region_x, region_y);
  struct lli_bounds bounds = region_bounds(region_x, region_y);

  char *output_filename;
  if(asprintf(&output_filename, "%llix_%lliy.tif", region_x, region_y) == -1)
  {
    fprintf(stderr, "Could not generate output file name.\n");
    exit(EXIT_FAILURE);
  }*/

  // Debugging progress as of 2020:
  // The bug appears to not be in maketif.c, as when excluding this entire GeoTIFF making step,
  // by writing the raw memory buffer to a file and inspecting that, it's still present.
  // This debugging step is in the commented out code below.
  //
  // The next step is outputting vector points to see whether it goes wrong in the rasterization process of the
  // elevation data that occurs in parsingutils.c

  /*if(cartesian_region_x == 2 && cartesian_region_y == -6) {
    printf("writing to out.bin!\n");
    FILE *tmpfp = fopen("out.bin", "w");
    size_t items_written = fwrite(imgbuf, imgbuf_size, 1, tmpfp);
    if(items_written != 1)
    {
      fprintf(stderr, "error writing to test file (%s)\n", strerror(errno));
      exit(EXIT_FAILURE);
    }
    fclose(tmpfp);
    exit(EXIT_SUCCESS);
  }
*/
  maketif(output_filename, imgbuf, compression,
      origin.x,
      origin.y,
      REGION_WIDTH,
      REGION_HEIGHT,
      bounds.maxx,
      bounds.minx,
      bounds.maxy,
      bounds.miny);

  free(output_filename);
  free(imgbuf); // TODO use atexit() instead to free up resources
  return EXIT_SUCCESS;
}

