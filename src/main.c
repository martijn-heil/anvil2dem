/*
  MIT License

  Copyright (c) 2017-2019 Martijn Heil

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

    nin-anvil - Generate a heightmap from a .mca file.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

#include <arpa/inet.h>

#include <nbt/nbt.h>

#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))

#define hton16(x) htons(x)
#define hton32(x) htonl(x)
#define hton64(x) htonll(x)

#define ntoh16(x) ntohs(x)
#define ntoh32(x) ntohl(x)
#define ntoh64(x) ntohll(x)

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
#define imgbuf_rowcol_to_index(row, col) ((row-1) * 16 + col - 1)

uint8_t current_chunk_heightmap[256];
static uint8_t *image_buf; // Gets allocated in main
static long long origin_cartesian_x, origin_cartesian_y;

static bool is_ground(uint8_t block_id)
{
  return block_id != 0; // TODOs
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

int main(int argc, char *argv[])
{
  image_buf = malloc((argc - 1) * 262144); // 262144: One byte for every block column in a region, 512x512
  if(image_buf == NULL)
  {
    fprintf(stderr, "Could not allocate memory. (%s)", strerror(errno));
    exit(EXIT_FAILURE);
  }

  for(int i = 1; i < argc; i++)
  {
    FILE *fp = fopen(argv[i], "rb");
    if(fp == NULL)
    {
      fprintf(stderr, "Could not open file '%s'. (%s)", argv[i], strerror(errno));
      exit(EXIT_FAILURE);
    }
    process_file(fp);
    fclose(fp);
  }

  TIFF *tif = TIFFOpen("out.tif", "w");
  if(out == NULL)
  {
    fprintf(stderr, "Could not open out.tif for writing.");
    // TODO print proper error message
    exit(EXIT_FAILURE);
  }
  GTIF *gtif = GTIFNew(tif)
  if(gtif == NULL)
  {
    fprintf(stderr, "Could not open tif as GeoTIFF");
    exit(EXIT_FAILURE);
  }
  TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, imgbuf_width);
  TIFFSetField(tif, TIFFTAG_IMAGEHEIGHT, imgbuf_height);
  // TODO write other tags



  // Write all rows to TIFF.
  // NOTE: uint32 is a typedef from libtiff.
  // It is NOT guaranteed to be the same as uint32_t.
  // It's used here beccause TIFFWriteScanLine expects the row value to be
  // that type.
  for(uint32 row = 1; row <= imgbuf_width; row++)
  {
    // tdata_t is `typedef void* tdata_t`
    if(TIFFWriteScanLine(tif, (tdata_t) (imgbuf + (row-1)*imgbuf_width), row, 0) != 1)
    {
      fprintf(stderr, "TIFFWriteScanLine returned an error.");
      exit(EXIT_FAILURE);
      // TODO print TIFF error message
    }
  }

  // Write GeoTIFF keys..
  // TODO GeoTIFF keys
  GTIFWriteKeys(gtif);
  GTIFFree(gtif);
  TIFFClose(tif);



  return EXIT_SUCCESS;
}

long long max_cartesian_x = 0;
long long min_cartesian_x = 0;
long long max_cartesian_y = 0;
long long min_cartesian y = 0;
static void output_point(long long cartesian_x, long long cartesian_y, uint8_t height)
{
  long long row = cartesian_x - origin_cartesian_x + 1;
  long long column = origin_cartesian_y - cartesian_y + 1;
  image_buf[imgbuf_rowcol_to_index(row, columm)] = height;

  if(cartesian_x > max_cartesian_x) max_cartesian_x = cartesian_x;
  if(cartesian_x < min_cartesian_x) min_cartesian_x = cartesian_x;
  if(cartesian_y > max_cartesian_y) max_cartesian_y = cartesian_y;
  if(cartesian_y < min_cartesian_y) min_cartesian_y = cartesian_y;
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
    // Outputs point at real mc world cartesian coordinates, so the z is now called y and is inverted
    output_point(chunkpos.x * 16 + i % 16, 0 - (chunkpos.z * 16 + i / 16), current_chunk_heightmap[i]);
  }

  memset(current_chunk_heightmap, 0, 256);
  last_section_y = -1;
}

static bool handle_section(nbt_node *section, void *aux)
{
  if((section->name != NULL && strcmp(section->name, "Sections") == 0) || section->type != TAG_COMPOUND) return true; // Ignore the root element
  // printf("name: %s\n", section->name);
  // char *dump = nbt_dump_ascii(section);
  // printf("%s\n", dump);
  //exit(EXIT_SUCCESS);

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
