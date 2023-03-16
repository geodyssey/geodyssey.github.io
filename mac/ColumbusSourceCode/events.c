/* events.c: event handlers used by columbus program. */

#include "columbus.h"
/* ======================================================================== */
pascal OSStatus windowUpdateSizePosition(EventHandlerCallRef handlerRef,
                                         EventRef event,
                                         void *userData) {
   CGrafPtr port;
   UInt32 eventKind;
   UInt32 eventAttributes;
   struct mapControl *mc;
   /* ------------------------------------------------------------------------ */
   eventKind = GetEventKind(event);
   GetEventParameter(event, kEventParamAttributes, typeSInt32, NULL, sizeof(UInt32), NULL, &eventAttributes);

   mc = (struct mapControl *)GetWRefCon((WindowRef)userData);
   port = GetWindowPort((WindowRef)userData);

   if (eventAttributes & kWindowBoundsChangeOriginChanged) {
      SetPort(port);
      mc->winOrig.v = mc->winOrig.h = 0;
      LocalToGlobal(&mc->winOrig);
/*    fprintf(stderr, "Global origin changed to %d %d\n", mc->winOrig.v, mc->winOrig.h); */
      }

   if (eventAttributes & kWindowBoundsChangeSizeChanged) {
      GetPortBounds(port, &mc->winRect);
      if (updateGeometrySize(mc, mc->winRect.right - mc->winRect.left, mc->winRect.bottom - mc->winRect.top) == 0) InvalWindowRect((WindowRef)userData, &mc->winRect);
/*    fprintf(stderr, "Window size changed to %d %d %d %d\n", mc->winRect.top, mc->winRect.left, mc->winRect.bottom, mc->winRect.right); */
      }

   return(noErr);
   }
/* ======================================================================== */
pascal OSStatus drawWindowContent(EventHandlerCallRef handlerRef,
                                  EventRef event,
                                  void *userData) {
   Rect rect;
   CGrafPtr port;
   struct mapControl *mc;
   SInt16 i1, i2, j1, j2;
   struct dpxl pixa[5];
   int isphi, islam;
   const struct columbusControl *cc = ccDP();
   static struct RGBColor gridColor = {32767, 32767, 32767};
   static struct RGBColor backgroundColor = {65535, 65535, 65535 - 16384};
   static struct RGBColor coastlineColor = {16383, 16383, 0};
/* ------------------------------------------------------------------------ */
   mc = (struct mapControl *)GetWRefCon((WindowRef)userData);

   port = GetWindowPort((WindowRef)userData);
   SetPort(port);
   GetPortBounds(port, &rect);
   RGBBackColor(&backgroundColor);
   EraseRect(&rect);

   if (mc->wdw->projection == NOPROJECTION) {
/*    fprintf(stderr, "No geometry - nothing to draw.\n"); */
      ForeColor(magentaColor);    /* unexpected; draw "no-signal" pattern */
      i1 = j1 = 0; i2 = mc->winRect.right - mc->winRect.left - 1; j2 = mc->winRect.bottom - mc->winRect.top - 1;
      while ((i2 > i1) && (j2 > j1)) {
         pixa[0].pxl_x = i1; pixa[0].pxl_y = j1; pixa[1].pxl_x = i1; pixa[1].pxl_y = j2;
         pixa[2].pxl_x = i2; pixa[2].pxl_y = j2; pixa[3].pxl_x = i2; pixa[3].pxl_y = j1;
         pixa[4].pxl_x = i1; pixa[4].pxl_y = j1;
         HQDXpolyline(pixa, 5);
         i1 += 4; j1 += 4; i2 -= 4; j2 -= 4;
         }
      return(noErr);
      }

/* fprintf(stderr, "Drawing window content %d x %d.\n", mc->winRect.right - mc->winRect.left, mc->winRect.bottom - mc->winRect.top); */
   PenNormal();
   PenSize(1, 1);
   RGBForeColor(&gridColor);
   h9_GraphOutputStart(&mc->wdw->go, 0, 0, 0, NULL);
   setGridDensity(mc->wdw->dpScale, &isphi, &islam);
   h9_LatLongGrid(mc->wdw, isphi, islam);
   if (mc->wdw->projection == ORTHO) h9_Horizon(mc->wdw);
   RGBForeColor(&coastlineColor);
   h9_DrawSgVxFile(mc->wdw, cc->cstsFp, cc->cstvFp);
   h9_GraphOutputEnd(&mc->wdw->go);

   return(noErr);
   }
