/* ---------------------------------------------------------------------- */
/* tileset.h : Header with cross-platform "tgxm" tileset file definitions */
/* ---------------------------------------------------------------------- */
/* The file format and content specifications in this file were developed
   by Geodyssey Limited of Calgary, Alberta, Canada. They are provided
   to application developers free of charge and free of copyright
   restriction in the hope that they will enable the development of
   robust cross-platform geographical applications. Attribution of this
   design to Geodyssey Limited at www.geodyssey.com would be appreciated.
 */

#ifndef TILESET_H
#define TILESET_H 1

/* The suffix 'tgxm' suggests tiled, geo-referenced, compressed bitmaps.
   To facilitate cross-platform use, file names must be less than 32
   characters, no blanks, first character alpha, the rest may be
   alphabetics, numerals, and the underscore '_' only. */

#define TS_FILE_BIG_ENDIAN      "tgxm"
#define TS_FILE_LITTLE_ENDIAN   "mxgt"
#define TS_FILE_SUFFIX          ".tgxm"     /* always tgxm, all lowercase */

#define TS_IMAGE_ZLIB         1
#define TS_IMAGE_JPEG         2
#define TS_IMAGE_ZLIB_JPEG    3

#define TS_PROJ_RECTANGULAR   1
#define TS_PROJ_AZIMUTAL      2

#define TS_COLOR_BLACKWHITE   1
#define TS_COLOR_PALETTE256   2
#define TS_COLOR_RGB          3

/* All integers stored in the file are 4 bytes wide; unsigned characters
   one byte. Headers (where integers occur, as opposed to the "body" of
   the compressed map image) are stored twice - in both big and little
   endian "mirrors"; so that the file can be easily processed in read-only
   memory-mapped mode on either platform. The order of headers is not
   prescribed; only that both MUST be present. Applications using the file
   can locate the preferred header block by checking the file "stamp":
   it must be "tgxm" for big-endian, "mxgt" for little-endian. If the
   first header block is not of the required endianess, the length to be
   skipped can be computed using the data in the first block and the
   TS_I4_RVRS(i) macro.

   All structures are tightly packed.

   To avoid storing floating point numbers in the file, angles in radians
   in the range of -PI < a < +PI are packed into 4-byte integers, with
   a resolution of about 1 cm on the Earth's surface. Thus a program on
   any platform, with any internal floating point format, can derive
   all angular (latitude/longitude) measurements from/to the integers
   stored in the file using TS_PI and the macros following it.

   It is assumed that an application working with "rectangular projection"
   tilesets will keep all longitudes relative to the central meridian of
   the tileset, and convert the values on input or output to the meridian
   of the user's choice (Greenwich, Paris, Pulkovo, Mecca or whatever)
   using the tileset header value in tsRect.centerLng - which is the
   tileset central meridian relative (-PI <-> +PI) to the Greenwich
   meridian. This will get around the necessity to constantly deal with
   the cyclic nature of longitude measure: trivial when computations are
   performed using real numbers, but awkward when using integer measure
   that employs nearly the full internal representation range.
 */

#define TS_PI            3.14159265358979323846
#define TS_PIHALF        1.57079632679489661923

#define TS_I4A_UNDEF      -2147483647
#define TS_I4A_SCALE_INT   2147483640
#define TS_I4A_SCALE_DBL   2147483640.0

#define TS_AngOfI4(i)  ((((double)(i)) / TS_I4A_SCALE_DBL) * TS_PI)
#define TS_I4OfAng(r)  ((int)(((r) / TS_PI) * TS_I4A_SCALE_DBL))

/* Following is only for reporting convenience (no file items use this): */
#define TS_DEG2RAD  (TS_PI / 180.0)
#define TS_RAD2DEG  (180.0 / TS_PI)

#define TS_DRN(l)          (1.000000001 * TS_RAD2DEG * TS_AngOfI4(l))  /* degrees */
#define TS_DAB(l)          (TS_DRN(abs(l)))                   /* absolute */
#define TS_DAW(l)          ((double)((int)TS_DAB(l))) /* absolute whole deg */

