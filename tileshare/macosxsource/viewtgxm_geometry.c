/* viewtgxm_geometry.c */

#include "viewtgxm.h"

/* ======================================================================== */
/* perform a very common pixel calculation, determining the limits of a
   window client rectangle on the tileset. Return 0 if limits of tileset
   are not excedeed; 1 otherwise.
 */
int windowLimits(const struct tsHdr *tsh,                   /* tileset size */
                 const struct tsRect *tsrp,      /* tileset latitude limits */
                 const struct Rect *cRect,              /* client rectangle */
                 const struct tsPixel *cntr,     /* center of it on tileset */
                 int *ileft, int *iright,      /* limit rows of cr (window) */
                 int *ibottom, int *itop) {       /* in tileset coordinates */
   int n;
/* ------------------------------------------------------------------------ */
   n = cRect->right - cRect->left;
   *ileft = cntr->pxl_x - n / 2;
   *iright = *ileft + n - 1;

   n = cRect->bottom - cRect->top;
   *itop = cntr->pxl_y + n / 2;
   *ibottom = *itop - n + 1;
/* Note that latitude range must be restricted: tileset extends above/below */
   if ((*ileft < 0) || (*iright > (tsh->tileColumns * tsh->tileWidth - 1)) ||
       (*ibottom < tsrp->lowLatY) || (*itop > tsrp->highLatY)) return(1);
   return(0);
   }
/* ======================================================================== */
int resizeWindow(struct mapControl *mc,                      /* system data */ 
                 struct Rect *newRect) {                 /* new window size */
   int i, m;
   int iret;
   int ileft, iright;
   int ibottom, itop;
/* ------------------------------------------------------------------------ */
   DBG_TRACE((stderr, "resizing window to:%3d %3d %3d %3d\n", newRect->left, newRect->right, newRect->bottom, newRect->top));
   i = windowLimits(mc->tsh, mc->tsrp, newRect, &mc->crcTile, &ileft, &iright, &ibottom, &itop);
   DBG_TRACE((stderr, "limits %d: %3d %3d %3d %3d\n", i, ileft, iright, ibottom, itop));
   if (i) {
      m = TS_PX_WIDTH(mc->tsh) - 1;
      if (ileft < 0) mc->crcTile.pxl_x -= ileft;
      if (iright > m) mc->crcTile.pxl_x -= (iright - m);
      if (newRect->right > m) mc->crcTile.pxl_x = (m + 1) / 2;

      if (ibottom < mc->tsrp->lowLatY) mc->crcTile.pxl_y = mc->tsrp->lowLatY + (mc->winRect.bottom - mc->winRect.top) / 2;
      if (itop > mc->tsrp->highLatY) mc->crcTile.pxl_y = mc->tsrp->highLatY - (mc->winRect.bottom - mc->winRect.top) / 2;
      m = mc->tsrp->highLatY - mc->tsrp->lowLatY;
      if (newRect->bottom > m) mc->crcTile.pxl_y = (m + 1) / 2;

      iret = -1;
      }
   else iret = 0;
   mc->winRect = *newRect; 
   return(iret);
   }
/* ======================================================================== */
/* Position window to a point given by latitude/longitude. The longitude is
   relative to the tileset mid-longitude, not the conventional meridian.
   New point will be at the window centre, unless this would move the edge
   of the window past the edge of the tileset; in which case the maximum
   possible move is effected.  Returns 0 if window was moved, as requested,
   -2 if null move was requested. -1 if moved but not the amount requested,
   1 if move to a location outside the tileset coverage was requested.
 */
int positionWindow(struct mapControl *mc,
                   struct tsLatLng *tll) {     /* global latitude/longitude */
   int i, m;
   int iret;
   struct tsTilePixel tstp;
   struct tsPixel nc;                                         /* new center */
   int ileft, iright;
   int ibottom, itop;
/* ------------------------------------------------------------------------ */
   i = ts_MapRect(mc->tsh, mc->tsrp, tll, &tstp);
   if (i) return(1);
   nc.pxl_x = tstp.tileColumn * mc->tsh->tileWidth + tstp.pxl_x;
   nc.pxl_y = tstp.tileRow * mc->tsh->tileHeight + tstp.pxl_y;
   iret = 0;
   i = windowLimits(mc->tsh, mc->tsrp, &mc->winRect, &nc, &ileft, &iright, &ibottom, &itop);
   if (i) {                       /* proposed move is not possible. Is any? */
      if (ileft < 0) nc.pxl_x = mc->winRect.right / 2;
      m = TS_PX_WIDTH(mc->tsh) - 1;
      if (iright > m) nc.pxl_x = m - mc->winRect.right / 2;
      if (ibottom < mc->tsrp->lowLatY) nc.pxl_y = mc->tsrp->lowLatY + mc->winRect.bottom / 2;
      if (itop > mc->tsrp->highLatY) nc.pxl_y = mc->tsrp->highLatY - mc->winRect.bottom / 2;
      iret = -1;
      }

   if ((nc.pxl_x == mc->crcTile.pxl_x)
    && (nc.pxl_y == mc->crcTile.pxl_y)) return(-2);            /* null move */
   mc->crcTile = nc;

   return(iret);
   }
