/* tileset.c --- Tileset mapping and utility library. */

/* This C language source code was developed by Geodyssey Limited of
   Calgary, Alberta, Canada. It is provided to application developers
   free of charge and free of copyright restriction with the hope that it
   will provide C language programming examples to be used in development
   of platform-independent geographical applications utilizing a common
   scanned map image file format, as described in tileset.h. The content
   of this file may be freely used and/or modified. Attribution of this
   code to Geodyssey Limited at www.geodyssey.com would be appreciated.
 */

#include "tileset.h"
#include <stdlib.h>                                            /* for div() */
/* ======================================================================== */
/* Given pixel coordinates measured North and East from the South-West
   corner of the whole tileset, return tile column and row and pixel
   coordinates in the tile. Return 1 (and leave the result undefined)
   if the pixel coordinates are outside the tileset.
 */
int ts_PixelToTilePixel(const struct tsHdr *tsh,
                        const struct tsPixel *tspx,
                        struct tsTilePixel *tstp) {
   div_t dv;
/* ------------------------------------------------------------------------ */
   if ((tspx->pxl_x < 0) || (tspx->pxl_x >= TS_PX_WIDTH(tsh))) return(1);
   if ((tspx->pxl_y < 0) || (tspx->pxl_y >= TS_PX_HEIGHT(tsh))) return(1);

   dv = div(tspx->pxl_x, tsh->tileWidth);
   tstp->tileColumn = dv.quot;
   tstp->pxl_x = dv.rem;

   dv = div(tspx->pxl_y, tsh->tileHeight);
   tstp->tileRow = dv.quot;
   tstp->pxl_y = dv.rem;
   return(0);
   }
/* ======================================================================== */
/* Given global point in integer-scaled latitude/longitude radians, find
   tile column/row and x/y in-tile pixel offsets. Note that the longitude
   is relative to the tileset central meridian. Arithmetic is in integers.
   Returns 1 if the point is outside the tileset coverage; 0 otherwise.
 */
int ts_MapRect(const struct tsHdr *tsh,                   /* tileset header */
               const struct tsRect *tsrp,         /* rectangular projection */
               const struct tsLatLng *gp,          /* given global lat/long */
               struct tsTilePixel *tp) {          /* returned tileset pixel */
   int i;
   int *tsrow;                                                 /* row table */
   int lrh;                                  /* half of the longitude range */
   int l;
   div_t dv;
/* ------------------------------------------------------------------------ */
   tsrow = (int *)(tsrp + 1);
/* debugTrace("Global location: %d, %d", gp->lat, gp->lng);*/
/* debugTrace("Tileset l/h lat: %d, %d", tsrow[0], tsrow[tsh->tileRows]);*/

   if ((gp->lat < tsrow[0]) || (gp->lat > tsrow[tsh->tileRows])) return(1);

   lrh = tsrp->rangeLng / 2;
   if ((gp->lng < -lrh) || (gp->lng > lrh)) return(1);

/* OK, global location is ~inside~ the tileset */

/* Latitude: for now we brute-force it: check all the (uneven!) rows. */
   for (i = 0; i < tsh->tileRows; i++) {
      if (gp->lat < tsrow[i + 1]) break;
      }
   if (i == tsrow[tsh->tileRows]) i--;     /* must be top-row, top-boundary */
   tp->tileRow = i;
   tp->pxl_y = ts_IntRat2(tsh->tileHeight, gp->lat - tsrow[i], tsrow[i + 1] - tsrow[i]);

/* Longitude: simple modulo artihmetic. First find pixel count from left edge: */
   l = ts_IntRat3(tsh->tileWidth, tsh->tileColumns, lrh + gp->lng, tsrp->rangeLng);

   if ((l < 0) || (l >= tsh->tileColumns * tsh->tileWidth)) return(1);
   dv = div(l, tsh->tileWidth);
   tp->tileColumn = dv.quot;
   tp->pxl_x = dv.rem;
   return(0);
   }
/* ======================================================================== */
/* Given tile column/row and x/y in-tile pixel offsets, find global point
   in integer-scaled latitude/longitude radians. Longitude is relative to
   the central meridian of the tileset, NOT to the tileset mid-longitude
   in conventional (for instance, Greenwich) meridian. Arithmetic is in
   integers. Returns 1 in case of invalid ~tp~ coordinates, 0 otherwise.
 */
int ts_UnMapRect(const struct tsHdr *tsh,                 /* tileset header */
                 const struct tsRect *tsrp,       /* rectangular projection */
                 const struct tsTilePixel *tp,       /* given tileset pixel */
                 struct tsLatLng *gp) {         /* returned global lat/long */
   int *tsrow;                                                 /* row table */
/* ------------------------------------------------------------------------ */
   tsrow = (int *)(tsrp + 1);

   if ((tp->tileRow < 0) || (tp->tileRow >= tsh->tileRows)) return(1);
   gp->lat = tsrow[tp->tileRow];
   if ((tp->pxl_y < 0) || (tp->pxl_y >= tsh->tileHeight)) return(1);
   gp->lat += ts_IntRat2(tp->pxl_y, tsrow[tp->tileRow + 1] - tsrow[tp->tileRow], tsh->tileHeight);

   if ((tp->tileColumn < 0) || (tp->tileColumn >= tsh->tileColumns)) return(1);
   gp->lng = ts_IntRat2(tsrp->rangeLng, tp->tileColumn * tsh->tileWidth + tp->pxl_x, tsh->tileColumns * tsh->tileWidth) - tsrp->rangeLng / 2;

   return(0);
   }
