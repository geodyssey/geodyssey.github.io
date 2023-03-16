/* viewtgxm_misc.c */

#include "viewtgxm.h"
/* ======================================================================== */
struct viewtgxmControl *initViewtgxm() {
   struct viewtgxmControl *vtc;   
   SInt16 scrnHRes, scrnVRes;
/* ======================================================================== */
   vtc = (struct viewtgxmControl *)malloc(sizeof(struct viewtgxmControl)); 
   if (vtc == NULL) return(NULL); 
   memset(vtc, '\0', sizeof(struct viewtgxmControl));  
   ScreenRes(&scrnHRes, &scrnVRes);
/* Come on now Apple, it's 2004: "inches"...? */
   vtc->screen_unit_x = 0.0254 / (double)scrnHRes;         /* pixel size... */
   vtc->screen_unit_y = 0.0254 / (double)scrnVRes;          /* ...in meters */
   DBG_TRACE((stderr, "Pixel size: %g x %g\n", vtc->screen_unit_x, vtc->screen_unit_y)); 
   
   getVTC(vtc);                 /* store its pointer, so others can read it */
   return(vtc);
   }
/* ======================================================================== */
struct viewtgxmControl *getVTC(void *p) {
   static struct viewtgxmControl *pp;  
/* ------------------------------------------------------------------------ */
   if (p) pp = p;                           /* it has just been initialized */
   return(pp);  /* anybody can have access to it in (mostly) read-only mode */
   }
/* ======================================================================== */
int ancillaryWindows(IBNibRef nibRef) {
   OSStatus		       err;
   EventTypeSpec      eventSpec;
   EventHandlerUPP    handlerUPP;
   struct viewtgxmControl *vtc = getVTC(NULL);
/* ------------------------------------------------------------------------ */
   DBG_TRACE((stderr, "About to create ancillary windows %p.\n", vtc));

   err = CreateWindowFromNib(nibRef, CFSTR("InfoWindow"), (WindowRef *)&vtc->infoWindow);
   if (err) return(201);
   SetWRefCon(vtc->infoWindow, 0);            /* so we know this is not map */
   DBG_TRACE((stderr, "info window created: %d.\n", (int)vtc->infoWindow));

   err = CreateWindowFromNib(nibRef, CFSTR("AboutWindow"), (WindowRef *)&vtc->aboutWindow);
   if (err) return(202);
   SetWRefCon(vtc->aboutWindow, 0);           /* so we know this is not map */
   setBuildDate(vtc->aboutWindow, __DATE__);  
   DBG_TRACE((stderr, "about window created: %d.\n", (int)vtc->aboutWindow));

/* Both of the above are simple "info-to-look-at" windows; they share... */    
   eventSpec.eventClass = kEventClassWindow;     /* ...close event handlers */
   eventSpec.eventKind = kEventWindowClose;
   handlerUPP = NewEventHandlerUPP(aboutInfoClose);
   InstallWindowEventHandler(vtc->aboutWindow, handlerUPP, 1, &eventSpec, (void *)vtc->aboutWindow, NULL);
   InstallWindowEventHandler(vtc->infoWindow, handlerUPP, 1, &eventSpec, (void *)vtc->infoWindow, NULL);

   eventSpec.eventKind = kEventWindowDrawContent;
   handlerUPP = NewEventHandlerUPP(displayInfo);
   InstallWindowEventHandler(vtc->infoWindow, handlerUPP, 1, &eventSpec, (void *)vtc->infoWindow, NULL);

   return(0);
   }
/* ======================================================================== */
void setBuildDate(WindowRef aboutWindow, char *date) {
   int l;
   char lblTxt[40];
   CFStringRef bdSRef;
   ControlRef bdRef;
   ControlID bdID = {viewtgxmAppSignature, buildDateControlId};
/* ------------------------------------------------------------------------ */
   strncpy(lblTxt, "Program build date: ", 22);
   l = strlen(lblTxt);
   strncpy(lblTxt+l+2, __DATE__, 16);       /* get, transform to yyyy/mm/dd */
   lblTxt[l+0] = lblTxt[l+8]; lblTxt[l+1] = lblTxt[l+9];       /* save year */
   lblTxt[l+8] = lblTxt[l+5]; lblTxt[l+9] = lblTxt[l+6];         /* set day */
   lblTxt[l+5] = lblTxt[l+2]; lblTxt[l+6] = lblTxt[l+3];       /* set month */
   lblTxt[l+2] = lblTxt[l+0]; lblTxt[l+3] = lblTxt[l+1];    /* restore year */
   lblTxt[l+0] = '2'; lblTxt[l+1] = '0';                     /* set century */
   lblTxt[l+10] = '\0';                                     /* just in case */
/* fprintf(stderr, "label:[%s]\n", lblTxt); */ 
   bdSRef = CFStringCreateWithCString(NULL, lblTxt, kCFStringEncodingASCII);
   GetControlByID(aboutWindow, &bdID, &bdRef);
   SetControlData(bdRef, kControlLabelPart, kControlStaticTextCFStringTag, sizeof(bdSRef), &bdSRef);
   return;
   }