#define TS_SIGN_LAT(l)      ((l < 0) ? 's' : 'n')
#define TS_SIGN_LNG(l)      ((l < 0) ? 'w' : 'e')
#define TS_INTDEG_LL(l)    ((int)(TS_DAB(l))) /* unsigned integer degrees */
#define TS_DECMIN_LL(l)    (60.0 * ((TS_DAB(l) - TS_DAW(l)))) /* decimal minutes, to follow */

/* For GPS stream manipulations */
#define TS_MIN2RAD  (TS_PI / 10800.0)
#define TS_FUZZA      4.e-11            /* in radians, corresponding to Earth surface millimeter */

/* Defined here for convenience, as it might be common to different
   programs passing a location obtained from a GPS device in a common,
   concise form: */

struct tsGpsLocation {                         /* location/elevation/time */
   int lat;                                          /* scaled integer... */
   int lng;                                       /* ...cf. TS_I4OfAng(r) */
   int elevation;                    /* centimeters above/below ellipsoid */
   unsigned time;              /* seconds after TS_EPOCH (up to AD. 2136) */
   };

/* For reversing order of bytes in a 4-byte integer */
#define TS_I4_RVRS(i) (((i) << 24) | (((unsigned)(i)) >> 24) | (((i) & 0x0000ff00) << 8) | (((i) & 0x00ff0000) >> 8))

/* Macro to swap red and green byte in a 24-bit pixel. (Win32 is BGR) */
#define TS_SWAP_RB(p)  {(*p)^=(*(p+2)); (*(p+2))^=(*p); (*p)^= *(p+2);}

/* Macros to produce a simple "human-readable" decimal degree latitude
   and longitude, given scaled integers in radian measure */
#define TS_DECDEG_LAT(l)      (TS_RAD2DEG * TS_AngOfI4(l))   /* always global */
#define TS_DECDEG_LNG1(l)     (TS_RAD2DEG * TS_AngOfI4(l))    /* global or... */
#define TS_DECDEG_LNG2(l0,ld) (TS_RAD2DEG * TS_AngOfI4(ts_LongitudeGlobal(l0, ld))) /* ...tileset center and delta */

/* Various utility macros: */

/* Tileset width, height in pixels, given pointer to tileset header */
#define TS_PX_WIDTH(p)  ((p)->tileColumns * (p)->tileWidth)
#define TS_PX_HEIGHT(p) ((p)->tileRows * (p)->tileHeight)

/* Undefined pixel coordinate value flag */
#define TS_PIX_UNDEF       -2147483647