/* ======================================================================== */
/* Given tile/pixel coordinate and x/y pixel delta, find new tile/pixel
   coordinate. Return 0 if successful, -1 if location is outside the
   tileset and the boundary coordinate was returned.
 */
int ts_PixelAdd(const struct tsHdr *tsh,                  /* tileset header */
                 const struct tsRect *tsrp,       /* rectangular projection */
                 const struct tsTilePixel *tp,          /* given tile/pixel */
                 int npx, int npy,        /* positive/negative pixel deltas */
                 struct tsTilePixel *tpn) {          /* returned tile/pixel */
   int iret;
   int *tsrow;                                                 /* row table */
   div_t dv;                              /* integer quotient and remainder */
/* ------------------------------------------------------------------------ */
   iret = 0;
   tsrow = (int *)(tsrp + 1);
   if ((npx == 0) && (npy == 0)) *tpn = *tp;
   else {
      dv = div(tp->tileColumn * tsh->tileWidth + tp->pxl_x + npx, tsh->tileWidth);
      tpn->tileColumn = dv.quot;
      tpn->pxl_x = dv.rem;
      dv = div(tp->tileRow * tsh->tileHeight + tp->pxl_y + npy, tsh->tileHeight);
      tpn->tileRow = dv.quot;
      tpn->pxl_y = dv.rem;
      }
   if (tpn->tileColumn < 0) {
      tpn->tileColumn = 0;
      tpn->pxl_x = 0;
      iret = -1;
      }
   if (tpn->tileColumn >= tsh->tileColumns) {
      tpn->tileColumn = tsh->tileWidth - 1;
      tpn->pxl_x = tsh->tileColumns - 1;
      iret = -1;
      }
   if (tpn->tileRow < 0) {
      tpn->tileRow = 0;
      tpn->pxl_y = 0;
      iret = -1;
      }
   if (tpn->tileRow >= tsh->tileRows) {
      tpn->tileRow = tsh->tileHeight - 1;
      tpn->pxl_y = tsh->tileRows - 1;
      iret = -1;
      }
   return(iret);
   }
/* ======================================================================== */
/* The following two functions provide the method to deal with the cyclic
   nature of longitudes, measured in radians, represented in scaled integers
   that map (nearly but not quite) the whole internal range of integer values
   to -PI <-> PI range of longitudes. (A mitigating assumption is that both
   the tile center and global longitude are always normalized to -PI <-> PI
   range, and that the "in-tileset" delta is always less than PI/2.
 */
/* Given tileset center and east/west delta, return global longitude. */
int ts_LongitudeGlobal(int lng0,                   /* tile center longitude */
                       int delta) {                                /* delta */
/* ------------------------------------------------------------------------ */
   int dd;
   if (delta == 0) return(lng0);
   if (lng0 > 0) {
      if (delta < 0) return(lng0 + delta);
      dd = TS_I4A_SCALE_INT - lng0;
      if (delta < dd) return(lng0 + delta);
      else return(-TS_I4A_SCALE_INT + (delta - dd));
      }
   else {
      if (delta > 0) return(lng0 + delta);
      dd = -(TS_I4A_SCALE_INT + lng0);
      if (delta > dd) return(lng0 + delta);
      else return(TS_I4A_SCALE_INT - (dd - delta));
      }
   }
/* ======================================================================== */
/* Given tileset center longitude based on some conventional meridian (most
   commonly Greenwich meridian) and global longitude measured (positive East,
   negative West) from the same meridian, return the tileset longitude:
   "delta", negative West and positive East from the tileset central
   meridian.
 */
int ts_LongitudeTileset(int lng0,                  /* tile center longitude */
                        int lng) {                      /* global longitude */
   unsigned iu;
/* ------------------------------------------------------------------------ */
   if (lng == lng0) return(0);
   if (lng0 > 0) {
      if (lng > 0) return(lng - lng0);
      iu = (unsigned)(TS_I4A_SCALE_INT - lng0) + (unsigned)(lng + TS_I4A_SCALE_INT);
      if (iu < TS_I4A_SCALE_INT) return((int)(iu));
      return(-(lng0 - lng));
      }
   else {
      if (lng < 0) return(lng - lng0);
      iu = (unsigned)(TS_I4A_SCALE_INT - lng) + (unsigned)(lng0 + TS_I4A_SCALE_INT);
      if (iu < TS_I4A_SCALE_INT) return(-(int)(iu));
      return(lng - lng0);
      }
   }
