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
void paintWindowTiles(HDC hdc,
                      struct tgxmViewControl *tvc) {
   int istat;
   int i, j;
   int iWest, iEast, jSouth, jNorth;
   int iXDest, iYDest;
   int ixt;                                      /* tile pixel buffer index */
/* ------------------------------------------------------------------------ */
/* debugTrace("***Painting tiles");*/
   windowTiles(tvc, &iWest, &iEast, &jSouth, &jNorth);
/* debugTrace("Edge tiles: columns:%d to %d, rows: %d to %d", iWest, iEast, jSouth, jNorth);*/
   for (j = jSouth; j <= jNorth; j++) {
      for (i = iWest; i <= iEast; i++) {       /* tile i, j must be painted */
/*       debugTrace("Tile column: %d  row: %d", i, j);*/
         for (ixt = 0; ixt < TILES_IN_MEM; ixt++) {     /* is it in memory? */
            if (tvc->tile[ixt].loadCounter == 0) continue;   /* virgin slot */
            if ((tvc->tile[ixt].column == i) && (tvc->tile[ixt].row == j)) break;  /* found it */
            }
         if (ixt == TILES_IN_MEM) ixt = loadTile(tvc, i, j);  /* get it from file */
         if (ixt == -1) {                 /* unexpectedly, failed to do so? */
            userError("Unexpected error: unable to access tile %d %d", i, j);
            return;
            }
/*       debugTrace("from memory buffer slot: %d", ixt);*/
         ts2win(&tvc->mpw, i * tvc->tsh->tileWidth, j * tvc->tsh->tileHeight + tvc->tsh->tileHeight, &iXDest, &iYDest);
/*       in Win32 blit reference point is upper-left corner */
/*       debugTrace("Screen position: %d %d", iXDest, iYDest);*/
         istat = SetDIBitsToDevice(hdc, iXDest, iYDest, tvc->tsh->tileWidth, tvc->tsh->tileHeight, 0, 0, 0, tvc->bm.bmiHeader.biHeight, tvc->fm.pixelBuffer + ixt * tvc->tileSize, &tvc->bm, DIB_RGB_COLORS);
         if (istat == 0) {
            userError("Unexpected error: unable to paint tile %d %d", i, j);
            return;
            }
         }
      }
   return;
   }
/* ======================================================================== */
void drawTileBoundaries(HDC hdc,
                        struct tgxmViewControl *tvc) {
   int i, j;
   int iWest, iEast, jSouth, jNorth;
   int iXDest, iYDest;
   HGDIOBJ hObj;
/* ------------------------------------------------------------------------ */
   debugTrace("***Drawing tile boundaries");
   windowTiles(tvc, &iWest, &iEast, &jSouth, &jNorth);
   hObj = SelectObject(hdc, tvc->tileBoundaryPen);
   for (i = iWest + 1; i <= iEast; i++) {
      ts2win(&tvc->mpw, i * tvc->tsh->tileWidth, 0, &iXDest, &iYDest);
      MoveToEx(hdc, iXDest, tvc->mpw.cRect.bottom, NULL);
      LineTo(hdc, iXDest, tvc->mpw.cRect.top);
      }
   for (j = jSouth + 1; j <= jNorth; j++) {
      ts2win(&tvc->mpw, 0, j * tvc->tsh->tileHeight, &iXDest, &iYDest);
      MoveToEx(hdc, tvc->mpw.cRect.left, iYDest, NULL);
      LineTo(hdc, tvc->mpw.cRect.right, iYDest);
      }
   SelectObject(hdc, hObj);
   return;
   }
