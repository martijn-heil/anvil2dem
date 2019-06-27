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
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <inttypes.h>

#include <arpa/inet.h>

#include <nbt/nbt.h>

#include <xtiffio.h>
#include <geotiffio.h>

#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))

#define hton16(x) htons(x)
#define hton32(x) htonl(x)
#define hton64(x) htonll(x)

#define ntoh16(x) ntohs(x)
#define ntoh32(x) ntohl(x)
#define ntoh64(x) ntohll(x)

#ifdef __GNUC__
  #define unused_ __attribute((unused))
#else
  #define unused_
#endif

// See https://stackoverflow.com/questions/24059421
#define TIFFTAG_GDAL_NODATA 42113
static const TIFFFieldInfo tiff_field_info[] = {
  { TIFFTAG_GDAL_NODATA, -1, -1, TIFF_ASCII, FIELD_CUSTOM, 0, 1, "GDAL_NODATA" }
};

static void register_custom_tiff_tags(TIFF *tif) {
  TIFFMergeFieldInfo(tif, tiff_field_info, sizeof(tiff_field_info) / sizeof(tiff_field_info[0]));
}


struct chunkpos {
  int32_t x;
  int32_t z;
};

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

static void handle_chunk(nbt_node *chunk);
static bool handle_section(nbt_node *section, void *aux);

#define block_xz_to_index(x, z) ((z-1) * 16 + x - 1)

// Assumes that row and column start at 1, not at 0
#define imgbuf_rowcol_to_index(row, col) ((row-1) * 16 + col - 1)

static uint8_t current_chunk_heightmap[256];

/*
 * About variable names:
 *
 * When a coordinate variable is named "cartesian" is is always an absolute coordinate, never relative,
 * unless explicitly specified otherwise.
 *
 * Row and column are always relative to their container.
 *
 */


/*
 * The image buffer gets allocated with ((argc-1) * 512*512) bytes (in main), that is to say,
 * it has one byte of space for each block in all regions that are submitted to the program to be
 * processed. Upon allocation, the image buffer gets initialized with all-zero values.
 *
 * The image buffer gets filled gradually when reading chunks from region files, this does not happen in linear order,
 * as the way the block columns are sorted in chunks does not allow for that.
 *
 * Chunks that have not been generated are skipped, so as a result, it could occur that the buffer's geographical bounds
 * are larger than the geographical bounds of actually-inserted height data. When writing to the GeoTIFF only all bytes
 * within the geographical bounds of the actually-inserted height data is written to the GeoTIFF. To achieve this,
 * a max and min for the cartesian x and y of actually-inserted height data is kept track of in the variables below.
 * These max and min are incremented on a by-chunk basis, not on individual block columns. If a chunk gets parsed that is
 * currently outside the geographical bounds kept track of by these variables, they are incremented so that the chunk has
 * expanded them.
 */
static uint8_t *image_buf; // Gets allocated in main
static long long origin_cartesian_x, origin_cartesian_y; // Origin is top-left

// These are continuously updated in handle_chunk()
static long long max_cartesian_x = 0;
static long long min_cartesian_x = 0;
static long long max_cartesian_y = 0;
static long long min_cartesian_y = 0;

/*
 * Whether a block type should be ignored or not when calculating block column height
 * This is useful for example when you want to exclude leaves and logs (trees) from the resulting DEM.
 */
static bool is_ground(uint8_t block_id)
{
  return block_id != 0; // TODOs
}

static void strupper(char *out, const char *in) {
  size_t i = 0;
  while(*in != '\0') {
    out[i] = toupper(*in);
    i++;
    in++;
  }
}

static bool streq(const char *str1, const char *str2) {
  return strcmp(str1, str2) == 0;
}

static bool string_starts_with(const char *subject, const char *prefix) {
  return strncmp(subject, prefix, strlen(prefix));
}