/* ======================================================================== */
pascal OSStatus mapWindowClose(EventHandlerCallRef handlerRef,
                               EventRef event,
                               void *userData) {
   WindowRef 	       window;
   struct mapControl *mc;                                 
/* ------------------------------------------------------------------------ */
   window = (WindowRef)userData;
   mc = (struct mapControl *)GetWRefCon(window);
/* fprintf(stderr, "Window %d  closed, memory at %p.\n", (int)window, mc);  */
   if (mc) {
      if (mc->wdw) {
         if (mc->wdw->projection != NOPROJECTION) {
/*          fprintf(stderr, "About to close map window.\n"); */
            h9_CloseWindow(mc->wdw);
            }
         h1_Free(mc->wdw);
         }
      h1_Free(mc);
      }
   return(eventNotHandledErr);
   }
/* ======================================================================== */
pascal OSStatus aboutInfoClose(EventHandlerCallRef handlerRef,
                               EventRef event,
                               void *userData) {
   OSStatus  err;
   UInt32    eventKind;
/* ------------------------------------------------------------------------ */
   err = eventNotHandledErr;
   eventKind = GetEventKind(event);
   if (eventKind == kEventWindowClose) {
      HideWindow((WindowRef)userData);
      err = noErr;
      }
   return(err);
   }
/* ======================================================================== */
pascal OSStatus mouseWheelMoved(EventHandlerCallRef handlerRef,
                                EventRef event,
                                void *userData) {
   EventMouseWheelAxis axis;
   SInt32 delta;
   struct mapControl *mc;
/* ------------------------------------------------------------------------ */
   mc = (struct mapControl *)GetWRefCon((WindowRef)userData);
   GetEventParameter(event, kEventParamMouseWheelAxis, typeMouseWheelAxis, NULL, sizeof(axis), NULL, &axis);
   GetEventParameter(event, kEventParamMouseWheelDelta, typeLongInteger, NULL, sizeof(delta), NULL, &delta);
   if (axis == kEventMouseWheelAxisY) {
/*    fprintf(stderr, "Wheel Y Moved, %d.\n", delta); */
      if (updateGeometryScale(mc, 1.0 - delta * 0.25) == 0) InvalWindowRect((WindowRef)userData, &mc->winRect);
      }
   return(noErr);
   }
/* ======================================================================== */
   pascal OSStatus mapLocationClick(EventHandlerCallRef handlerRef,
                                 EventRef event,
                                 void *userData) {

   struct mapControl *mc;
   Point clickPix;
   /* ------------------------------------------------------------------------ */
   mc = (struct mapControl *)GetWRefCon((WindowRef)userData);
   GetEventParameter(event, kEventParamMouseLocation, typeQDPoint, NULL, sizeof(clickPix), NULL, &clickPix);
   SubPt(mc->winOrig, &clickPix);
/* fprintf(stderr, "Mouse click x:%d y:%d.\n", clickPix.h, clickPix.v); */
   if (updateGeometryCenter(mc, clickPix.h, clickPix.v) == 0) InvalWindowRect((WindowRef)userData, &mc->winRect);
   return(noErr);
   }