/* ======================================================================== */
void drawLatLongGrid(HDC hdc,
                     struct tgxmViewControl *tvc) {
   int istat;
   struct tsPixel tspx;
   struct tsTilePixel tstp;
   int iwx, iwy;
   struct tsLatLng gpsw;                          /* lower left corner */
   struct tsLatLng gpne;                         /* upper right corner */
   struct tsLatLng gp;                               /* traverse point */
   double range, d, x;
   int istep, i, l, n;
   HGDIOBJ hObj;
/* ------------------------------------------------------------------------ */
   iwx = (tvc->mpw.cRect.right - tvc->mpw.cRect.left) - 1;
   iwy = (tvc->mpw.cRect.bottom - tvc->mpw.cRect.top) - 1;

   win2ts(&tvc->mpw, 0, iwy, &tspx.pxl_x, &tspx.pxl_y);
   istat = ts_PixelToTilePixel(tvc->tsh, &tspx, &tstp);
   if (istat) {
      debugTrace("can't draw grid, whole tileset inside window?");
      return;
      }
   istat = ts_UnMapRect(tvc->tsh, tvc->tsrp, &tstp, &gpsw);    /* southwest */
   if (istat) return;

   win2ts(&tvc->mpw, iwx, 0, &tspx.pxl_x, &tspx.pxl_y);
   istat = ts_PixelToTilePixel(tvc->tsh, &tspx, &tstp);
   if (istat) {
      debugTrace("can't draw grid, whole tileset inside window?");
      return;
      }
   istat = ts_UnMapRect(tvc->tsh, tvc->tsrp, &tstp, &gpne);    /* northeast */
   if (istat) return;

/*   debugTrace("grid limits:");*/
/*   debugTrace("south:%10.6f, north:%10.6f", TS_DECDEG_LAT(gpsw.lat), TS_DECDEG_LAT(gpne.lat));*/
/*   debugTrace("west:%10.6f, east:%10.6f", TS_DECDEG_LNG2(tvc->tsrp->centerLng, gpsw.lng), TS_DECDEG_LNG2(tvc->tsrp->centerLng, gpne.lng));*/

   hObj = SelectObject(hdc, tvc->latLngPen);

/* Find window range in degrees of latitude, floating point: */
   range = TS_RAD2DEG * ((TS_AngOfI4(gpne.lat) - TS_AngOfI4(gpsw.lat)));
/*   debugTrace("range degrees: %10.6f", range);*/

/* Determine the grid step in integer seconds */
   istep = gridDensity(range);
/*   debugTrace("latitude range deg: %10.6f, step secs: %d", range, istep);*/

/* Determine a starting latitude at even step, just below the south edge:   */
   d = 3600.0 * TS_RAD2DEG * (TS_AngOfI4(gpsw.lat));  /* south lat, seconds */
   n = (int)(d / (double)(istep));                 /* south latitude, steps */
/*   debugTrace("stepcount at latitude start: %d", n);*/

/* first latitude on even step, in degrees */
   x = ((double)(n) * (double)(istep) / 3600.0);
/*   debugTrace("start latitude degrees: %10.6f", x);*/

   d = (double)istep / 3600.0;                              /* step degrees */
/*   debugTrace("step degrees: %10.6f", d);*/

   n = (int)(range / d) + 2;                                       /* steps */
/*   debugTrace("steps in range: %d", n);*/

   gp.lng = 0;
   for (i = 0; i < n; i++) {
/*    debugTrace("%2d lat: %10.6f", i, x); */
      gp.lat = TS_I4OfAng(TS_DEG2RAD * x);
      istat = ts_MapRect(tvc->tsh, tvc->tsrp, &gp, &tstp);
      l = tstp.tileRow * tvc->tsh->tileHeight + tstp.pxl_y;   /* tile pixel */
      ts2win(&tvc->mpw, 0, l, &iwx, &iwy);
      MoveToEx(hdc, tvc->mpw.cRect.left, iwy, NULL);
      LineTo(hdc, tvc->mpw.cRect.right, iwy);
      x += d;
      }

/* Very similar for longitudes, but based on conventional central meridian */
   range = TS_RAD2DEG * ((TS_AngOfI4(gpne.lng) - TS_AngOfI4(gpsw.lng)));
   istep = gridDensity(range);
/*   debugTrace("longitude range deg: %10.6f, step secs: %d", range, istep);*/

   l = ts_LongitudeGlobal(tvc->tsrp->centerLng, gpsw.lng);
/*   debugTrace("int global longitude west: %10.6f", TS_DECDEG_LNG1(l));*/

   d = 3600.0 * TS_RAD2DEG * (TS_AngOfI4(l));
   n = (int)(d / (double)(istep));
/*   debugTrace("stepcount at longitude start: %d", i);*/

   x = ((double)(n) * (double)(istep) / 3600.0);
/*   debugTrace("start latitude degrees: %10.6f", x);*/

   d = (double)istep / 3600.0;                              /* step degrees */
/*   debugTrace("step degrees: %10.6f", d);*/

   n = (int)(range / d) + 2;                                       /* steps */
/*   debugTrace("steps in range: %d", n);*/

   gp.lat = tvc->tsrp->centerLat;
   for (i = 0; i < n; i++) {
/*    debugTrace("%2d lng, global: %10.6f", i, x);*/
      l = TS_I4OfAng(TS_DEG2RAD * x);
      gp.lng = ts_LongitudeTileset(tvc->tsrp->centerLng, l);
/*    debugTrace("   ...tileset: %10.6f", TS_DECDEG_LNG1(gp.lng));*/
      istat = ts_MapRect(tvc->tsh, tvc->tsrp, &gp, &tstp);
/*    debugTrace("status: %d", istat);*/
      l = tstp.tileColumn * tvc->tsh->tileWidth + tstp.pxl_x; /* tile pixel */
      ts2win(&tvc->mpw, l, 0, &iwx, &iwy);
/*    debugTrace("window coords: %d %d", iwx, iwy);*/
      MoveToEx(hdc, iwx, tvc->mpw.cRect.bottom, NULL);
      LineTo(hdc, iwx, tvc->mpw.cRect.top);
      x += d;
      }

   SelectObject(hdc, hObj);                                  /* restore pen */
   return;
   }