// returns -1 if none matched
static int compression_from_string(const char *str) {
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


#define BUF_SIZE 52428800ULL
static uint8_t buf[BUF_SIZE];
static void process_file(FILE *fp)
{
  size_t size = fread(buf, 1, BUF_SIZE, fp);
  if(size == 0)
  {
    fprintf(stderr, "Could not read from file. (%s)\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  if(size < 4096)
  {
    fprintf(stderr, "stdin is not at least 4096 bytes.\n");
    exit(EXIT_FAILURE);
  }
  for(size_t i = 0; i < 4096; i += 4)
  {
    uint32_t offset = 0;
    ((unsigned char *) &offset)[1] = buf[i];
    ((unsigned char *) &offset)[2] = buf[i + 1];
    ((unsigned char *) &offset)[3] = buf[i + 2];
    offset = ntoh32(offset);
    offset *= 4096;

    uint8_t chunk_size = 0;
    chunk_size = buf[i + 3];

    if(offset == 0 && chunk_size == 0) continue; // Chunk hasn't been generated yet.

    if(offset >= size)
    {
      fprintf(stderr, "Corrupt file.\n");
      exit(EXIT_FAILURE);
    }
    uint8_t *chunk_pointer = buf + offset;
    uint32_t chunk_length = 0;
    memcpy(&chunk_length, chunk_pointer, 4);
    chunk_length = ntoh32(chunk_length);
    uint8_t compression_scheme = 0;
    memcpy(&compression_scheme, chunk_pointer + 4, 1);
    if(compression_scheme != 2)
    {
      fprintf(stderr, "Unsupported chunk compression scheme. (%" PRIu8 ")\n", compression_scheme);
      exit(EXIT_FAILURE);
    }

    nbt_node *chunk = nbt_parse_compressed(chunk_pointer + 5, chunk_length);
    if(chunk == NULL)
    {
      fprintf(stderr, "Could not parse chunk NBT. (%s)\n", nbt_error_to_string(errno));
      exit(EXIT_FAILURE);
    }
    handle_chunk(chunk);
    nbt_free(chunk);
  }
}

void print_usage(const char *prog_str) {
  printf(
    "Usage: %s [options] region_file...\n"
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

void print_version(void) {
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
  for(int i = 1; i < argc; i++) {
    if(string_starts_with(argv[i], "-"))
      opts[opts_i++] = argv[i];
    else
      files[files_i++] = argv[i];
  }
  size_t filecount = files_i;
  size_t optscount = opts_i;

  // Print requested information and continue
  for(size_t i = 0; i < optscount; i++) {
    if(streq(opts[i], "--version") || streq(opts[i], "-v"))
      print_version();
    else if(streq(opts[i], "--help") || streq(opts[i], "-h"))
      print_usage(argv[0]);
  }

  // One byte for every block column in a region, 512x512
  image_buf = calloc(filecount * (512*512), 1);
  if(image_buf == NULL)
  {
    fprintf(stderr, "Could not allocate memory. (%s)", strerror(errno));
    exit(EXIT_FAILURE);
  }

  for(size_t i = 0; i < filecount; i++)
  {
    FILE *fp = fopen(files[i], "rb");
    if(fp == NULL)
    {
      fprintf(stderr, "Could not open file '%s'. (%s)", argv[i], strerror(errno));
      exit(EXIT_FAILURE);
    }
    process_file(fp);
    fclose(fp);
  }

  TIFF *tif = TIFFOpen("out.tif", "w");
  if(tif == NULL)
  {
    fprintf(stderr, "Could not open out.tif for writing.");
    // TODO print proper error message
    exit(EXIT_FAILURE);
  }
  register_custom_tiff_tags(tif);
  GTIF *gtif = GTIFNew(tif);
  if(gtif == NULL)
  {
    fprintf(stderr, "Could not open tif as GeoTIFF");
    exit(EXIT_FAILURE);
  }


  // TODO check validity
  // These all start at 1, not 0
  size_t minrow = origin_cartesian_y - max_cartesian_y + 1;
  size_t maxrow = origin_cartesian_y - min_cartesian_y + 1;
  size_t maxcol = max_cartesian_x - origin_cartesian_x + 1;
  size_t mincol = min_cartesian_x - origin_cartesian_x + 1;

  size_t width = maxcol - mincol + 1;
  size_t height = maxrow - minrow + 1;

  // TODO checked integer casts to uint32 from TIFF
  TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
  TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
  TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
  TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
  TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);

  // The number must be an ASCII string.
  TIFFSetField(tif, TIFFTAG_GDAL_NODATA, "0");

  // NOTE: More tags are set after filling in the data below.

  // Write all rows to TIFF.
  // NOTE: uint32 is a typedef from libtiff.
  // It is NOT explicitly guaranteed to be the same as uint32_t.
  // It's used here beccause TIFFWriteScanLine expects the row value to be
  // that type.
  // TODO integer types should be the same or checked
  for(uint32 row = minrow; row <= maxrow; row++)
  {
    /*printf("Width: %i,\n"
        "calculated width: %i,\n"
        "maxcol: %zu,\n"
        "mincol: %zu,\n"
        "Height: %i,\n"
        "calculated height: %i,\n"
        "Maxrow: %i,\n"
        "row: %i,\n"
        "minrow: %i\n\n\n",
        width,
        maxcol - mincol + 1,
        maxcol,
        mincol,
        height,
        maxrow - minrow + 1,
        maxrow,
        row,
        minrow);*/

    // tdata_t is TIFFalese for `typedef void* tdata_t`
    // IMPORTANT: TIFF 'row' seems to start at 0 instead of our 1, thus we subtract 1
    if(TIFFWriteScanline(tif, (tdata_t) (image_buf + imgbuf_rowcol_to_index(row, mincol)), row - 1, 0) != 1)
    {
      fprintf(stderr, "TIFFWriteScanLine returned an error.");
      exit(EXIT_FAILURE);
      // TODO print TIFF error message
    }
  }
  //printf("Done looping!\n");

  // Write GeoTIFF keys..
  // TODO GeoTIFF keys
  // TODO possibility for integer overflows?
  double tiepoints[6] = {0, 0, 0, (double) origin_cartesian_x, (double) origin_cartesian_y, 0.0};
  double pixscale[3] = {1, 1, 1};
  TIFFSetField(tif, TIFFTAG_GEOTIEPOINTS, 6, tiepoints);
  TIFFSetField(tif, TIFFTAG_GEOPIXELSCALE, 3, pixscale);

  GTIFWriteKeys(gtif);
  GTIFFree(gtif);
  TIFFClose(tif);



  return EXIT_SUCCESS;
}

static void output_point(long long cartesian_x, long long cartesian_y, uint8_t height)
{
  long long row = cartesian_x - origin_cartesian_x + 1;
  long long column = origin_cartesian_y - cartesian_y + 1;
  image_buf[imgbuf_rowcol_to_index(row, column)] = height;
}

static int8_t last_section_y = -1;
static void handle_chunk(nbt_node *chunk)
{
  nbt_node *level = nbt_find_by_name(chunk, "Level");
  if(level == NULL)
  {
    fprintf(stderr, "Could not find 'Level' tag in 'Chunk' compound.");
    exit(EXIT_FAILURE);
  }

  nbt_node *x_pos = nbt_find_by_name(level, "xPos");
  if(x_pos == NULL)
  {
    fprintf(stderr, "Could not find 'xPos' tag in 'Chunk' compound.");
    exit(EXIT_FAILURE);
  }
  if(x_pos->type != TAG_INT)
  {
    fprintf(stderr, "'xPos' tag in 'Chunk' compound is not of type TAG_INT.");
    exit(EXIT_FAILURE);
  }

  nbt_node *z_pos = nbt_find_by_name(level, "zPos");
  if(z_pos == NULL)
  {
    fprintf(stderr, "Could not find 'zPos' tag in 'Chunk' compound.");
    exit(EXIT_FAILURE);
  }
  if(z_pos->type != TAG_INT)
  {
    fprintf(stderr, "'zPos' tag in 'Chunk' compound is not of type TAG_INT.");
    exit(EXIT_FAILURE);
  }

  struct chunkpos chunkpos;
  chunkpos.x = x_pos->payload.tag_int;
  chunkpos.z = z_pos->payload.tag_int;

  nbt_node *sections = nbt_find_by_name(level, "Sections");
  if(sections == NULL)
  {
    fprintf(stderr, "Could not find 'Sections' tag in 'Level' compound.");
    exit(EXIT_FAILURE);
  }
  if(sections->type != TAG_LIST)
  {
    fprintf(stderr, "'Sections' tag in 'Level' compound is not of type TAG_LIST.");
    exit(EXIT_FAILURE);
  }

  nbt_map(sections, handle_section, NULL);

  for(size_t i = 0; i < 256; i++)
  {
    // Outputs point at absolute cartesian coordinates, so the Minecraft z is now called y and is inverted
    output_point(chunkpos.x * 16 + i % 16, 0 - (chunkpos.z * 16 + i / 16), current_chunk_heightmap[i]);
  }

  // Update filled-in data bounds
  long long llchunkx = (long long) chunkpos.x;
  long long llchunkz = (long long) chunkpos.z;
  long long new_max_cartesian_x = llchunkx * 16 + 15;
  long long new_min_cartesian_x = llchunkx * 16;
  long long new_max_cartesian_y = 0 - (llchunkz * 16 + 15);
  long long new_min_cartesian_y = 0 - llchunkz * 16;
  if(new_max_cartesian_x > max_cartesian_x) max_cartesian_x = new_max_cartesian_x;
  if(new_min_cartesian_x < min_cartesian_x) min_cartesian_x = new_min_cartesian_x;
  if(new_max_cartesian_y > max_cartesian_y) max_cartesian_y = new_max_cartesian_y;
  if(new_min_cartesian_y < min_cartesian_y) min_cartesian_y = new_min_cartesian_y;

  // Reset current chunk heightmap
  memset(current_chunk_heightmap, 0, 256);
  last_section_y = -1;
}


static bool handle_section(nbt_node *section, unused_ void *aux)
{
  if((section->name != NULL &&
      strcmp(section->name, "Sections") == 0) ||
      section->type != TAG_COMPOUND) return true; // Ignore the root element

  nbt_node *section_y_nbt = nbt_find_by_name(section, "Y");
  if(section_y_nbt == NULL)
  {
    fprintf(stderr, "Could not find 'Y' tag in chunk section.\n");
    exit(EXIT_FAILURE);
  }
  if(section_y_nbt->type != TAG_BYTE)
  {
    fprintf(stderr, "'Y' tag in chunk section is not of type TAG_BYTE.\n");
    exit(EXIT_FAILURE);
  }
  int8_t section_y = section_y_nbt->payload.tag_byte;
  if(section_y <= last_section_y)
  {
    return true;
  }
  else
  {
    last_section_y = section_y;
  }
  nbt_node *blocks = nbt_find_by_name(section, "Blocks");
  if(blocks == NULL)
  {
    fprintf(stderr, "Could not find 'Blocks' tag in chunk section.\n");
    exit(EXIT_FAILURE);
  }
  else if(blocks->type != TAG_BYTE_ARRAY)
  {
    fprintf(stderr, "'Blocks' tag in chunk section is not of type TAG_BYTE_ARRAY.\n");
    exit(EXIT_FAILURE);
  }
  else if(blocks->payload.tag_byte_array.length != 4096)
  {
    fprintf(stderr, "'Blocks' byte array length is not 4096.\n");
    exit(EXIT_FAILURE);
  }
  for(int y = 15; y >= 0; y--)
  {
    uint_fast8_t current_y = section_y * 16 + y;
    for(uint_fast16_t j = 0; j < 256; j++)
    {
      uint8_t current_block_id = (uint8_t) blocks->payload.tag_byte_array.data[y * 255 + j];

      if(is_ground(current_block_id) && current_chunk_heightmap[j] < current_y)
      {
        current_chunk_heightmap[j] = current_y;
      }
    }
  }
  return true;
}