/* ======================================================================== */
pascal OSStatus menuUpdate(EventHandlerCallRef handlerRef,
                           EventRef event,
                           void *userData) {
   OSStatus  err;
   Boolean first;
   MenuRef mrf, mrfProj, mrfScale;
   WindowRef actWin;
   struct mapControl *mc;
/* ------------------------------------------------------------------------ */
   err = eventNotHandledErr;
   GetEventParameter(event, kEventParamDirectObject, typeMenuRef, NULL, sizeof(SInt32), NULL, &mrf);
   GetEventParameter(event, kEventParamMenuFirstOpen, typeBoolean, NULL, sizeof(Boolean), NULL, &first);
   if (!first) return(eventNotHandledErr);
   mrfProj = GetMenuHandle(projectionMenuId);
   mrfScale = GetMenuHandle(scaleMenuId);
   if ((mrf != mrfProj) && (mrf != mrfScale)) return(eventNotHandledErr);
   if (mrf == mrfProj) {
      DisableMenuCommand(mrf, 'prjW');
      DisableMenuCommand(mrf, 'prjO');
      DisableMenuCommand(mrf, 'prjS');
      }
   if (mrf == mrfScale) {
      DisableMenuCommand(mrf, 'zmOt');
      DisableMenuCommand(mrf, 'zmIn');
      }
/* is there an active map window that the menu command applies to? */
   actWin = FrontWindow();
   if (!actWin) return(eventNotHandledErr);
   mc = (struct mapControl *)GetWRefCon(actWin);
   if (!mc) return(eventNotHandledErr);

/* fprintf(stderr, "Update Menu, map (mc) %p.\n", mc); */  
/* If we got this far we assume the active window is "one of ours",
   and that non-null refcon means it must be a map window. While we
   still will confirm it has an active Hippachus map control block
   and non-null projection, we don't really expect otherwise. */
   if ((mc->wdw == NULL) || (mc->wdw->projection == NOPROJECTION)) return(noErr);
   if (mrf == mrfProj) {
      if (mc->wdw->projection != WWVIEW) EnableMenuCommand(mrf, 'prjW');
      if (mc->wdw->projection != ORTHO) EnableMenuCommand(mrf, 'prjO');
      if (mc->wdw->projection != STEREO) EnableMenuCommand(mrf, 'prjS');
      }
   if (mrf == mrfScale) {
      if (2.0 * mc->wdw->dpScale <= MIN_SCALE) EnableMenuCommand(mrf, 'zmOt');
      if (0.5 * mc->wdw->dpScale >= MAX_SCALE) EnableMenuCommand(mrf, 'zmIn');
      }
   return(noErr);
   }
/* ======================================================================== */
pascal OSStatus appMenuCommand(EventHandlerCallRef handlerRef,
                               EventRef event,
                               void *userData) {
   OSStatus  err;
   HICommand command;
   const struct columbusControl *cc = ccDP();
/* ------------------------------------------------------------------------ */
   err = eventNotHandledErr;
   GetEventParameter(event, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &command);   
   switch (command.commandID) {
      case 'abtC':
      ShowWindow(cc->aboutWindow);
      SelectWindow(cc->aboutWindow);
      err = noErr;
      break;
      case 'newM':
      if (newMapWindow(NULL)) unexpectedError("Unable to create new map.", 0);
      err = noErr;
      break;
      case 'inFo':
      ShowWindow(cc->infoWindow);
      SelectWindow(cc->infoWindow);
      err = noErr;
      break;
      }
   return(err);
   }
/* ======================================================================== */
pascal OSStatus mapMenuCommand(EventHandlerCallRef handlerRef,
                               EventRef event,
                               void *userData) {
   OSStatus  err;
   HICommand command;
   struct mapControl *mc;
   /* ------------------------------------------------------------------------ */
   err = eventNotHandledErr;
   GetEventParameter(event, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &command);
   mc = (struct mapControl *)GetWRefCon((WindowRef)userData);
   switch (command.commandID) {
      case 'prjW':
      if (updateGeometryProjection(mc, WWVIEW) == 0) InvalWindowRect((WindowRef)userData, &mc->winRect);
/*    fprintf(stderr, "Proj.: World-View %p.\n", mc); */  
      err = noErr;
      break;
      case 'prjO':
      if (updateGeometryProjection(mc, ORTHO) == 0) InvalWindowRect((WindowRef)userData, &mc->winRect);
/*    fprintf(stderr, "Proj.: Ortho %p.\n", mc); */  
      err = noErr;
      break;
      case 'prjS':
      if (updateGeometryProjection(mc, STEREO) == 0) InvalWindowRect((WindowRef)userData, &mc->winRect);
/*    fprintf(stderr, "Proj.: Stereo %p.\n", mc); */  
      err = noErr;
      break;
      case 'zmOt':
      if (updateGeometryScale(mc, 2.0) == 0) InvalWindowRect((WindowRef)userData, &mc->winRect);
/*    fprintf(stderr, "Scale: Zoom Out %p.\n", mc); */  
      err = noErr;
      break;
      case 'zmIn':
      if (updateGeometryScale(mc, 0.5) == 0) InvalWindowRect((WindowRef)userData, &mc->winRect);
/*    fprintf(stderr, "Scale: Zoom In %p.\n", mc); */  
      err = noErr;
      break;
      }
   return(err);
   }
/* ======================================================================== */
      