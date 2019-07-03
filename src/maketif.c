#include <stddef.h> // for size_t
#include <xtiffio.h>
#include <geotiffio.h>

// See https://stackoverflow.com/questions/24059421
// And see https://www.asmail.be/msg0054699392.html
// This is horribly documented
#define TIFFTAG_GDAL_NODATA 42113
static const TIFFFieldInfo tiff_field_info[] = {
  { TIFFTAG_GDAL_NODATA, 1, 1, TIFF_ASCII, FIELD_CUSTOM, true, false, "GDAL_NODATA" }
};


static void register_custom_tiff_tags(TIFF *tif) {
  TIFFMergeFieldInfo(tif, tiff_field_info, sizeof(tiff_field_info) / sizeof(tiff_field_info[0]));
}


// Assumes that row and column start at 1, not at 0
#define rowcol_to_index(row, col) ((row-1) * 16 + col - 1)


// TODO implement Result struct
struct Result maketif(
    const char *filepath,
    const void *buf,
    const long long max_cartesian_x,
    const long long min_cartesian_x,
    const long long max_cartesian_y,
    const long long min_cartesian_y)
{
  TIFF *tif = XTIFFOpen(filepath, "w");
  if(tif == NULL)
  {
    fprintf(stderr, "Could not open out.tif for writing.");
    // TODO print proper error message
    exit(EXIT_FAILURE);
  }
  GTIF *gtif = GTIFNew(tif);
  if(gtif == NULL)
  {
    fprintf(stderr, "Could not open tif as GeoTIFF");
    exit(EXIT_FAILURE);
  }

  // Integer rounding is on purpose
  const long long origin_cartesian_x = min_cartesian_x / 16 / 32 * 32 * 16;
  const long long origin_cartesian_y = max_cartesian_y / 16 / 32 * 32 * 16;

  // These all start at 1, not 0
  // Note that TIFF rows start at 0 instead of 1
  const size_t minrow = origin_cartesian_y - max_cartesian_y + 1;
  const size_t maxrow = origin_cartesian_y - min_cartesian_y + 1;
  const size_t maxcol = max_cartesian_x - origin_cartesian_x + 1;
  const size_t mincol = min_cartesian_x - origin_cartesian_x + 1;

  const size_t width = maxcol - mincol + 1;
  const size_t height = maxrow - minrow + 1;

  // TODO checked integer casts to uint32 from TIFF
  TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
  TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
  TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
  TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
  TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);



  // NOTE: More tags are set after filling in the data below.

  // Write all rows to TIFF.
  // NOTE: uint32 is a typedef from libtiff.
  // It is NOT explicitly guaranteed to be the same as uint32_t.
  // It's used here beccause TIFFWriteScanLine expects the row value to be
  // that type.
  // TODO integer types should be the same or checked
  for(uint32 row = minrow; row <= maxrow; row++)
  {
    uint32 tiffrow = row - minrow;

    // tdata_t is TIFFalese for `typedef void* tdata_t`
    // IMPORTANT: TIFF 'row' seems to start at 0 instead of our 1, thus we subtract 1
    if(TIFFWriteScanline(tif, (tdata_t) (buf + rowcol_to_index(row, mincol)), tiffrow, 0) != 1)
    {
      fprintf(stderr, "TIFFWriteScanLine returned an error.");
      exit(EXIT_FAILURE);
      // TODO print TIFF error message
    }
  }

  // Write GeoTIFF keys..
  // TODO GeoTIFF keys
  // TODO possibility for integer overflows?
  // TODO origin is wrong
  double tiepoints[6] = {0, 0, 0, (double) min_cartesian_x, (double) max_cartesian_y, 0.0};
  double pixscale[3] = {1, 1, 1};
  TIFFSetField(tif, TIFFTAG_GEOTIEPOINTS, 6, tiepoints);
  TIFFSetField(tif, TIFFTAG_GEOPIXELSCALE, 3, pixscale);

  GTIFWriteKeys(gtif);
  GTIFFree(gtif);

  register_custom_tiff_tags(tif);
  TIFFSetField(tif, TIFFTAG_GDAL_NODATA, "0"); // The number must be an ASCII string.

  XTIFFClose(tif);
}