/* ======================================================================== */
void drawGPSTrace(HDC hdc,
                  struct tgxmViewControl *tvc) {
   int i, n, l, istat;
   struct tsLatLng gp;
   struct tsTilePixel tspxNext;
   int itx, ity;                                           /* tileset pixel */
   int iwx, iwy;                                            /* window pixel */
   HGDIOBJ hObj;
/* ------------------------------------------------------------------------ */
   if (tvc->hWndGpsMntr == NULL) return;            /* no active connection */
   if (tvc->lGpsLoc == -1) return;               /* no message received yet */
   debugTrace("***Drawing GPS trace");

   hObj = SelectObject(hdc, tvc->tracePen);
   l = 0;                                           /* line has not started */
   for (n = 0, i = tvc->lGpsLoc + 2;                /* some breathing space */
        n < tvc->mGpsLoc;
        n++, i++) {
      if (i >= tvc->mGpsLoc) i -= tvc->mGpsLoc;
      gp.lat = tvc->gpsLoc[i].lat;
      gp.lng = ts_LongitudeTileset(tvc->tsrp->centerLng, tvc->gpsLoc[i].lng);
      istat = ts_MapRect(tvc->tsh, tvc->tsrp, &gp, &tspxNext);
      if (istat) l = 0;
      else {
         itx = tspxNext.tileColumn * tvc->tsh->tileWidth + tspxNext.pxl_x;
         ity = tspxNext.tileRow * tvc->tsh->tileHeight + tspxNext.pxl_y;
         ts2win(&tvc->mpw, itx, ity, &iwx, &iwy);     /* window coordinates */
         if (l) LineTo(hdc, iwx, iwy);
         else MoveToEx(hdc, iwx, iwy, NULL);
         l++;
         }
      }
   SelectObject(hdc, hObj);                                  /* restore pen */
   return;
   }
/* ======================================================================== */
/* for window range in decimal degrees, return integer grid step in seconds */
int gridDensity(double range) {           /* window size in decimal degrees */
   int istep;
/* ------------------------------------------------------------------------ */
   if (range < 1.0 / 60.0) istep = 5; /* range up to 1 minute - step 5 seconds */
   else if (range < 1.0 / 30.0) istep = 10;       /* 2 minutes - 10 seconds */
   else if (range < 1.0 / 15.0) istep = 20;       /* 4 minutes - 20 seconds */
   else if (range < 1.0 /  6.0) istep = 60;        /* 10 minutes - 1 minute */
   else if (range < 1.0 /  2.0) istep = 120;       /* 30 minutes - 2 minute */
   else if (range < 1.0) istep = 600;               /* 1 degree - 10 minute */
   else if (range < 5.0) istep = 1800;              /* 1 degree - 30 minute */
   else if (range < 10.0) istep = 3600;            /* 10 degrees - 1 degree */
   else istep = 18000;                                         /* 5 degrees */
   return(istep);
   }
