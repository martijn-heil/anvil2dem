# nin-anvil
Program that generates a Digital Elevation Model (DEM) from a Minecraft Anvil world region file in the form of a georeferenced GeoTIFF.

```
Usage: anvil2dem [options] region_file
Options:
  -h, --help                Show this usage information.
  -v, --version             Show version information.
  --blocks=<file>           List of blocks that should be taken into account.
  --ignoredblocks=<file>    List of blocks that should NOT be taken into account.
  --compression=<scheme>    TIFF compression scheme, defaults to DEFLATE.

scheme is case-insensitive and can be one of the following values:
NONE, CCITTRLE, CCITTFAX3, CCITTFAX4, LZW, OJPEG, JPEG, NEXT, CCITTRLEW, PACKBITS, THUNDERSCAN, IT8CTPAD, IT8LW, IT8MP, IT8BL, PIXARFILM, PIXARLOG, DEFLATE, ADOBE_DEFLATE, DCS, JBIG, SGILOG, SGILOG24, JP2000
```

This enables you to make some things using standard GIS software, like some examples shown below.
![screenshot](pictures/Screenshot_20211012_133702.png)
![screenshot](pictures/Screenshot_20211012_193750.png)
![screenshot](pictures/Screenshot_20211012_193854.png)
