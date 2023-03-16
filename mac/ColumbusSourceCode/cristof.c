/* cristof.c - general map window functions */
#include "columbus.h"
/* ======================================================================== */
int newMapWindow(IBNibRef nibRef) {
   OSStatus		       err;
   WindowRef 	       window;
   EventTypeSpec      eventSpec;
   EventHandlerUPP    handlerUPP;
   int                nrCreated;
   struct mapControl *mc;
   static int nWin;
   static SInt16 winPosH = 200;
   static SInt16 winPosV = 160;
/* ------------------------------------------------------------------------ */
   if (nibRef) nrCreated = 0;     /* first map, created at program start-up */
   else {                        /* subsequent, response to new map command */
      if (CreateNibReference(CFSTR("main"), &nibRef)) unexpectedError("Unable to create map window", 1);
      nrCreated = 1;
      }
   err = CreateWindowFromNib(nibRef, CFSTR("MapWindow"), &window);
   if (nrCreated) DisposeNibReference(nibRef);
   if (err) {
      unexpectedError("System: unable to create map window?", 0);
      return(err);
      }
   mc = h1_Malloc(sizeof(struct mapControl));
   if (!mc) {
      DisposeWindow(window);
      unexpectedError("System: memory shortfall?", 0);
      return(1);      
      }
   memset(mc, '\0', sizeof(struct mapControl));
   mc->mapWindow = window;
   SetWRefCon(window, (SInt32)mc);       /* cross-link with Hipparchus data */
/* fprintf(stderr, "Window %d created, memory at %p.\n", (int)window, mc); */

   eventSpec.eventClass = kEventClassWindow;       /* Window event handlers */

/* When map changes size geometry is updated and map is re-drawn. When
   position cganges, the origin is uodated for screen/window transforms. */
   eventSpec.eventKind = kEventWindowBoundsChanged;
   handlerUPP = NewEventHandlerUPP(windowUpdateSizePosition);
   InstallWindowEventHandler(window, handlerUPP, 1, &eventSpec, (void *)window, NULL);

/* When the time comes to draw map, Hipparchus h9 section functions are used. */
   eventSpec.eventKind = kEventWindowDrawContent;
   handlerUPP = NewEventHandlerUPP(drawWindowContent);
   InstallWindowEventHandler(window, handlerUPP, 1, &eventSpec, (void *)window, NULL);

/* Clicking on the map will reposition map center. */
   eventSpec.eventKind = kEventWindowClickContentRgn;
   handlerUPP = NewEventHandlerUPP(mapLocationClick);
   InstallWindowEventHandler(window, handlerUPP, 1, &eventSpec, (void *)window, NULL);

/* When the map window closes, Hipparchus resources must be released. */
   eventSpec.eventKind = kEventWindowClose;
   handlerUPP = NewEventHandlerUPP(mapWindowClose);
   InstallWindowEventHandler(window, handlerUPP, 1, &eventSpec, (void *)window, NULL);

/* Mouse wheel will change scale, providing (almost) continous "zooming". */
   eventSpec.eventClass = kEventClassMouse;          /* Mouse event handler */ 
   eventSpec.eventKind = kEventMouseWheelMoved; 
   handlerUPP = NewEventHandlerUPP(mouseWheelMoved);
   InstallWindowEventHandler(window, handlerUPP, 1, &eventSpec, (void *)window, NULL);

/* Map window specific menu commands: change cartographic projection and scale. */
   eventSpec.eventClass = kEventClassCommand;      /* command event handler */
   eventSpec.eventKind = kEventProcessCommand;
   handlerUPP = NewEventHandlerUPP(mapMenuCommand);
   InstallWindowEventHandler(window, handlerUPP, 1, &eventSpec, (void *)window, NULL);

   MoveWindow(window, winPosH, winPosV, TRUE);
   winPosH += 20; winPosV += 24;
   if (++nWin%10 == 0) {
      winPosH = 200 + 20 * nWin/10; winPosV = 160;
      }

   if (initWindowMap(window)) {
      h1_Free(mc);
      SetWRefCon(window, 0);
      DisposeWindow(window);
      unexpectedError("System: memory shortfall?", 0);
      return(1);
      }

   setMapWindowTitle(window, nWin);
   ShowWindow(window);
   return(0);
   }
/* ======================================================================== */
int ancillaryWindows(IBNibRef nibRef) {
   OSStatus		            err;
   EventTypeSpec      eventSpec;
   EventHandlerUPP    handlerUPP;
   const struct columbusControl *cc = ccDP();
/* ------------------------------------------------------------------------ */
   err = CreateWindowFromNib(nibRef, CFSTR("InfoWindow"), (WindowRef *)&cc->infoWindow);
   if (err) return(201);
   SetWRefCon(cc->infoWindow, 0);             /* so we know this is not map */

   err = CreateWindowFromNib(nibRef, CFSTR("AboutWindow"), (WindowRef *)&cc->aboutWindow);
   if (err) return(202);
   SetWRefCon(cc->aboutWindow, 0);            /* so we know this is not map */
   setBuildDate(cc->aboutWindow, __DATE__); 

/* Both of the above are simple "info-to-look-at" windows; they share... */    
   eventSpec.eventClass = kEventClassWindow;     /* ...close event handlers */
   eventSpec.eventKind = kEventWindowClose;
   handlerUPP = NewEventHandlerUPP(aboutInfoClose);
   InstallWindowEventHandler(cc->aboutWindow, handlerUPP, 1, &eventSpec, (void *)cc->aboutWindow, NULL);
   InstallWindowEventHandler(cc->infoWindow, handlerUPP, 1, &eventSpec, (void *)cc->infoWindow, NULL);

   eventSpec.eventKind = kEventWindowDrawContent;
   handlerUPP = NewEventHandlerUPP(displayInfo);
   InstallWindowEventHandler(cc->infoWindow, handlerUPP, 1, &eventSpec, (void *)cc->infoWindow, NULL);

   return(0);
   }
/* ======================================================================== */
void setBuildDate(WindowRef aboutWindow, char *date) {
   int l;
   char lblTxt[40];
   CFStringRef bdSRef;
   ControlRef bdRef;
   ControlID bdID = {columbusAppSignature, buildDateControlId};
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
void setMapWindowTitle(WindowRef window, int n) {
   char lblTxt[40];
/* ------------------------------------------------------------------------ */
   sprintf(lblTxt, "#Columbus Map [%d]", n);
   *lblTxt = (char)(strlen(lblTxt) - 1);
   SetWTitle(window, lblTxt);
   return;
   }
/* ======================================================================== */
void unexpectedError(char *errorDesc,            /* error description strng */
                     int iTerminate) {          /* indicator, non-0 to quit */
   CFStringRef     edRef;
   DialogItemIndex itemHit;
   DialogRef       alertDlg;
/* ------------------------------------------------------------------------ */
   edRef = CFStringCreateWithCString(NULL, errorDesc, kCFStringEncodingASCII);
   CreateStandardAlert(kAlertStopAlert, edRef, NULL, NULL, &alertDlg);
   RunStandardAlert(alertDlg, NULL, &itemHit);
   if (iTerminate) ExitToShell();
   return;
   }
/* ======================================================================== */
