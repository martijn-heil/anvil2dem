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
#include <errno.h>
#include <inttypes.h>
#include <assert.h>

#include <arpa/inet.h>

#include <nbt/nbt.h>

#include "utils.h"
#include "parseregion.h"


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


static bool handle_section(nbt_node *section, void *aux);
static void handle_chunk(nbt_node *chunk,
    long long *max_cartesian_x,
    long long *min_cartesian_x,
    long long *max_cartesian_y,
    long long *min_cartesian_y,
    output_point_func_t output_point,
    void *output_point_aux);

static is_ground_func_t is_ground_func;

// buf size should be at least 4096.
// 'size' is the amount of available bytes in buf, thus it should be at least 4096.
void parse_region(const uint8_t *buf, const size_t size,
    long long *out_max_cartesian_x,
    long long *out_min_cartesian_x,
    long long *out_max_cartesian_y,
    long long *out_min_cartesian_y,
    output_point_func_t output_point_func,
    void *output_point_aux,
    is_ground_func_t loc_is_ground_func)
{
  assert(size >= 4096);
  assert(out_max_cartesian_x != NULL);
  assert(out_min_cartesian_x != NULL);
  assert(out_max_cartesian_y != NULL);
  assert(out_min_cartesian_y != NULL);
  assert(output_point_func != NULL);
  assert(loc_is_ground_func != NULL);

  is_ground_func = loc_is_ground_func;
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
    const uint8_t *chunk_pointer = buf + offset;
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
    handle_chunk(chunk,
      out_max_cartesian_x,
      out_min_cartesian_x,
      out_max_cartesian_y,
      out_min_cartesian_y,
      output_point_func,
      output_point_aux);
    nbt_free(chunk);
  }
}

// Variables used in handle_chunk and handle_section
static uint8_t current_chunk_heightmap[256];
static int8_t last_section_y = -1;

static void handle_chunk(nbt_node *chunk,
    long long *max_cartesian_x,
    long long *min_cartesian_x,
    long long *max_cartesian_y,
    long long *min_cartesian_y,
    output_point_func_t output_point,
    void *output_point_aux)
{
  assert(max_cartesian_x != NULL);
  assert(min_cartesian_x != NULL);
  assert(max_cartesian_y != NULL);
  assert(min_cartesian_y != NULL);
  assert(output_point != NULL);
  assert(chunk != NULL);

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
    long long minecraft_z = chunkpos.z * 16 + i / 16;
    long long cartesian_x = chunkpos.x * 16 + i % 16;
    long long cartesian_y = 0 - minecraft_z - 1;
    // Outputs point at absolute cartesian coordinates, so the Minecraft z is now called y and is inverted
    output_point(cartesian_x, cartesian_y, current_chunk_heightmap[i], output_point_aux);
  }

  // Update filled-in data bounds
  long long llchunkx = (long long) chunkpos.x;
  long long llchunkz = (long long) chunkpos.z;
  long long new_max_cartesian_x = llchunkx * 16 + 15;
  long long new_min_cartesian_x = llchunkx * 16;
  long long new_max_cartesian_y = 0 - (llchunkz * 16 + 15);
  long long new_min_cartesian_y = 0 - llchunkz * 16;
  if(new_max_cartesian_x > *max_cartesian_x) *max_cartesian_x = new_max_cartesian_x;
  if(new_min_cartesian_x < *min_cartesian_x) *min_cartesian_x = new_min_cartesian_x;
  if(new_max_cartesian_y > *max_cartesian_y) *max_cartesian_y = new_max_cartesian_y;
  if(new_min_cartesian_y < *min_cartesian_y) *min_cartesian_y = new_min_cartesian_y;

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
      uint8_t current_block_id = (uint8_t) blocks->payload.tag_byte_array.data[y * 256 + j];

      if(is_ground_func(current_block_id) && current_chunk_heightmap[j] < current_y)
      {
        current_chunk_heightmap[j] = current_y;
      }
    }
  }
  return true;
}