/* ======================================================================== */
char *makeMessage(char *fmt, ...) {
   int n;
   va_list p_arg;
   static char msg[LINE_LENGTH + 2];
/* ------------------------------------------------------------------------ */
   va_start(p_arg, fmt);
   n = vsprintf(msg, fmt, p_arg);
   if (n > LINE_LENGTH) ExitToShell();
   va_end(p_arg);
   return(msg);
   }
/* ======================================================================== */
void putAlert(char *errorDesc,                   /* error description strng */
              int iType) {                                /* type indicator */
   CFStringRef     edRef;
   DialogItemIndex itemHit;
   DialogRef       alertDlg;
   AlertType       alertType;
/* ------------------------------------------------------------------------ */
   edRef = CFStringCreateWithCString(NULL, errorDesc, kCFStringEncodingASCII);
   if (iType == INFO_CONTINUE) alertType = kAlertNoteAlert;
   else if (iType == ALERT_CONTINUE) alertType = kAlertCautionAlert;
   else if (iType == ALERT_TERMINATE) alertType = kAlertStopAlert;
   else alertType =  kAlertPlainAlert;
   CreateStandardAlert(alertType, edRef, NULL, NULL, &alertDlg);
   RunStandardAlert(alertDlg, NULL, &itemHit);
   if (iType == ALERT_TERMINATE) ExitToShell();
   return;
   }
/* ======================================================================== */
void closeViewtgxm(struct viewtgxmControl *vtc) {
   free(vtc);
   vtc = NULL;
   DBG_TRACE((stderr, "Closing viewtgxm application.\n"));     
   }
/* ======================================================================== */
void reportFile(struct mapControl *mc) {
   char dscr1[34];
   char dscr2[34];
   char tilesetFileName[FILENAME_MAX + 4];    
   int ln, ls, lgw, lge;
/* ------------------------------------------------------------------------ */
   GetWTitle(mc->mapWindow, tilesetFileName);
   tilesetFileName[FILENAME_MAX + 2] = '\0';
   DBG_TRACE((stderr, "tileset: %s.\n", tilesetFileName));
   
   memcpy(dscr1, mc->dscr_l1, 32);
   memcpy(dscr2, mc->dscr_l2, 32);
   dscr1[32] = dscr2[32] = '\0';
   
   ln = (mc->tsrp->centerLat + mc->tsrp->rangeLat / 2);
   ls = (mc->tsrp->centerLat - mc->tsrp->rangeLat / 2);
   lgw = ts_LongitudeGlobal(mc->tsrp->centerLng, -mc->tsrp->rangeLng / 2);
   lge = ts_LongitudeGlobal(mc->tsrp->centerLng,  mc->tsrp->rangeLng / 2);

   putAlert(makeMessage("%s\n\n%s\n%s\n\ntile columns, rows: %d x %d\ntile width, height: %d x %d\n\nTileset limits:\n               North: %c%d:%06.3f\nWest: %c%d:%06.3f   East: %c%d:%06.3f\n               South: %c%d:%06.3f\n",
    tilesetFileName + 1, dscr1, dscr2,
    mc->tsh->tileColumns, mc->tsh->tileRows,
    mc->tsh->tileWidth, mc->tsh->tileHeight,
   TS_SIGN_LAT(ln),  TS_INTDEG_LL(ln),  TS_DECMIN_LL(ln),
   TS_SIGN_LNG(lgw), TS_INTDEG_LL(lgw), TS_DECMIN_LL(lgw),
   TS_SIGN_LNG(lge), TS_INTDEG_LL(lge), TS_DECMIN_LL(lge),
   TS_SIGN_LAT(ls),  TS_INTDEG_LL(ls),  TS_DECMIN_LL(ls)), INFO_CONTINUE);
 
   return;
   }
/* ======================================================================== */