/* ======================================================================== */
void updateScrollGeom(HWND hWnd,
                      struct tgxmViewControl *tvc) {
   SCROLLINFO scrInfo;
   int l, m, n;
/* ------------------------------------------------------------------------ */
   if (tvc->fm.lpvFile == NULL) return;

/* debugTrace("Updating scroll geometry...");*/
/* debugTrace("Window:  %d x %d", tvc->mpw.cRect.right, tvc->mpw.cRect.bottom);*/
/* debugTrace("Tileset: %d x %d", TS_PX_WIDTH(tvc->tsh), TS_PX_HEIGHT(tvc->tsh));*/
/* debugTrace("Lat Y, low: %d, high: %d", tvc->tsrp->lowLatY, tvc->tsrp->highLatY);*/

   scrInfo.cbSize = sizeof(SCROLLINFO);
   scrInfo.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
   scrInfo.nMin = 0;
   scrInfo.nMax = TS_PX_WIDTH(tvc->tsh);
   scrInfo.nPage = tvc->mpw.cRect.right - tvc->mpw.cRect.left;
   scrInfo.nPos = tvc->mpw.crcTile.pxl_x - scrInfo.nPage / 2;
   scrInfo.nTrackPos = 0;
/* debugTrace("Horizontal range: %d page: %d position: %d", scrInfo.nMax - scrInfo.nMin, scrInfo.nPage, scrInfo.nPos);*/
   SetScrollInfo(hWnd, SB_HORZ, &scrInfo, TRUE);

/* Latitude range is restricted to nominal south/north tileset latitudes.   */
   scrInfo.nMin = 0;
   scrInfo.nMax = tvc->tsrp->highLatY - tvc->tsrp->lowLatY;
   scrInfo.nPage = tvc->mpw.cRect.bottom - tvc->mpw.cRect.top;
   m = TS_PX_HEIGHT(tvc->tsh) - tvc->mpw.crcTile.pxl_y;
   n = TS_PX_HEIGHT(tvc->tsh) - tvc->tsrp->highLatY;
   l = tvc->mpw.cRect.bottom - tvc->mpw.cRect.bottom / 2;
   scrInfo.nPos = m - n - l;
/* debugTrace("Vertical range: %d page: %d position: %d", scrInfo.nMax - scrInfo.nMin, scrInfo.nPage, scrInfo.nPos);*/
   SetScrollInfo(hWnd, SB_VERT, &scrInfo, TRUE);
   return;
   }
/* ======================================================================== */
void displayInfo(HWND hWnd,
                 HDC hdc,
                 struct tgxmViewControl *tvc) {
   RECT cr;
   int i0x, i0y;
/* ------------------------------------------------------------------------ */
   GetClientRect(hWnd, &cr);
   i0x = cr.right / 2;
   i0y = cr.bottom / 2;
   if (tvc->fm.lpvFile) SetBkColor(hdc, RGB(255, 255, 255));
   else SetBkColor(hdc, tvc->bakCol);
   SetTextColor(hdc, RGB(102, 0, 0));
   SetTextAlign(hdc, TA_CENTER);
   SelectObject(hdc, tvc->font2);
   TextOut(hdc, i0x, i0y - 48, "ViewTgxm", 8);
   SelectObject(hdc, tvc->font1);
   TextOut(hdc, i0x, i0y, "Map and GPS trace view program", 30);
   SelectObject(hdc, tvc->font1);
   TextOut(hdc, i0x, i0y + 22, "www.geodyssey.com/tileshare", 27);
   return;
   }
/* ======================================================================== */