/* File Structure and Geometry:
   ----------------------------
   In brief, a tgxm file is composed of:

   File header (one tsHdr structure)
   Geometry header (one tsRect structure)
   Row table (tileRows + 1 integers)
   Tile table (tileTableSize * tsTile1 or tsTile2 structures)
   Mirror of all of the above, in the opposite endianess.
   Palette (optional, 256 tsColor structures)
   Transparency color (optional, only for IMAGE_ZLIB_JPEG images)
   Descriptions
   Pixel data

   A file starts with the tsHdr file header structure. Next (in the case of
   the "rectangular" projection, the only one implemented so far) follows
   one tsRect structure, immediately followed by tileRows + 1 four-byte
   integers, ordered from South to North ('up'), defining the South limit
   of each row in latitude on the ground and, as the last value in the
   table, the North limit of the northernmost row.

   Next are tsTile1 (for TS_IMAGE_ZLIB or TS_IMAGE_JPEG types) or
   tsTile2 (for TS_IMAGE_ZLIB_JPEG type) structures, in "rectangular
   Morton" order.

   Next (in the case of colorDepth == TS_COLOR_PALETTE256) are
   256 tsPalette entries.

   Next (in the case of  imageType== TS_IMAGE_ZLIB_JPEG) is one
   tsPalette entry, defining the transparent color of the lossless
   layer.

   Next are two short, fixed length description lines.

   Following are tileColumns x tileRows number of compressed tile
   pixel blocks; their individual sizes and offsets from the file
   start are given in the respective tsTile structure.

   Pixels lines are ordered opposite than tile rows: tile rows start
   at the South boundary of the set whereas pixel lines start at the
   North boundary of the tile.

   "Rectangular" projection geometry specifications:

   Given the number of tile columns, together with the tileset range
   of longitude, determine the tile longitude width, which will be the
   same for all tiles in the set.

   Each set has at least one row of tiles immediately above and
   one immediately below the mid-latitude parallel. Each tile North
   of the set mid-latitude has an extent in latitude such that its
   "height" (North/South extent) on the ground is equal to the ground
   length of its south bounding parallel, and each tile South of the
   set mid-latitude has an extent in latitude such that its latitude
   extent is equal to the length of its North bounding parallel.

   As many rows North and South of the set mid-latitude will be
   required as are necessary to reach the North and South limits
   of the tileset. Note that these two numbers might not be equal.

   This method will produce a "near-conformal" map image; yet it
   is very simple to implement. All the geodetic computations required
   consist of the determination of the circumference of a given
   parallel of latitude and the length of the meridian arc between two
   given latitudes. This in turn makes it easy for various programs
   to produce tilesets with the same geometry, given the same
   longitude range, number of tile columns and mid-latitude.
   (If the precision requirements are only moderate, even spherical
   Earth computations might yield tilesets with servicable geometry.
   However, such tilesets could not be considered to conform to the
   TileShare "Rectangular Projection" standard).

   Tile order:
   -----------
   The tile row table is in South to North order. If the number of rows
   or columns is not an even power of two, the tile index table will
   be somewhat sparse, but can be entered directly using tile column
   and row as arguments. Tiles in the file are stored in neither row-
   nor column-major order, but rather in a "rectangular Morton" order.
   (cf. ts_Morton()) - as the spatial clustering of tile pixel blocks
   in the file is the most important design parameter. The offset and
   size values in the unused index table slots are both set to xffffffff
   and can be ignored. The total size of the table is given explicitly
   in the tileTableSize header structure item.

   Tile size:
   ----------
   The present implementation of the utility functions assumes that the
   tile will have less than 64K pixels in either dimension. While there
   is no such limitation built into the file definition, it is probably
   reasonable to stick to this limit; especially since it is unlikely
   that tiles larger than this would be of much practical use.

   Color and tileset levels:
   -------------------------
   Lossless compression is based on the zlib library which is well suited
   for maps. Lossy compression is based on the JPEG library which is best
   avoided as a single-layer (Level 2, see below) tileset, but is a
   good coice for maps with shaded background in level 3 tilesets.
   For efficient lossless compression it is probably worth reducing the
   number of colors of the original scans.

   Tilesets can be of three levels, in increased order of sophistication
   and programming complexity:

   Level 1: Single layer, lossless (zlib compressed)
   Level 2: Single layer, lossy (JPEG library compressed)
   Level 3: Combined background and linework layers.

   The first level is the most common one and is the only that should be
   assumed to be supported by all applications working with tgxm files.
   The second level is similar, except that it is using JPEG library
   compression. The third type consists of two layers: one for the map
   background with a continuously changing color, the other for the
   linework and labels. A linework layer is assumed to have a transparent
   color. Such bitmaps could only be created from the scanned material
   with some rather involved image processing. The combined layer type
   is therefore to be considered as 'experimental'.

   colorDepth applies only to the lossless layer; JPEG layer is always RGB.

   File Stamp:
   -----------
   If the file stamp is created as:
   tsHdr.fileStamp = 16777216 * 't' + 65536 * 'g' + 256 * 'x' + 'm'
   it will be "tgxm" for the big-endian header, "mxgt" for the
   little-endian preamble.

   Description:
   ------------
   All creators are required to place two short lines of descriptive
   data in the file. Note however that the access to all data following
   the description is based on the offsets in the tile table; thus
   any extraneous data between the description and first tile (always
   tile 0 in the tile table) has no negative impact on the file
   integrity, only on its size.
 */