/* ======================================================================== */
/* set values of minimum and maximum tile columns and rows that are covered
   by the curent window, return 0 if OK, 1 if (unexpected!) error.
 */
int windowTiles(struct mapControl *mc,
                int *iWest, int *iEast,
                int *jSouth, int *jNorth) {

   int nWidth, nHeight;
   int nWidthLeftHalf, nHeightLowHalf;
   struct tsPixel tspx;
   struct tsTilePixel tstp;
/* ------------------------------------------------------------------------ */
   nWidth = mc->winRect.right - mc->winRect.left;           /* see in above */
   nHeight = mc->winRect.bottom - mc->winRect.top;

   DBG_TRACE((stderr, "Window tiles, size: %d x %d.\n", nWidth, nHeight));

   nWidthLeftHalf = nWidth / 2;
   nHeightLowHalf = nHeight / 2;

   DBG_TRACE((stderr, "center on tile: %d, %d.\n", mc->crcTile.pxl_x, mc->crcTile.pxl_y)); 

/* mark lower left window corner in tileset coordinates... */
   tspx.pxl_x = mc->crcTile.pxl_x - nWidthLeftHalf;
   tspx.pxl_y = mc->crcTile.pxl_y - nHeightLowHalf;
/* debugTrace("ll corner: %d, %d", tspx.pxl_x, tspx.pxl_y); */

/* ...and find the corresponding tileset point */
   if (ts_PixelToTilePixel(mc->tsh, &tspx, &tstp)) {
      *iWest = 0;
      *jSouth = 0;
      }
   else {
      *iWest = tstp.tileColumn;
      *jSouth = tstp.tileRow;
      }
/* mark upper right window corner in tileset coordinates... */
   tspx.pxl_x = mc->crcTile.pxl_x + (nWidth - nWidthLeftHalf) - 1;
   tspx.pxl_y = mc->crcTile.pxl_y + (nHeight - nHeightLowHalf) - 1;
/* ...and find the corresponding tileset point */
   if (ts_PixelToTilePixel(mc->tsh, &tspx, &tstp)) {
      *iEast = mc->tsh->tileColumns - 1;
      *jNorth = mc->tsh->tileRows - 1;
      }
   else {
      *iEast = tstp.tileColumn;
      *jNorth = tstp.tileRow;
      }
   DBG_TRACE((stderr, "Tiles in window: S:%d <-> N:%d, W:%d <-> E:%d.\n", *jSouth, *jNorth, *iWest, *iEast)); 
   return(0);
   }
/* ======================================================================== */
/* convert from client window rectangle coordinates to tileset pixel. Note
   that client rectangle in Win32/Mac OS X has 0, 0 in upper-left corner, 
   tileset pixel 0, 0 is in the lower-left corner.
 */
void win2ts(struct Rect *winRect,                   /* map window rectangle */
            struct tsPixel *crcTile,     /* window centre in tileset coords */
            int iwx, int iwy,                        /* given window coords */
            int *itx, int *ity) {                /* returned tileset coords */

   int i0, j0;
/* ------------------------------------------------------------------------ */
/* window rectangle upper left corner in tileset pixel coordinates. */
   i0 = crcTile->pxl_x - (winRect->right - winRect->left) / 2;
   j0 = crcTile->pxl_y + (winRect->bottom - winRect->top) / 2;
   *itx = i0 + iwx;
   *ity = j0 - iwy;
   return;
   }
/* ======================================================================== */
/* Convert from tileset pixel to window rectangle pixel. No range checking
   is performed.
 */
void ts2win(struct Rect *winRect,                   /* map window rectangle */
            struct tsPixel *crcTile,     /* window centre in tileset coords */
            int itx, int ity,                        /* given tileset pixel */
            int *iwx, int *iwy) {                   /* return window coords */

   int i0, j0;                 /* window coordinate origin as tileset pixel */
/* ------------------------------------------------------------------------ */
   i0 = crcTile->pxl_x - (winRect->right - winRect->left) / 2;
   j0 = crcTile->pxl_y + (winRect->bottom - winRect->top) / 2;
   *iwx = itx - i0;
   *iwy = j0 - ity;
   return;
   }
/* ======================================================================== */
