/* This C language source code was developed by Geodyssey Limited of
   Calgary, Alberta, Canada. It is provided to application developers
   free of charge and free of copyright restriction with the hope that it
   will provide C language programming examples to be used in development
   of platform-independent geographical applications utilizing a common
   scanned map file format, as described in tileset.h. The content of this
   file may be freely used and/or modified. Attribution of this code to
   Geodyssey Limited at www.geodyssey.com would be appreciated.
 */
#include "viewtgxm.h"
/* ======================================================================== */
/* perform a very common pixel calculation, determining the limits of a
   window client rectangle on the tileset. Return 0 if limits of tileset
   are not excedeed; 1 otherwise.
 */
int windowLimits(const struct tsHdr *tsh,                   /* tileset size */
                 const struct tsRect *tsrp,      /* tileset latitude limits */
                 const RECT *cRect,                     /* client rectangle */
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
/* Window size has changed. If the size increase placed part of the window
   beyond the tileset, adjust the window center on tileset.
   Return -1 if change forced recentering, 0 otherwise.
 */
int resizeWindow(struct tgxmViewControl *tvc,                /* system data */
                 RECT *cr) {                             /* new window size */
   int i, m;
   int iret;
   int ileft, iright;
   int ibottom, itop;
/* ------------------------------------------------------------------------ */
/* debugTrace("sizing window");*/
/* debugTrace("old size   :%3d %3d", tvc->mpw.cRect.right, tvc->mpw.cRect.bottom);*/
/* debugTrace("old center :%3d %3d", tvc->mpw.crcTile.pxl_x, tvc->mpw.crcTile.pxl_y);*/
/* debugTrace("new size   :%3d %3d", cr->right, cr->bottom);*/
   i = windowLimits(tvc->tsh, tvc->tsrp, cr, &tvc->mpw.crcTile, &ileft, &iright, &ibottom, &itop);
   if (i) {
      m = TS_PX_WIDTH(tvc->tsh) - 1;
      if (ileft < 0) tvc->mpw.crcTile.pxl_x -= ileft;
      if (iright > m) tvc->mpw.crcTile.pxl_x -= (iright - m);
      if (cr->right > m) tvc->mpw.crcTile.pxl_x = (m + 1) / 2;

      if (ibottom < tvc->mpw.lowLatY) tvc->mpw.crcTile.pxl_y = tvc->mpw.lowLatY + (tvc->mpw.cRect.bottom - tvc->mpw.cRect.top) / 2;
      if (itop > tvc->mpw.highLatY) tvc->mpw.crcTile.pxl_y = tvc->mpw.highLatY - (tvc->mpw.cRect.bottom - tvc->mpw.cRect.top) / 2;
      m = tvc->mpw.highLatY - tvc->mpw.lowLatY;
      if (cr->bottom > m) tvc->mpw.crcTile.pxl_y = (m + 1) / 2;

      iret = -1;
      }
   else iret = 0;

   tvc->mpw.cRect = *cr;
   return(iret);
   }
/* ======================================================================== */
/* Move the window by determining new window center in tileset coordinates.
   The measure specified (~move~) is that of the number of pixels to move the
   window center over the tileset, East or North if positive, West or South
   if negative. or the position requested by moving the slider. Full or partial
   amount of the move requested by the invoker may be possible. Return amount
   moved if successful, 0 in case no move was possible, i.e., any amount of
   move in the required direction would take the winwow edge beyond the edge
   of the tileset.
 */
int moveWindow(struct tgxmViewControl *tvc,
               int move,        /* in signed pixels, or new slider position */
               int idir) {               /* direction or position indicator */
   int i, m;
   int ileft, iright;
   int ibottom, itop;
   struct tsPixel nc;                                         /* new center */
/* ------------------------------------------------------------------------ */
/* debugTrace("requested move: %d", move);*/
   if (move == 0) return(0);                                /* just in case */
   nc = tvc->mpw.crcTile;                            /* proposed new center */
   if (idir == MOVE_EAST_WEST) nc.pxl_x += move;
   else if (idir == MOVE_NORTH_SOUTH) nc.pxl_y += move;

   i = windowLimits(tvc->tsh, tvc->tsrp, &tvc->mpw.cRect, &nc, &ileft, &iright, &ibottom, &itop);
   if (i) {                       /* proposed move is not possible. Is any? */
      if (idir == MOVE_EAST_WEST) {
         m = TS_PX_WIDTH(tvc->tsh) - 1;
         if (ileft < 0) {
            move -= ileft;
            if (move >= 0) return(0);
            }
         if (iright > m) {
            move -= (iright - m);
            if (move <= 0) return(0);
            }
         }
      if (idir == MOVE_NORTH_SOUTH) {
         if (ibottom < tvc->mpw.lowLatY) {
/*          debugTrace("check south limit:%d %d", ibottom, tvc->mpw.lowLatY);*/
            move = move - ibottom + tvc->mpw.lowLatY;
            if (move >= 0) return(0);
            }
         if (itop > tvc->mpw.highLatY) {
/*          debugTrace("check north limit:%d %d", itop, tvc->mpw.highLatY);*/
            move = move - itop + tvc->mpw.highLatY;
            if (move <= 0) return(0);
            }
         }
      }

   if (idir == MOVE_EAST_WEST) tvc->mpw.crcTile.pxl_x += move;
   else if (idir == MOVE_NORTH_SOUTH) tvc->mpw.crcTile.pxl_y += move;
   return(move);                         /* inform invoker to force repaint */
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
int positionWindow(struct tgxmViewControl *tvc,
                   struct tsLatLng *tll) {     /* global latitude/longitude */
   int i, m;
   int iret;
   struct tsTilePixel tstp;
   struct tsPixel nc;                                         /* new center */
   int ileft, iright;
   int ibottom, itop;
/* ------------------------------------------------------------------------ */
   i = ts_MapRect(tvc->tsh, tvc->tsrp, tll, &tstp);
   if (i) return(1);
   nc.pxl_x = tstp.tileColumn * tvc->tsh->tileWidth + tstp.pxl_x;
   nc.pxl_y = tstp.tileRow * tvc->tsh->tileHeight + tstp.pxl_y;
   iret = 0;
   i = windowLimits(tvc->tsh, tvc->tsrp, &tvc->mpw.cRect, &nc, &ileft, &iright, &ibottom, &itop);
   if (i) {                       /* proposed move is not possible. Is any? */
      if (ileft < 0) nc.pxl_x = tvc->mpw.cRect.right / 2;
      m = TS_PX_WIDTH(tvc->tsh) - 1;
      if (iright > m) nc.pxl_x = m - tvc->mpw.cRect.right / 2;
      if (ibottom < tvc->mpw.lowLatY) nc.pxl_y = tvc->mpw.lowLatY + tvc->mpw.cRect.bottom / 2;
      if (itop > tvc->mpw.highLatY) nc.pxl_y = tvc->mpw.highLatY - tvc->mpw.cRect.bottom / 2;
      iret = -1;
      }

   if ((nc.pxl_x == tvc->mpw.crcTile.pxl_x)
    && (nc.pxl_y == tvc->mpw.crcTile.pxl_y)) return(-2);       /* null move */
   tvc->mpw.crcTile = nc;

   return(iret);
   }
/* ======================================================================== */
/* convert from client window rectangle coordinates to tileset pixel. Note
   that client rectangle in Win32 has 0, 0 in upper-left corner, tileset
   pixel 0, 0 is in the lower-left corner.
 */
void win2ts(struct mapWindow *mpw,
            int iwx, int iwy,                        /* given window coords */
            int *itx, int *ity) {                /* returned tileset coords */

   int i0, j0;
/* ------------------------------------------------------------------------ */
/* window rectangle upper left corner in tileset pixel coordinates. */
   i0 = mpw->crcTile.pxl_x - (mpw->cRect.right - mpw->cRect.left) / 2;
   j0 = mpw->crcTile.pxl_y + (mpw->cRect.bottom - mpw->cRect.top) / 2;
   *itx = i0 + iwx;
   *ity = j0 - iwy;
   return;
   }
/* ======================================================================== */
/* Convert from tileset pixel to window rectangle pixel. No range checking
   is performed.
 */
void ts2win(struct mapWindow *mpw,                            /* map window */
            int itx, int ity,                        /* given tileset pixel */
            int *iwx, int *iwy) {                   /* return window coords */

   int i0, j0;                 /* window coordinate origin as tileset pixel */
/* ------------------------------------------------------------------------ */
   i0 = mpw->crcTile.pxl_x - (mpw->cRect.right - mpw->cRect.left) / 2;
   j0 = mpw->crcTile.pxl_y + (mpw->cRect.bottom - mpw->cRect.top) / 2;
   *iwx = itx - i0;
   *iwy = j0 - ity;
   return;
   }
/* ======================================================================== */
/* set values of minimum and maximum tile columns and rows that are covered
   by the curent window, return 0 if OK, 1 if (unexpected!) error.
 */
int windowTiles(struct tgxmViewControl *tvc,
                int *iWest, int *iEast,
                int *jSouth, int *jNorth) {

   int nWidth, nHeight;
   int nWidthLeftHalf, nHeightLowHalf;
   struct tsPixel tspx;
   struct tsTilePixel tstp;
/* ------------------------------------------------------------------------ */
   nWidth = tvc->mpw.cRect.right - tvc->mpw.cRect.left;     /* see in above */
   nHeight = tvc->mpw.cRect.bottom - tvc->mpw.cRect.top;
/* debugTrace("window: %d x %d", nWidth, nHeight);*/
   nWidthLeftHalf = nWidth / 2;
   nHeightLowHalf = nHeight / 2;

/* debugTrace("center on tile: %d, %d", tvc->mpw.crcTile.pxl_x, tvc->mpw.crcTile.pxl_y);*/

/* mark lower left window corner in tileset coordinates... */
   tspx.pxl_x = tvc->mpw.crcTile.pxl_x - nWidthLeftHalf;
   tspx.pxl_y = tvc->mpw.crcTile.pxl_y - nHeightLowHalf;
/* debugTrace("ll corner: %d, %d", tspx.pxl_x, tspx.pxl_y);*/

/* ...and find the corresponding tileset point */
   if (ts_PixelToTilePixel(tvc->tsh, &tspx, &tstp)) {
      *iWest = 0;
      *jSouth = 0;
      }
   else {
      *iWest = tstp.tileColumn;
      *jSouth = tstp.tileRow;
      }
/* mark upper right window corner in tileset coordinates... */
   tspx.pxl_x = tvc->mpw.crcTile.pxl_x + (nWidth - nWidthLeftHalf) - 1;
   tspx.pxl_y = tvc->mpw.crcTile.pxl_y + (nHeight - nHeightLowHalf) - 1;
/* ...and find the corresponding tileset point */
   if (ts_PixelToTilePixel(tvc->tsh, &tspx, &tstp)) {
      *iEast = tvc->tsh->tileColumns - 1;
      *jNorth = tvc->tsh->tileRows - 1;
      }
   else {
      *iEast = tstp.tileColumn;
      *jNorth = tstp.tileRow;
      }
/* debugTrace("Tiles in window: S:%d <-> N:%d, W:%d <-> E:%d", *jSouth, *jNorth, *iWest, *iEast);*/
   return(0);
   }
/* ======================================================================== */
void reportFile(HWND hWnd, struct tgxmViewControl *tvc) {

   char msg[4096];
   char dscr1[34];
   char dscr2[34];
   int ln, ls, lgw, lge;
   int ileft, iright;
   int itop, ibottom;
/* ------------------------------------------------------------------------ */
   debugTrace("Tileset: %d x %d", TS_PX_WIDTH(tvc->tsh), TS_PX_HEIGHT(tvc->tsh));
   debugTrace("Lat Y, low: %d, high: %d", tvc->tsrp->lowLatY, tvc->tsrp->highLatY);

   debugTrace("Window:  %d x %d", tvc->mpw.cRect.right, tvc->mpw.cRect.bottom);

   windowLimits(tvc->tsh, tvc->tsrp, &tvc->mpw.cRect, &tvc->mpw.crcTile, &ileft, &iright, &itop, &ibottom);
   debugTrace("Window in ts coords: left:%d right:%d bottom:%d top:%d", ileft, iright, ibottom, itop);

   debugTrace("Scroll positions: horiz %d, vert %d", GetScrollPos(hWnd, SB_HORZ),  GetScrollPos(hWnd, SB_VERT));

   memcpy(dscr1, tvc->dscr_l1, 32);
   memcpy(dscr2, tvc->dscr_l2, 32);
   dscr1[32] = dscr2[32] = '\0';

   ln = (tvc->tsrp->centerLat + tvc->tsrp->rangeLat / 2);
   ls = (tvc->tsrp->centerLat - tvc->tsrp->rangeLat / 2);
   lgw = ts_LongitudeGlobal(tvc->tsrp->centerLng, -tvc->tsrp->rangeLng / 2);
   lge = ts_LongitudeGlobal(tvc->tsrp->centerLng,  tvc->tsrp->rangeLng / 2);

   sprintf(msg, "Tileset file:\n%s\n%s\n%s\ntile columns, rows: %d x %d\ntile width, height: %d x %d\n\nTileset limits:\n               North: %c%d:%06.3f\nWest: %c%d:%06.3f   East: %c%d:%06.3f\n               South: %c%d:%06.3f\n",
    tvc->fn, dscr1, dscr2,
    tvc->tsh->tileColumns, tvc->tsh->tileRows,
    tvc->tsh->tileWidth, tvc->tsh->tileHeight,
   TS_SIGN_LAT(ln),  TS_INTDEG_LL(ln),  TS_DECMIN_LL(ln),
   TS_SIGN_LNG(lgw), TS_INTDEG_LL(lgw), TS_DECMIN_LL(lgw),
   TS_SIGN_LNG(lge), TS_INTDEG_LL(lge), TS_DECMIN_LL(lge),
   TS_SIGN_LAT(ls),  TS_INTDEG_LL(ls),  TS_DECMIN_LL(ls));

   MessageBox(hWnd, msg, APP_NAME, MB_OK);
   return;
   }
/* ======================================================================== */
void userError(char *fmt, ...) {
   static char msg[256];
   va_list      p_arg;
   va_start(p_arg, fmt);
/* ------------------------------------------------------------------------ */
   vsprintf(msg, fmt, p_arg);
   MessageBeep(MB_ICONHAND);
   MessageBox(GetFocus(), msg, APP_NAME, MB_ICONEXCLAMATION | MB_OK);
   va_end(p_arg);
   return;
   }
/* ======================================================================== */
void userInfo(char *fmt, ...) {
   static char msg[256];
   va_list      p_arg;
   va_start(p_arg, fmt);
/* ------------------------------------------------------------------------ */
   vsprintf(msg, fmt, p_arg);
   MessageBeep(MB_ICONHAND);
   MessageBox(GetFocus(), msg, APP_NAME, MB_ICONINFORMATION | MB_OK);
   va_end(p_arg);
   return;
   }
/* ======================================================================== */
/* Debugging tracing can be active or inactive. If the first call to this
   function is with a NULL argument, it will go to sleep; otherwise it
   will assume it is to be activated, and that the argument is the pointer
   to an open tracing file - saved and used in subsequent trace requests.
   It is assumed tracing calls in tight loops are commented out; thus
   this function can be left included "as is" in "production builds".
 */
void debugTrace(char *fmt, ...) {
   static int initialized;
   static FILE *traceFp;
   static char  msg[LINE_LENGTH + 1];
   va_list      p_arg;
   int          i;
/* ------------------------------------------------------------------------ */
   if (initialized == 0) {                                    /* first call */
      initialized = 1;
      traceFp = (FILE *)fmt;                /* if NULL, we just go to sleep */
      return;
      }
   if (traceFp == NULL) return;                      /* no trace file open? */
   va_start(p_arg, fmt);
   if (fmt == NULL) traceFp = va_arg(p_arg, FILE *);          /* initialize */
   else {
      vsprintf(msg, fmt, p_arg);
      i = strlen(msg);
      if (i && traceFp) {
         fputs(msg, traceFp);
         if (msg[i - 1] != '\n') fputc('\n', traceFp);
         fflush(traceFp);
         }
      }
   va_end(p_arg);
   return;
   }
/* ======================================================================== */