struct tsHdr {                                    /* Tile-set file header */
   int fileStamp;                          /* filestamp, "tgxm" or "mxgt" */
   int reserved;                                              /* always 0 */
   int id;                       /* creator or user controlled identifier */
   int projection;                    /* TS_PROJ_RECTANGULAR only for now */
   int tileWidth, tileHeight;                                /* in pixels */
   int tileColumns, tileRows;                    /* tile rows and columns */
   int colorDepth;                           /* TS_COLOR_... (1, 2, or 3) */
   int imageType;                           /* TS_IMAGE_ZLIB only for now */
   };                                                /* 10 ints, 40 bytes */

struct tsRect {      /* rectangular set sub-header, row table must follow */
   int centerLat;    /* set center latitude, -PI/2 <-> +PI/2 from Equator */
   int centerLng;  /* longitude, Greenwich meridian relative, -PI <-> +PI */
   int rangeLat, rangeLng;         /* set range in latitude and longitude */
   int lowLatY;       /* tileset coordinate corresponding to low latitude */
   int highLatY;                       /* as above, tileset high latitude */
   };              /* 6 ints, 24 bytes, followed immediately by row table */

struct tsTile1 {                                           /* Tile header */
   int dataStart;  /* compressed block offset from file start or ffffffff */
   int dataLength;   /* size in bytes, negative: uncompressed, 0: no tile */
   };                                                  /* 2 ints, 8 bytes */

struct tsTile2 {                                           /* Tile header */
   int dataStartJ;        /* JPEG compressed block offset from file start */
   int dataLengthJ; /* size of the above in bytes, negative: uncompressed */
   int dataStartZ;        /* zlib compressed block offset from file start */
   int dataLengthZ; /* size of the above in bytes, negative: uncompressed */
   };                                                  /* 4 ints, 8 bytes */

struct tsColor {                /* palette (always 256 color) table entry */
   unsigned char red;
   unsigned char green;
   unsigned char blue;
   unsigned char reserved;                                    /* always 0 */
   };                                                          /* 4 bytes */

struct tsTrans {                                    /* Transparency color */
   unsigned char redTransparent;                /* for IMAGE_ZLIB_JPEG... */
   unsigned char greenTransparent;                      /* lossless layer */
   unsigned char blueTransparent;                    /* transparent color */
   unsigned char reserved;                                    /* always 0 */
   };                                                          /* 4 bytes */

struct tsDescr {                                           /* description */
   char descr_l1[32];                   /* ascii text description, line 1 */
   char descr_l2[32];                   /* ascii text description, line 2 */
   };                                              /* 64 (at least) bytes */

/* Structures representing global point and tileset tile/pixel location:  */

struct tsLatLng {         /* Global (or tileset) latitude/longitude: */
   int lat;               /* integer-scaled radian, -PI/2 <-> +PI/2 range */
   int lng;        /* as above, usually "normalized" to -PI <-> +PI range */
   };                                                  /* 2 ints, 8 bytes */

/* All values in the following are non-negative, but signed integers are
   used as an application is likely to use them in integer additions and
   subtractions.
 */

struct tsPixel {              /* tileset pixel x,y from South-West corner */
   int pxl_x;                                             /* West to East */
   int pxl_y;                                           /* South to North */
   };                                                 /* 4 ints, 16 bytes */

struct tsTilePixel {        /* tileset "coordinates" (counts 0-relative): */
   int tileColumn;                                        /* West to East */
   int tileRow;                                         /* South to North */
   int pxl_x;                                             /* West to East */
   int pxl_y;                                           /* South to North */
   };                                                 /* 4 ints, 16 bytes */

/* Tileset mapping and utility functions, in tileset.c: */
int ts_PixelToTilePixel(const struct tsHdr *, const struct tsPixel *, struct tsTilePixel *);
int ts_MapRect(const struct tsHdr *, const struct tsRect *, const struct tsLatLng *, struct tsTilePixel *);
int ts_UnMapRect(const struct tsHdr *, const struct tsRect *, const struct tsTilePixel *, struct tsLatLng *);
int ts_PixelAdd(const struct tsHdr *, const struct tsRect *, const struct tsTilePixel *, int, int, struct tsTilePixel *);
int ts_LongitudeGlobal(int, int);
int ts_LongitudeTileset(int, int);
int ts_IntRat2(int, int, int);
int ts_IntRat3(int, int, int, int);
int ts_Morton(int, int, int, int);

#endif
