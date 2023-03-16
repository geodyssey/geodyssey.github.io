/* Didactic code, showing the essential elements of access and display of 
   tilesets on Mac OSX Carbon platform.  Hrvoje Lukatela, a.d. 2004.
    
   This C language source code was developed by Geodyssey Limited of
   Calgary, Alberta, Canada. It is provided to application developers
   free of charge and free of copyright restriction in the hope that it
   will provide C language programming examples to be used in development
   of platform-independent geographical applications utilizing a common
   scanned map file format, as described in tileset.h. The content of
   this file may be freely used and/or modified. Attribution of this
   code to Geodyssey Limited at www.geodyssey.com would be appreciated.
 */   
#include "viewtgxm.h"
/* ======================================================================== */
int main(int argc, char* argv[]) {
   IBNibRef 		nibRef;
/* WindowRef 		window; */
   struct viewtgxmControl *vtc;
   EventHandlerUPP handlerUPP;
   EventTypeSpec   eventSpec;
   EventTargetRef  etRef;
/* ------------------------------------------------------------------------ */   
   DBG_TRACE((stderr, "viewtgxm as of: %s %s - start.\n", __DATE__, __TIME__));
/* DBG_TRACE((stderr, "zlib version %s - %s.\n", ZLIB_VERSION, zlibVersion())); */
/* DBG_TRACE((stderr, "jpeglib version %d.\n", JPEG_LIB_VERSION)); */
/* putAlert(makeMessage("params: %d [%s] [%s]", argc, argv[0], argv[1]), ALERT_CONTINUE); */
    
   vtc = initViewtgxm();
   if (vtc == NULL) putAlert("Unexpected program initialization error (1)", ALERT_TERMINATE);
   if (CreateNibReference(CFSTR("main"), &nibRef)) putAlert("Unexpected program initialization error (2)", ALERT_TERMINATE);
   if (SetMenuBarFromNib(nibRef, CFSTR("MenuBar"))) putAlert("Unexpected program initialization error (3)", ALERT_TERMINATE);
   
   eventSpec.eventClass = kEventClassCommand;
   eventSpec.eventKind = kEventProcessCommand;
   handlerUPP = NewEventHandlerUPP(appMenuCommand);
   etRef = GetApplicationEventTarget();
   if (InstallEventHandler(etRef, handlerUPP, 1, &eventSpec, NULL, NULL)) putAlert("Unexpected program initialization error (3)", ALERT_TERMINATE);

   if (ancillaryWindows(nibRef)) putAlert("Unexpected program initialization error (6)", ALERT_TERMINATE);

   if ((argc > 1) && (*argv[1] != '-')) {
      strncpy(vtc->tilesetFileName, argv[1], FILENAME_MAX); 
      openMapWindow(nibRef);
      }
   DisposeNibReference(nibRef);
   
   RunApplicationEventLoop();
   
   closeViewtgxm(vtc);
   return(noErr);
   }
/* ======================================================================== */
