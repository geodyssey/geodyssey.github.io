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
LRESULT CALLBACK mapWinProc(HWND hWnd,
                            UINT iMsg,
                            WPARAM wParam,
                            LPARAM lParam) {
/* ------------------------------------------------------------------------ */
   switch(iMsg) {
/*    --------------------------------------------------------------------- */
      case WM_CREATE: {
         LPCREATESTRUCT lpcs;
         struct tgxmViewControl *tvc;
         lpcs = (LPCREATESTRUCT)lParam;
         tvc = (struct tgxmViewControl *)(lpcs->lpCreateParams);
         SetWindowLong(hWnd, GWL_USERDATA, (DWORD)tvc);
         return(0);                                 /* message is processed */
         }
/*    --------------------------------------------------------------------- */
      case WM_INITMENUPOPUP: {
         HMENU hmenuInit, mmenu;
         struct tgxmViewControl *tvc = (struct tgxmViewControl *)GetWindowLong(hWnd, GWL_USERDATA);
         mmenu = GetMenu(hWnd);
         hmenuInit = (HMENU)wParam;
         if (hmenuInit == GetSubMenu(mmenu, IDM_FILE_POPPOS)) {
            EnableMenuItem(mmenu, IDM_RPRT, tvc->fm.lpvFile ? MF_ENABLED : MF_GRAYED);
            }
         return(0);
         }
/*    --------------------------------------------------------------------- */
      case WM_COMMAND: {
         HMENU hMenu;
         RECT clRect;
         HWND gmHwnd;
         struct tgxmViewControl *tvc = (struct tgxmViewControl *)GetWindowLong(hWnd, GWL_USERDATA);
         hMenu = GetMenu(hWnd);
         GetClientRect(hWnd, &clRect);

         switch (LOWORD(wParam)) {

            case IDM_OPEN: {
               char tsFn[FILENAME_MAX + 2];
               int istat;
               RECT cr;
               if (selectTileset(hWnd, tsFn) == 0) return(0);
               closeTgxm(&tvc->fm);
               istat = openTgxm(tvc, tsFn);
               if (istat) {
                  if ((istat == 1) || (istat == 2) || (istat == 3)) userError("Unable to open file [%s], status: %d", tsFn, istat);
                  else if (istat == 4) userError("File [%s] does not appear to be a tileset file, status: %d", tsFn, istat);
                  else if (istat == 5) userError("File [%s] appears corrupt, status: %d", tsFn, istat);
                  else if (istat == 6) userError("File [%s] unable to allocate memory for tile buffer", tsFn);
                  return(0);
                  }
               SetWindowText(hWnd, tvc->fn);
               GetClientRect(hWnd, &cr);
               if (resizeWindow(tvc, &cr) == 0) updateScrollGeom(hWnd, tvc);
               tvc->dispInfo = 1;    /* display it again */
               InvalidateRect(hWnd, NULL, TRUE);

               debugTrace("new tileset opened: %s", tvc->fn);
               return(0);
               }

            case IDM_RPRT:
            if (tvc->fm.lpvFile) reportFile(hWnd, tvc);
            return(0);

            case IDM_EXIT:
            if (MessageBox(hWnd, "Exit?", APP_NAME, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES) DestroyWindow(hWnd);
            return(0);

            case IDM_TILES:
            if (tvc->drawFlags & DRAW_PIXELS_A) {
               CheckMenuItem(hMenu, IDM_TILES, MF_UNCHECKED);
               tvc->drawFlags &= ~(DRAW_PIXELS_A);
               }
            else {
               CheckMenuItem(hMenu, IDM_TILES, MF_CHECKED);
               tvc->drawFlags |= DRAW_PIXELS_A;
               }
            if (tvc->fm.lpvFile) InvalidateRect(hWnd, NULL, TRUE);
            return(0);

            case IDM_GPSTRACE:
            if (tvc->drawFlags & DRAW_GPSTRACE) {
               CheckMenuItem(hMenu, IDM_GPSTRACE, MF_UNCHECKED);
               tvc->drawFlags &= ~(DRAW_GPSTRACE);
               }
            else {
               CheckMenuItem(hMenu, IDM_GPSTRACE, MF_CHECKED);
               tvc->drawFlags |= DRAW_GPSTRACE;
               }
            if (tvc->fm.lpvFile) InvalidateRect(hWnd, NULL, TRUE);
            return(0);

            case IDM_LLGRID:
            if (tvc->drawFlags & DRAW_LATLNG) {
               CheckMenuItem(hMenu, IDM_LLGRID, MF_UNCHECKED);
               tvc->drawFlags &= ~(DRAW_LATLNG);
               }
            else {
               CheckMenuItem(hMenu, IDM_LLGRID, MF_CHECKED);
               tvc->drawFlags |= DRAW_LATLNG;
               }
            if (tvc->fm.lpvFile) InvalidateRect(hWnd, NULL, TRUE);
            return(0);

            case IDM_TILEBDY:
            if (tvc->drawFlags & DRAW_TILEBDY) {
               CheckMenuItem(hMenu, IDM_TILEBDY, MF_UNCHECKED);
               tvc->drawFlags &= ~(DRAW_TILEBDY);
               }
            else {
               CheckMenuItem(hMenu, IDM_TILEBDY, MF_CHECKED);
               tvc->drawFlags |= DRAW_TILEBDY;
               }
            if (tvc->fm.lpvFile) InvalidateRect(hWnd, NULL, TRUE);
            return(0);

            case IDM_GPSON:
            if (tvc->hWndGpsMntr) {          /* disconnect from the monitor */
               if (tvc->gpsLoc) UnmapViewOfFile(tvc->gpsLoc);
               tvc->gpsLoc = NULL;
               if (tvc->smm) CloseHandle(tvc->smm);
               tvc->smm = NULL;
               PostMessage(tvc->hWndGpsMntr, UM_GPS_MONITOR_REQUEST, (WPARAM)0, (LPARAM)hWnd);
               debugTrace("GPS monitor release submitted (%d).", tvc->hWndGpsMntr);
               tvc->hWndGpsMntr = NULL;
               CheckMenuItem(hMenu, IDM_GPSON, MF_UNCHECKED);
               }
            else {                              /* connect with the monitor */
               gmHwnd = FindWindow(GPS_MONITOR_CLASS, NULL);
               if (gmHwnd == NULL) userError("No GPS monitor program (program:%s class:%s) appears to be running.", GPS_MONITOR_PGM, GPS_MONITOR_CLASS);
               else {
                  PostMessage(gmHwnd, UM_GPS_MONITOR_REQUEST, (WPARAM)1, (LPARAM)hWnd);  /* apply for data */
                  debugTrace("GPS data request submitted (%d).", gmHwnd);
                  }
               }
            return(0);

            case IDM_GPSMOVE:
            if (tvc->mapMovable) {
               CheckMenuItem(hMenu, IDM_GPSMOVE, MF_UNCHECKED);
               tvc->mapMovable = 0;
               }
            else {
               CheckMenuItem(hMenu, IDM_GPSMOVE, MF_CHECKED);
               tvc->mapMovable = 1;
               }
            return(0);

            case IDM_ABOUT:
            userInfo("Simple tgxm map viewer.\nBuild date:%s\nGeodyssey Limited, 2004.\nhttp://www.geodyssey.com/tileshare", __DATE__);
            return(0);
            }

         return(DefWindowProc(hWnd, iMsg, wParam, lParam));
         }
/*    --------------------------------------------------------------------- */
      case UM_GPS_POSITION_MESSAGE: {         /* gps monitor sends location */
         struct tsTilePixel tspxNext;
         struct tsLatLng gp;                              /* traverse point */
         int n, m, istat;
         int itx, ity;                                     /* tileset pixel */
         int iwx, iwy;                                      /* window pixel */
         HGDIOBJ hObj;
         HDC hDc;
         RECT cr;
         struct tgxmViewControl *tvc = (struct tgxmViewControl *)GetWindowLong(hWnd, GWL_USERDATA);
         if (tvc->gpsLoc == NULL) return(0);            /* is someone sick? */
         tvc->lGpsLoc = (int)wParam;            /* mark last-given position */
         debugTrace("Received location message %d", UM_GPS_POSITION_MESSAGE);
         debugTrace("Location at: %d %10.6f %10.6f\n", tvc->lGpsLoc, TS_RAD2DEG * TS_AngOfI4(tvc->gpsLoc[tvc->lGpsLoc].lat), TS_RAD2DEG * TS_AngOfI4(tvc->gpsLoc[tvc->lGpsLoc].lng));
         gp.lat = tvc->gpsLoc[tvc->lGpsLoc].lat;
         gp.lng = ts_LongitudeTileset(tvc->tsrp->centerLng, tvc->gpsLoc[tvc->lGpsLoc].lng);
         debugTrace("l/l, local: %10.6f %10.6f", TS_RAD2DEG * TS_AngOfI4(gp.lat), TS_RAD2DEG * TS_AngOfI4(gp.lng));
         if ((gp.lat == TS_I4A_UNDEF) || (gp.lng == TS_I4A_UNDEF)) {   /* No signal? */
            tvc->tspxPrev.pxl_x = TS_PIX_UNDEF;
            return(0);
            }
         else {                                      /* valid location data */
            istat = ts_MapRect(tvc->tsh, tvc->tsrp, &gp, &tspxNext);
            debugTrace("tile:   %d:%d %d:%d", tspxNext.tileColumn, tspxNext.pxl_x, tspxNext.tileRow, tspxNext.pxl_y);
            if (istat) {
               tvc->tspxPrev.pxl_x = TS_PIX_UNDEF;
               return(0);
               }
            if ((tvc->drawFlags & DRAW_GPSTRACE)
             && (tvc->tspxPrev.pxl_x != TS_PIX_UNDEF)) {
               hDc = GetDC(hWnd);
               hObj = SelectObject(hDc, tvc->tracePen);
               itx = tvc->tspxPrev.tileColumn * tvc->tsh->tileWidth + tvc->tspxPrev.pxl_x;
               ity = tvc->tspxPrev.tileRow * tvc->tsh->tileHeight + tvc->tspxPrev.pxl_y;
               ts2win(&tvc->mpw, itx, ity, &iwx, &iwy);  /* window coordinates */
               MoveToEx(hDc, iwx, iwy, NULL);
               itx = tspxNext.tileColumn * tvc->tsh->tileWidth + tspxNext.pxl_x;
               ity = tspxNext.tileRow * tvc->tsh->tileHeight + tspxNext.pxl_y;
               debugTrace("tilest: %d %d", itx, ity);
               ts2win(&tvc->mpw, itx, ity, &iwx, &iwy);  /* window coordinates */
               debugTrace("window: %d %d", iwx, iwy);
               LineTo(hDc, iwx, iwy);
               hObj = SelectObject(hDc, hObj);
               ReleaseDC(hWnd, hDc);
               if (tvc->mapMovable) {
                  GetClientRect(hWnd, &cr);
                  n = (cr.right - cr.left) / EDGE_FRACTION;
                  m = (cr.bottom - cr.top) / EDGE_FRACTION;
                  if ((iwx < (cr.left + n))
                     || (iwx > (cr.right - n))
                     || (iwy < (cr.top + m))
                     || (iwy > (cr.bottom - m))) {
/*                      Attempt to re-center map to the current position.   */
                     istat = positionWindow(tvc, &gp);
/*                   debugTrace("Position status: %d", istat);*/
                     if ((istat == 0) || (istat == -1)) {
                        updateScrollGeom(hWnd, tvc);
                        InvalidateRect(hWnd, NULL, TRUE);
                        }
                     }
                  }
               }
            tvc->tspxPrev = tspxNext;
            }
         return(0);
         }
/*    --------------------------------------------------------------------- */
      case UM_GPS_MONITOR_ACQ: {       /* gps monitor confirms registration */
         struct tgxmViewControl *tvc = (struct tgxmViewControl *)GetWindowLong(hWnd, GWL_USERDATA);
         debugTrace("Received confirmation message %d", UM_GPS_MONITOR_ACQ);
         debugTrace("Mapping object: %d monitor process: %d\n", (int)wParam, (int)lParam);
         tvc->smm = CreateFileMapping((HANDLE)0xFFFFFFFF, NULL, PAGE_READONLY, 0, (int)wParam * sizeof(struct tsGpsLocation), GPS_MONITOR_SMO);
         if (tvc->smm == NULL) {
            userError("Unable to create location buffer shared object?");
            return(1);
            }
         tvc->gpsLoc = (struct tsGpsLocation *)MapViewOfFile(tvc->smm, FILE_MAP_READ, 0, 0, 0);
         if (tvc->gpsLoc == NULL) {
            CloseHandle(tvc->smm);
            tvc->smm = NULL;
            userError("Unable to connect to location buffer?");
            return(1);
            }
         if (IsBadReadPtr(tvc->gpsLoc, (int)wParam * sizeof(struct tsGpsLocation))) {
            if (tvc->gpsLoc) UnmapViewOfFile(tvc->gpsLoc);
            tvc->gpsLoc = NULL;
            if (tvc->smm) CloseHandle(tvc->smm);
            tvc->smm = NULL;
            userError("Unable to read location buffer ?");
            return(1);
            }
         tvc->hWndGpsMntr = (HWND)lParam;      /* we did get a confirmation */
         tvc->mGpsLoc = (int)wParam;
         tvc->lGpsLoc = -1;                                    /* undefined */
         tvc->tspxPrev.pxl_x = TS_PIX_UNDEF;                    /* likewise */
         CheckMenuItem(GetMenu(hWnd), IDM_GPSON, MF_CHECKED);
         debugTrace("Application acknowledged, from:%d at: %d of: %d\n", (int)tvc->hWndGpsMntr, (int)tvc->gpsLoc, tvc->mGpsLoc);
         return(0);
         }
/*    --------------------------------------------------------------------- */
      case WM_HSCROLL: {
         int n, moved;
         int move = 0;
         struct tgxmViewControl *tvc = (struct tgxmViewControl *)GetWindowLong(hWnd, GWL_USERDATA);
         if (tvc->fm.lpvFile == NULL) return(0);

         switch (LOWORD(wParam)) {
            case SB_LINEUP:     move = -8; break;
            case SB_LINEDOWN:   move =  8; break;
            case SB_PAGEUP:     move = -(tvc->mpw.cRect.right - 8); break;
            case SB_PAGEDOWN:   move =   tvc->mpw.cRect.right - 8;  break;
            case SB_THUMBTRACK: move =   HIWORD(wParam) - GetScrollPos(hWnd, SB_HORZ); break;
            }
         if (move) {
            moved = moveWindow(tvc, move, MOVE_EAST_WEST);
            if (moved) {
               n = GetScrollPos(hWnd, SB_HORZ);
               SetScrollPos(hWnd, SB_HORZ, n + moved, TRUE);
               InvalidateRect(hWnd, NULL, TRUE);
               }
            }
         return(0);
         }
/*    --------------------------------------------------------------------- */
      case WM_VSCROLL: {
         int n, moved;
         int move = 0;
         struct tgxmViewControl *tvc = (struct tgxmViewControl *)GetWindowLong(hWnd, GWL_USERDATA);
         if (tvc->fm.lpvFile == NULL) return(0);

         switch (LOWORD(wParam)) {
            case SB_LINEUP:     move =  8; break;
            case SB_LINEDOWN:   move = -8; break;
            case SB_PAGEUP:     move =   tvc->mpw.cRect.bottom - 8;  break;
            case SB_PAGEDOWN:   move = -(tvc->mpw.cRect.bottom - 8); break;
            case SB_THUMBTRACK: move = -(HIWORD(wParam) - GetScrollPos(hWnd, SB_VERT)); break;
            }
         if (move) {
            moved = moveWindow(tvc, move, MOVE_NORTH_SOUTH);
            if (moved) {
               n = GetScrollPos(hWnd, SB_VERT);
               SetScrollPos(hWnd, SB_VERT, n - moved, TRUE);
               InvalidateRect(hWnd, NULL, TRUE);
               }
            }
         return(0);
         }
/*    --------------------------------------------------------------------- */
      case WM_SIZE: {
         RECT cr;
         struct tgxmViewControl *tvc = (struct tgxmViewControl *)GetWindowLong(hWnd, GWL_USERDATA);
         if ((tvc->fm.lpvFile == NULL) || (LOWORD(lParam) == 0) || (HIWORD(lParam) == 0)) return(DefWindowProc(hWnd, iMsg, wParam, lParam));
         GetClientRect(hWnd, &cr);
         if (resizeWindow(tvc, &cr) <= 0) {
            updateScrollGeom(hWnd, tvc);
            InvalidateRect(hWnd, NULL, TRUE);
            }
         return(0);
         }
/*    --------------------------------------------------------------------- */
      case WM_LBUTTONDOWN: {                                /* recenter map */
         struct tgxmViewControl *tvc = (struct tgxmViewControl *)GetWindowLong(hWnd, GWL_USERDATA);
         int istat;
         struct tsPixel tspx;
         struct tsTilePixel tstp;
         struct tsLatLng tll;                            /* tileset lat/lng */
         int iwx, iwy;

         if (tvc->fm.lpvFile == NULL) return(0);
         iwx = LOWORD(lParam);
         iwy = HIWORD(lParam);
         win2ts(&tvc->mpw, iwx, iwy, &tspx.pxl_x, &tspx.pxl_y);
         istat = ts_PixelToTilePixel(tvc->tsh, &tspx, &tstp);
         if (istat) {
            debugTrace("unexpected location error?");
            MessageBeep(MB_ICONHAND);
            return(0);
            }
         istat = ts_UnMapRect(tvc->tsh, tvc->tsrp, &tstp, &tll);
         if (istat) {
            debugTrace("unexpected location error?");
            MessageBeep(MB_ICONHAND);
            return(0);
            }
/*       debugTrace("Tile ll: %c%d:%06.3f, %c%d:%06.3f", TS_SIGN_LAT(tll.lat), TS_INTDEG_LL(tll.lat), TS_DECMIN_LL(tll.lat), TS_SIGN_LNG(tll.lng), TS_INTDEG_LL(tll.lng), TS_DECMIN_LL(tll.lng));*/
         istat = positionWindow(tvc, &tll);
/*       debugTrace("Position status: %d", istat);*/
         if (istat == -2) return(0);
         if (istat > 0) MessageBeep(MB_ICONHAND);
         else {
            updateScrollGeom(hWnd, tvc);
            InvalidateRect(hWnd, NULL, TRUE);
            }
         return(0);
         }
/*    --------------------------------------------------------------------- */
      case WM_RBUTTONDOWN: {                          /* coordinate readout */
         struct tgxmViewControl *tvc = (struct tgxmViewControl *)GetWindowLong(hWnd, GWL_USERDATA);
         int lgl;
         int istat;
         int iwx, iwy;
         struct tsPixel tspx;
         struct tsTilePixel tstp;
         struct tsLatLng gp;                    /* tileset coordinates */
         if (tvc->fm.lpvFile == NULL) return(0);           /* nothing to do */

         iwx = LOWORD(lParam);
         iwy = HIWORD(lParam);
         debugTrace("window: %d %d", iwx, iwy);
         win2ts(&tvc->mpw, iwx, iwy, &tspx.pxl_x, &tspx.pxl_y);
         debugTrace("tilest: %d %d", tspx.pxl_x, tspx.pxl_y);
         istat = ts_PixelToTilePixel(tvc->tsh, &tspx, &tstp);
         debugTrace("tile:   %d:%d %d:%d", tstp.tileColumn, tstp.pxl_x, tstp.tileRow, tstp.pxl_y);
         if (istat) {
            debugTrace("unexpected location error?");
            MessageBeep(MB_ICONHAND);
            return(0);
            }
         istat = ts_UnMapRect(tvc->tsh, tvc->tsrp, &tstp, &gp);
         debugTrace("l/l, local: %10.6f %10.6f", TS_RAD2DEG * TS_AngOfI4(gp.lat), TS_RAD2DEG * TS_AngOfI4(gp.lng));
         if (istat) {
            debugTrace("unexpected location error?");
            MessageBeep(MB_ICONHAND);
            return(0);
            }
         lgl = ts_LongitudeGlobal(tvc->tsrp->centerLng, gp.lng);
         debugTrace("l/l, global: %10.6f %10.6f", TS_RAD2DEG * TS_AngOfI4(gp.lat), TS_RAD2DEG * TS_AngOfI4(lgl));
         userInfo("%c%d:%06.4f, %c%d:%06.4f\n(%d:%d %d:%d)\n",
            TS_SIGN_LAT(gp.lat), TS_INTDEG_LL(gp.lat), TS_DECMIN_LL(gp.lat),
            TS_SIGN_LNG(lgl), TS_INTDEG_LL(lgl), TS_DECMIN_LL(lgl),
/*          TS_RAD2DEG * TS_AngOfI4(gp.lat), TS_RAD2DEG * TS_AngOfI4(lgl),*/
            tstp.tileColumn, tstp.pxl_x, tstp.tileRow, tstp.pxl_y);
/*       debug/trace pixel/ground pileline: */
/*       gp.lng = ts_LongitudeTileset(tvc->tsrp->centerLng, lgl);*/
/*       debugTrace("l/l, local: %10.6f %10.6f", TS_RAD2DEG * TS_AngOfI4(gp.lat), TS_RAD2DEG * TS_AngOfI4(gp.lng));*/
/*       istat = ts_MapRect(tvc->tsh, tvc->tsrp, &gp, &tstp);*/
/*       debugTrace("tile:   %d:%d %d:%d", tstp.tileColumn, tstp.pxl_x, tstp.tileRow, tstp.pxl_y);*/

         return(0);
         }
/*    --------------------------------------------------------------------- */
      case WM_CHAR: {
         struct tgxmViewControl *tvc = (struct tgxmViewControl *)GetWindowLong(hWnd, GWL_USERDATA);
         if (tvc->fm.lpvFile == NULL) return(0);
         return(0);
         }
/*    --------------------------------------------------------------------- */
      case WM_PAINT: {
         PAINTSTRUCT ps;
         HDC hdc;
         struct tgxmViewControl *tvc = (struct tgxmViewControl *)GetWindowLong(hWnd, GWL_USERDATA);

         hdc = BeginPaint(hWnd, &ps);
         if (tvc->fm.lpvFile) {                 /* there is an open tileset */
            if (tvc->drawFlags & DRAW_PIXELS_A) paintWindowTiles(hdc, tvc);
            if (tvc->drawFlags & DRAW_LATLNG) drawLatLongGrid(hdc, tvc);
            if (tvc->drawFlags & DRAW_TILEBDY) drawTileBoundaries(hdc, tvc);
            if (tvc->drawFlags & DRAW_GPSTRACE) drawGPSTrace(hdc, tvc);
            }
         if (tvc->dispInfo) {
            displayInfo(hWnd, hdc, tvc);
            tvc->dispInfo = 0;
            }
         EndPaint(hWnd, &ps);

         return(0);
         }
/*    --------------------------------------------------------------------- */
      case WM_ERASEBKGND: {
         struct tgxmViewControl *tvc = (struct tgxmViewControl *)GetWindowLong(hWnd, GWL_USERDATA);
         if (tvc->fm.lpvFile) return(1);
         else return(DefWindowProc(hWnd, iMsg, wParam, lParam));
         }
/*    --------------------------------------------------------------------- */
      case WM_CLOSE: {
         struct tgxmViewControl *tvc = (struct tgxmViewControl *)GetWindowLong(hWnd, GWL_USERDATA);
         if (MessageBox(hWnd, "Exit?", APP_NAME, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES) {
            if (tvc->fm.lpvFile) closeTgxm(&tvc->fm);
            DestroyWindow(hWnd);
            }
         return(0);
         }
/*    --------------------------------------------------------------------- */
      case WM_DESTROY: {
         struct tgxmViewControl *tvc = (struct tgxmViewControl *)GetWindowLong(hWnd, GWL_USERDATA);
         if (tvc->hWndGpsMntr) {
            if (tvc->gpsLoc) UnmapViewOfFile(tvc->gpsLoc);
            tvc->gpsLoc = NULL;
            if (tvc->smm) CloseHandle(tvc->smm);
            tvc->smm = NULL;
            PostMessage(tvc->hWndGpsMntr, UM_GPS_MONITOR_REQUEST, (WPARAM)0, (LPARAM)hWnd);
            tvc->hWndGpsMntr = 0;
            }
         PostQuitMessage(0);
         return(0);
         }
      }
   return(DefWindowProc(hWnd, iMsg, wParam, lParam));
   }
/* ======================================================================== */