/* ======================================================================== */
/* This is the integer ratio evaluation ("m = (ma * mb) / n") isolated,
   so that the implementors on platforms that lack an 8-byte integer data
   type can choose to either:
      a) implement long multiplication and division from the whole cloth.
      b) resort to floating point atithmetic or
      c) suffer the indignity of unnecessary precision loss.
 */
int ts_IntRat2(int ma,              /* probably small (< 64K) multiplicand. */
               int mb,              /* probably large (> 64K) multiplicand. */
               int n) {                          /* probably large divisor. */
   long long m;
/* ------------------------------------------------------------------------ */
   m = (long long)ma * (long long)mb;
   m = m / n;
   return((int)(m));
   }
/* ======================================================================== */
/* as above for "m = (ma * mb * mc) / md" */
int ts_IntRat3(int ma,              /* probably small (< 64K) multiplicand. */
               int mb,              /* probably small (> 64K) multiplicand. */
               int mc,              /* probably large (> 64K) multiplicand. */
               int n) {                          /* probably large divisor. */
   long long m;
/* ------------------------------------------------------------------------ */
   m = (long long)ma * (long long)mb * (long long)mc;
   m = m / n;
   return((int)(m));
   }
/* ======================================================================== */
/* Return modified 'Morton Order' number of a rectangular matrix element.
   This function facilitates spatial clustering of tiles in the file.

   It is based on an unpublished schema developed by Guy M. Morton, then of
   IBM Canada, in support of the Canada Land Inventory project, circa 1966.

   The program that compiles the tileset might do something like this:
   (This is only a very simple example, note also the total absence of
   any clean-up and error checking!)

   FILE *tsFp;                               /# tileset stream file pointer #/
   int nCols, nRows;                     /# number of tile columns and rows #/
   int nx;                                  /# size of tile index worktable #/
   int ix;                                        /# tile index file offset #/
   struct tsTile1 *workTlx;                        /# tile index work table #/
   unsigned char *tile;           /# where compressed tile will be composed #/
   int i, j, n, m;
   ...
   determine total number of tile columns and rows
   ...
/# allocate space for work tile index table #/
   nx = ts_Morton(nCols - 1, nRows - 1, nCols, nRows) + 1;
   workTlx = (struct tsTile1 *)malloc(nx * sizeof(struct tsTile1));
/# Initialize the table #/
   memset(workTix, 0xff, nx * sizeof(struct tsTile1));
   for (j = 0; j < nRows; j++) {                           /# traverse rows #/
      for (i = 0; i < nCols; i++) {                     /# traverse columns #/
         n = ts_Morton(i, j, nCols, nRows);          /# get worktable index #/
         workTlx[n].dataStart = i;
         workTlx[n].dataLength = j;
         }
      }
   ...
/# Open the file #/
   tsFp = fopen("outerCascadia", "wb");
   ...
   write the file content that precedes the tile index
   ...
   ix = ftell(tsFp); /# Record tile index file offset and write dummy table #/
   fwrite(workTix, sizeof(struct tsTile1), nCols * nRows, tsFp);
   ...
   write the file content between tile index and tiles
   ...
/# Write tiles #/
   for (n = 0; n < nx; n++) {                  /# traverse work index table #/
      i = workTlx[n].dataStart;
      j = workTlx[n].dataLength;
      if ((i == 0xffffffff) || (j == 0xffffffff)) continue; /# no such tile #/
      ...
      compose/compress to size ~m~ the tile in column ~i~, row ~j~ in ~tile~
      ...
      workTlx[n].dataStart = ftell(tsFp);
      workTlx[n].dataLength = m;
      fwrite(tile, m, 1, tsFp);
      }
   fclose(tsFp);
   ...
/# Re-write tile index table with size/offset data in prescribed i, j order #/
   outFp = fopen(tsFp, "rb+");
   fseek(tsFp, ix, SEEK_SET);
   for (j = 0; j < nRows; j++) {                           /# traverse rows #/
      for (i = 0; i < nCols; i++) {                     /# traverse columns #/
         n = ts_Morton(i, j, nCols, nRows);          /# get worktable index #/
         fwrite(workTlx + m, sizeof(struct tsTile1), 1, tsFp);
         }
      }
   fclose(tsFp);
   ...
 */
int ts_Morton(int i, int j,                            /* column, row index */
              int m, int n) {                       /* matrix width, height */
   int ix = 0;
   unsigned mm = m;
   unsigned nn = n;
   unsigned m1 = 1;
   unsigned n1 = 1;
/* ------------------------------------------------------------------------ */
   while (mm >>= 1) m1 <<= 1;
   while (nn >>= 1) n1 <<= 1;
   while(m1 || n1) {
      if (m1) {
         ix <<= 1;
         if (m1 & i) ix |= 1;
         m1 >>= 1;
         }
      if (n1) {
         ix <<= 1;
         if (n1 & j) ix |= 1;
         n1 >>= 1;
         }
      }
   return(ix);
   }
/* ======================================================================== */
