/* viewtgxm_events.c */

#include "viewtgxm.h"
/* ======================================================================== */
/* menu command that is relative to the whole application.                  */
pascal OSStatus appMenuCommand(EventHandlerCallRef handlerRef,
                               EventRef event,
                               void *userData) {
   OSStatus  err;
   HICommand command;
   struct viewtgxmControl *vtc = getVTC(NULL);
/* ------------------------------------------------------------------------ */
   err = eventNotHandledErr;
   GetEventParameter(event, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &command); 
 
   DBG_TRACE((stderr, "App menu command %c%c%c%c.\n", ((char *)&command.commandID)[0], ((char *)&command.commandID)[1], ((char *)&command.commandID)[2], ((char *)&command.commandID)[3]));
   switch (command.commandID) {
      case 'abou':
      ShowWindow(vtc->aboutWindow);
      SelectWindow(vtc->aboutWindow);
      err = noErr;
      break;
      case 'open':
      if (getTilesetFileName() == 0) openMapWindow(NULL);
      err = noErr;
      break;
      case 'inFo':
      ShowWindow(vtc->infoWindow);
      SelectWindow(vtc->infoWindow);
      err = noErr;
      break;
      }
   return(err);
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
/* Processing call-backs by the file selection dialog.                      */
pascal void fileNavCallBack(NavEventCallbackMessage callBackSelector,
                            NavCBRecPtr             callBackParms,
                            void*                   callBackUD) {
   OSStatus  err;
   NavReplyRecord reply;
   NavUserAction userAction;
   AEDesc fd;
   FSRef  fr;
   struct viewtgxmControl *vtc = getVTC(NULL);
/* ------------------------------------------------------------------------ */
   userAction = 0;

   switch (callBackSelector) {
      
      case kNavCBUserAction:
      err = NavDialogGetReply(callBackParms->context, &reply);
      if (err != noErr) {
         if (err != -128) putAlert(makeMessage("Unexpected file navigation error (%d).", err), ALERT_CONTINUE);
         return;
         }
      userAction = NavDialogGetUserAction(callBackParms->context);
      switch (userAction) {
         case kNavUserActionOpen:
         DBG_TRACE((stderr, "User selection done\n"));
         err = AECoerceDesc(&reply.selection, typeFSRef, &fd);
         if (err) {
            AEDisposeDesc(&fd);                                                
            putAlert("Unexpected file name retrieval error (1).", ALERT_CONTINUE);
            return;
            }
         err = AEGetDescData (&fd, (void *)(&fr), sizeof(FSRef));
         AEDisposeDesc(&fd);                                                
         if (err) {
            putAlert("Unexpected file name retrieval error (2).", ALERT_CONTINUE);
            return;
            }
         err = FSRefMakePath(&fr, vtc->tilesetFileName, FILENAME_MAX);
         CFRelease(&fr);
         AEDisposeDesc(&fd);                                                         
         if (err) {
            putAlert("Unexpected file name retrieval error (3).", ALERT_CONTINUE);
            return;
            }
         break;
         }

      err = NavDisposeReply(&reply);   
      break;
      
      case kNavCBTerminate:
      DBG_TRACE((stderr, "Dialog closed.\n"));
      NavDialogDispose(callBackParms->context);
      DisposeNavEventUPP(vtc->fnep);
      break;
      }
   
   return;
   }
/* ======================================================================== */
/* Map window has been resized or moved.                                    */
pascal OSStatus windowSizeMove(EventHandlerCallRef handlerRef, 
                                 EventRef event, 
                                 void *userData) {
   CGrafPtr port;
   UInt32 eventKind;
   UInt32 eventAttributes;
   struct Rect newRect;
   struct mapControl *mc = (struct mapControl *)GetWRefCon((WindowRef)userData);
/* ------------------------------------------------------------------------ */
/* DBG_TRACE((stderr, "Move/Size window %p.\n", mc->mapWindow)); */
   eventKind = GetEventKind(event);
   GetEventParameter(event, kEventParamAttributes, typeSInt32, NULL, sizeof(UInt32), NULL, &eventAttributes);
   port = GetWindowPort((WindowRef)userData);

   if (eventAttributes & kWindowBoundsChangeOriginChanged) {
      SetPort(port);
      mc->winOrig.v = mc->winOrig.h = 0;
      LocalToGlobal(&mc->winOrig);
      DBG_TRACE((stderr, "Global origin changed to %d %d\n", mc->winOrig.h, mc->winOrig.v)); 
      }

   if (eventAttributes & kWindowBoundsChangeSizeChanged) {
      GetPortBounds(port, &newRect);
      DBG_TRACE((stderr, "Old size: %d %d %d %d\n", mc->winRect.top, mc->winRect.left, mc->winRect.bottom, mc->winRect.right)); 
      DBG_TRACE((stderr, "New size: %d %d %d %d\n", newRect.top, newRect.left, newRect.bottom, newRect.right)); 
      if (!EqualRect(&newRect, &mc->winRect)) {
         if (resizeWindow(mc, &newRect) <= 0) InvalWindowRect((WindowRef)userData, &mc->winRect);
         DBG_TRACE((stderr, "UPD size: %d %d %d %d\n", mc->winRect.top, mc->winRect.left, mc->winRect.bottom, mc->winRect.right)); 
         }
      }

   return(noErr);
   }
/* ======================================================================== */
/* OS determined that the window is to be redrawn.                          */
pascal OSStatus drawWindowContent(EventHandlerCallRef handlerRef, 
                                  EventRef event, 
                                  void *userData) {
 
   struct mapControl *mc = (struct mapControl *)GetWRefCon((WindowRef)userData);
/* ------------------------------------------------------------------------ */
   DBG_TRACE((stderr, "Draw window %p.\n", mc->mapWindow));
   if (mc->drawFlags & DRAW_PIXELS_A) paintWindowTiles((WindowRef)userData, mc);
   
   return(noErr);
   }
/* ======================================================================== */
/* Map window is closing, release the memory and let the system do the rest */
pascal OSStatus mapWindowClose(EventHandlerCallRef handlerRef, 
                               EventRef event, 
                               void *userData) {
   WindowRef window = (WindowRef)userData;
   struct mapControl *mc = (struct mapControl *)GetWRefCon((WindowRef)userData);
/* ------------------------------------------------------------------------ */
   window = window;
   mc = mc;
   DBG_TRACE((stderr, "Close window %p %p.\n", mc->mapWindow, window));
   if (mc) {
      if (mc->fm.fileStart) {
         munmap(mc->fm.fileStart, mc->fm.fileSize);
         close(mc->fm.fd);
         }
      free(mc);     /* this releases both the control bloch and tile buffer */
      }
   return(eventNotHandledErr); /* let the system do the rest of the closing */
   }
/* ======================================================================== */
/* Mouse click on the map window.                                           */
pascal OSStatus mapLocationClick(EventHandlerCallRef handlerRef, 
                                 EventRef event, 
                                 void *userData) {
   int istat;
   int iwx, iwy;
   int lgl;
   Point clickPix;   
   UInt16 nbtn; 
   struct tsPixel tspx;
   struct tsTilePixel tstp;
   struct tsLatLng tll;                            /* tileset lat/lng */
   struct mapControl *mc = (struct mapControl *)GetWRefCon((WindowRef)userData);
/* ------------------------------------------------------------------------ */
   GetEventParameter(event, kEventParamMouseLocation, typeQDPoint, NULL, sizeof(clickPix), NULL, &clickPix);
   SubPt(mc->winOrig, &clickPix);
   iwx = clickPix.h;
   iwy = clickPix.v;
   GetEventParameter(event, kEventParamMouseButton, typeMouseButton, NULL, sizeof(nbtn), NULL, &nbtn);
   DBG_TRACE((stderr, "Clicked on window %p at %d %d, button:%d.\n", mc->mapWindow, iwx, iwy, nbtn));
   if ((nbtn == 1) || (nbtn == 2)) {     /* will need ground coordinates of clicked loc'n */
      win2ts(&mc->winRect, &mc->crcTile, iwx, iwy, &tspx.pxl_x, &tspx.pxl_y);
      DBG_TRACE((stderr, "Tileset pixel: %d %d.\n", tspx.pxl_x, tspx.pxl_y));
      istat = ts_PixelToTilePixel(mc->tsh, &tspx, &tstp);
      if (istat) {
         DBG_TRACE((stderr, "unexpected location error?"));
         SysBeep(30);
         return(noErr);
         }
      istat = ts_UnMapRect(mc->tsh, mc->tsrp, &tstp, &tll);
      if (istat) {
         DBG_TRACE((stderr, "unexpected location error?"));
         SysBeep(30);
         return(noErr);
         }
      DBG_TRACE((stderr, "Tile ll: %c%d:%06.3f, %c%d:%06.3f\n", TS_SIGN_LAT(tll.lat), TS_INTDEG_LL(tll.lat), TS_DECMIN_LL(tll.lat), TS_SIGN_LNG(tll.lng), TS_INTDEG_LL(tll.lng), TS_DECMIN_LL(tll.lng)));
      }
   if (nbtn == 1) {                   /* recenter the map on click location */
      istat = positionWindow(mc, &tll);
      if (istat == -2) return(noErr);
      if (istat > 0) SysBeep(30);
      else {
/*       updateScrollGeom(hWnd, tvc); (if scroll-bars are imlemented) */
         InvalWindowRect((WindowRef)userData, &mc->winRect);
         }
      }
   else if (nbtn == 2) {                       /* report latitude/longitude */
      lgl = ts_LongitudeGlobal(mc->tsrp->centerLng, tll.lng);
      putAlert(makeMessage("coordinates:\nground:%c%d:%06.4f %c%d:%06.4f\ntileset: %d:%d %d:%d\nwindow:%d %d\n",
       TS_SIGN_LAT(tll.lat), TS_INTDEG_LL(tll.lat), TS_DECMIN_LL(tll.lat),
       TS_SIGN_LNG(lgl), TS_INTDEG_LL(lgl), TS_DECMIN_LL(lgl),
       tstp.tileColumn, tstp.pxl_x, tstp.tileRow, tstp.pxl_y, iwx, iwy), INFO_CONTINUE);
      }
   else if (nbtn == 3) {       /* flip-flop mouse wheel direction indicator */
      mc->mouseWheelDirection = 1 - mc->mouseWheelDirection;
      }

   return(noErr);
   }
/* ======================================================================== */
pascal OSStatus mouseWheelMoved(EventHandlerCallRef handlerRef,
                                EventRef event,
                                void *userData) {
   int istat;
   SInt32 delta;
   EventMouseWheelAxis axis;
   struct tsPixel tspx;
   struct tsTilePixel tstp;
   struct tsLatLng tll;                            /* tileset lat/lng */
   struct mapControl *mc = (struct mapControl *)GetWRefCon((WindowRef)userData);
/* ------------------------------------------------------------------------ */
   GetEventParameter(event, kEventParamMouseWheelAxis, typeMouseWheelAxis, NULL, sizeof(axis), NULL, &axis);
   GetEventParameter(event, kEventParamMouseWheelDelta, typeLongInteger, NULL, sizeof(delta), NULL, &delta);
   if (axis == kEventMouseWheelAxisY) {
      DBG_TRACE((stderr, "Wheel Y Moved, %d.\n", (int)delta)); 
      tspx = mc->crcTile;
      if (mc->mouseWheelDirection) {                           /* East/West */
         tspx.pxl_x += 8 * delta;   
         tspx.pxl_y += 1;
         }  
      else {                                                 /* North/South */ 
         tspx.pxl_x += 1;
         tspx.pxl_y += 8 * delta;  
         }  
      istat = ts_PixelToTilePixel(mc->tsh, &tspx, &tstp);
      if (istat) {
         DBG_TRACE((stderr, "unexpected location error?"));
         SysBeep(30);
         return(noErr);
         }
      istat = ts_UnMapRect(mc->tsh, mc->tsrp, &tstp, &tll);
      if (istat) {
         DBG_TRACE((stderr, "unexpected location error?"));
         SysBeep(30);
         return(noErr);
         }
/*    DBG_TRACE((stderr, "Tile ll: %c%d:%06.3f, %c%d:%06.3f", TS_SIGN_LAT(tll.lat), TS_INTDEG_LL(tll.lat), TS_DECMIN_LL(tll.lat), TS_SIGN_LNG(tll.lng), TS_INTDEG_LL(tll.lng), TS_DECMIN_LL(tll.lng)));*/
      istat = positionWindow(mc, &tll);
/*    DBG_TRACE((stderr, "Position status: %d", istat));*/
      if (istat == -2) return(noErr);
      if (istat > 0) SysBeep(30);
      else {
/*       updateScrollGeom(hWnd, tvc); (if scroll-bars are imlemented) */
         InvalWindowRect((WindowRef)userData, &mc->winRect);
         }
      }
   return(noErr);
   }
/* ======================================================================== */
pascal OSStatus menuUpdate(EventHandlerCallRef handlerRef, 
                           EventRef event, 
                           void *userData) {
   struct mapControl *mc = (struct mapControl *)GetWRefCon((WindowRef)userData);
/* ------------------------------------------------------------------------ */
   mc = mc;
   DBG_TRACE((stderr, "Updating menu, window %p.\n", mc->mapWindow));
   return(noErr);
   }
/* ======================================================================== */
/* menu command for the map window.                                         */
pascal OSStatus mapMenuCommand(EventHandlerCallRef handlerRef, 
                               EventRef event, 
                               void *userData) { 
   OSStatus  err;
   HICommand command;
   struct mapControl *mc = (struct mapControl *)GetWRefCon((WindowRef)userData); 
/* ------------------------------------------------------------------------ */
   err = eventNotHandledErr;
   GetEventParameter(event, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &command); 
   DBG_TRACE((stderr, "Map menu command %c%c%c%c.\n", ((char *)&command.commandID)[0], ((char *)&command.commandID)[1], ((char *)&command.commandID)[2], ((char *)&command.commandID)[3]));
   switch (command.commandID) {
      case 'rprT':
      reportFile(mc);
      err = noErr;
      break;
      case 'vTls':                                    /* to be implemented */
      err = noErr;
      break;
      case 'vGtr':                                    /* to be implemented */
      err = noErr;
      break;
      case 'vLLs':                                    /* to be implemented */
      err = noErr;
      break;
      case 'vTlb':                                    /* to be implemented */
      err = noErr;
      break;
      }
   return(err);
   }
/* ======================================================================== */
