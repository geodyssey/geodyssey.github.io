/* viewtgxm_file.c */
#include "viewtgxm.h"
/* ======================================================================== */
/* Allow the user to navigate for and select the tileset to be opened in a 
   new map window. If the file has been located store the name in an 
   application-wide buffer and signal sucess by returning 0, -1 if user
   bailed, 1 if an error was encountered.
*/    
int getTilesetFileName() {
   OSStatus       err;
   struct NavDialogCreationOptions gfd;
   struct viewtgxmControl *vtc = getVTC(NULL);
   NavDialogRef dlgRef;
/* ------------------------------------------------------------------------ */
   DBG_TRACE((stderr, "About to start file navigation.\n")); 
   *vtc->tilesetFileName = '\0';
   err = NavGetDefaultDialogCreationOptions(&gfd);
   if (err) {
      putAlert("Ufile dialog initialization error (1).", ALERT_CONTINUE);
      return(1);
      }
   gfd.modality = kWindowModalityAppModal;
   vtc->fnep = NewNavEventUPP(fileNavCallBack);
   err = NavCreateGetFileDialog(&gfd, NULL, vtc->fnep, NULL, NULL, NULL, &dlgRef);
   if (err) {
      putAlert("Unable to start file navigation dialog.", ALERT_CONTINUE);
      return(1);
      }
   err = NavDialogRun(dlgRef);
   if (err != noErr) {
      NavDialogDispose(dlgRef);
      DisposeNavEventUPP(vtc->fnep);
      putAlert("Error in closing file navigation dialog.", ALERT_CONTINUE);
      return(1);
      }   
   DBG_TRACE((stderr, "File selected: %s.\n", *vtc->tilesetFileName ? vtc->tilesetFileName : "NONE"));       
   if (*vtc->tilesetFileName) return(0);
   return(-1);                                   
   }
/* ======================================================================== */
/* The invoker must ensure a reasonably good file name is in the vtc->...   */
int openMapWindow(IBNibRef nibRef) {                             /* or null */
   char *pa;
   OSStatus err;
   WindowRef window;
   EventTypeSpec eventSpec;   
   EventHandlerUPP handlerUPP;
   struct viewtgxmControl *vtc = getVTC(NULL);
   struct fileMapping fm;
   struct mapControl *mc;
   struct tsHdr *tsh;
   int lh, istat, nrc, i, n, m;
   struct Rect newRect;
   int sLineSize, tileSize;
   char pTilesetFileName[FILENAME_MAX + 4];
   static int myEndian = 16777216 * 't' + 65536 * 'g' + 256 * 'x' + 'm';
   static int nWin;
   static SInt16 winPosH = INITIAL_MAP_POSITION_X;
   static SInt16 winPosV = INITIAL_MAP_POSITION_Y;
/* ------------------------------------------------------------------------ */
   window = 0;
   DBG_TRACE((stderr, "Opening map window, tileset: %s.\n", vtc->tilesetFileName));
   
/* *** phase 1: memory-map tilest and check file for reasonableness *** */   
/* Tileset is primarily a cross-platform object, and only secondary a file on 
   a particular OS; thus there are rules that govern the format of its name 
   beyond what a particular platform might or might not mandate. */
   if (access(vtc->tilesetFileName, R_OK)) {
      putAlert(makeMessage("Unable to access file: [%s].", vtc->tilesetFileName), ALERT_CONTINUE);
      return(1);
      }
   pa = strrchr(vtc->tilesetFileName, '.');
   if (!pa || (strcasecmp(pa, TS_FILE_SUFFIX))) {
      putAlert(makeMessage("Invalid file extension: [%s].", vtc->tilesetFileName), ALERT_CONTINUE);
      return(2);
      }
   memset(&fm, '\0', sizeof(struct fileMapping));
   fm.fd = open(vtc->tilesetFileName, O_RDONLY);
   if (fm.fd == -1) {
      putAlert(makeMessage("Unable to open file: [%s].", vtc->tilesetFileName), ALERT_CONTINUE);
      return(3);
      }
   fm.fileSize = lseek(fm.fd, 0, SEEK_END);                 /* find size... */
   lseek(fm.fd, 0, SEEK_SET);                             /* ...and rewind. */

   fm.fileStart = (char *)mmap(0, fm.fileSize, PROT_READ, MAP_SHARED, fm.fd, 0); 
   if (fm.fileStart == MAP_FAILED) {
      close(fm.fd);
      putAlert(makeMessage("Unable to map file: [%s].", vtc->tilesetFileName), ALERT_CONTINUE);
      return(4);
      }
   
   if ((memcmp(fm.fileStart, TS_FILE_BIG_ENDIAN, 4)) && (memcmp(fm.fileStart, TS_FILE_LITTLE_ENDIAN, 4))) {
      munmap(fm.fileStart, fm.fileSize);
      close(fm.fd);
      putAlert(makeMessage("Tileset file [%s], unexpected file-stamp.", vtc->tilesetFileName), ALERT_CONTINUE);
      return(5);
      }

   tsh = (struct tsHdr *)fm.fileStart;
/* Locate the header mirror of the same endianness as the platform run on.  */
   if (tsh->fileStamp != myEndian) {                     /* the 'wrong one' */
      lh = sizeof(struct tsHdr)                              /* file header */
         + sizeof(struct tsRect)                       /* projection header */
         + (TS_I4_RVRS(tsh->tileRows) + 1) * 4                 /* row table */
         + TS_I4_RVRS(tsh->tileColumns) * TS_I4_RVRS(tsh->tileRows) * sizeof(struct tsTile1);  /* tile table */
      tsh = (struct tsHdr *)((char *)fm.fileStart + lh); /* point to mirror */
      }

/* Confirm that the second header block is as expected and that the tileset
   is of the simple type this program knows how to process: */
   istat = 0;
   if (tsh->fileStamp != myEndian) {
      putAlert(makeMessage("Tileset file [%s], unexpected endian mirror?", vtc->tilesetFileName), ALERT_CONTINUE);
      istat = 1;
      } 
   else if (tsh->projection != TS_PROJ_RECTANGULAR) {
      putAlert(makeMessage("Tileset file [%s], unexpected projection indicator?", vtc->tilesetFileName), ALERT_CONTINUE);
      istat = 2;
      } 
   else if (tsh->colorDepth != TS_COLOR_RGB) {
      putAlert(makeMessage("Tileset file [%s], unexpected color depth?", vtc->tilesetFileName), ALERT_CONTINUE);
      istat = 3;
      } 
   else if (tsh->imageType != TS_IMAGE_ZLIB) {
      putAlert(makeMessage("Tileset file [%s], unexpected image type?", vtc->tilesetFileName), ALERT_CONTINUE);
      istat = 4;
      } 
    
   if (istat) {           
      munmap(fm.fileStart, fm.fileSize);
      close(fm.fd);
      return(5 + istat);
      }
   
/* We need to know the size of the header block in any case */
   lh = sizeof(struct tsHdr)                                 /* file header */
      + sizeof(struct tsRect)                          /* projection header */
      + (tsh->tileRows + 1) * 4                                /* row table */
      + tsh->tileColumns * tsh->tileRows * sizeof(struct tsTile1); /* tile table */
   DBG_TRACE((stderr, "File OK, mapped at: %p; header at %p.\n", fm.fileStart, tsh));
   
/* *** phase 2: allocate memory for window tileset structure and tile buffer *** */
/* tsh was set to right header, colorDepth was checked. */
   if (tsh->colorDepth == TS_COLOR_BLACKWHITE) sLineSize = tsh->tileWidth / 8;
   else if (tsh->colorDepth == TS_COLOR_PALETTE256) sLineSize = tsh->tileWidth;
   else if (tsh->colorDepth == TS_COLOR_RGB) tileSize = sLineSize = tsh->tileWidth * 3;
   else sLineSize = 0;                      /* colorDeptd was checked above */
   tileSize = sLineSize * tsh->tileHeight;                 /* bytes in tile */
   n = sizeof(struct mapControl);   /* used later on to set buffer pointer! */
   while (n%8) n++;                             /* ensure buffer is aligned */
   m = TILES_IN_MEM * tileSize + sLineSize;                  /* buffer size */
   mc = malloc(n + m);                       /* allocate structure + bubber */
   if (mc == NULL) {
      munmap(fm.fileStart, fm.fileSize);
      close(fm.fd);
      putAlert("System: window memory shortfall?", ALERT_CONTINUE);
      return(10);
      }
   memset(mc, '\0', sizeof(struct mapControl));
   DBG_TRACE((stderr, "Window memory of %d + %d, allocated at %p.\n", n, m, mc));
      
/* *** phase 3: create window *** */
   if (nibRef) nrc = 0;   
   else {
      err = CreateNibReference(CFSTR("main"), &nibRef);
      if (err != noErr) {
         munmap(fm.fileStart, fm.fileSize);
         close(fm.fd);
         free(mc);
         putAlert("Unexpected window creation error (1).", ALERT_CONTINUE);
         return(11);
         }
      nrc = 1;
      }
   err = CreateWindowFromNib(nibRef, CFSTR("MapWindow"), &window);
   if (nrc) DisposeNibReference(nibRef);
   if (err != noErr) {
      munmap(fm.fileStart, fm.fileSize);
      close(fm.fd);
      free(mc);
      putAlert("Unexpected window creation error (2).", ALERT_CONTINUE);
      return(10);
      }
   SetWRefCon(window, (SInt32)mc);      /* cross-link with Hipparchus data */
/* DBG_TRACE((stderr, "Window %p created\n", window)); */
    
/* *** phase 4: Load/update window control block, set event handlers:  *** */
   mc->vtc = vtc;                 /* point back to application-wide values */
   mc->mapWindow = window;   /* nothing can go wrong now, window will open */             
   mc->fm = fm;
   mc->tsh = tsh; 
   mc->tsrp = (struct tsRect *)(tsh + 1);   
   mc->tstt1 = (struct tsTile1 *)((char *)(mc->tsrp + 1) + (tsh->tileRows + 1) * sizeof(int));
   mc->tstt2 = NULL;      /* as an alternative to tstt1, to be implemented */
   mc->tspltt = NULL;                                 /* to be implemented */
   mc->tstrsp = NULL;                                 /* to be implemented */
   mc->dscr_l1 = (char *)fm.fileStart + lh + lh;
   mc->dscr_l2 = mc->dscr_l1 + 32;
   mc->sLineSize = sLineSize;
   mc->tileSize = tileSize;
   mc->drawFlags = DRAW_PIXELS_A;  
   mc->mouseWheelDirection = 0;
   SetRect(&mc->winRect, -1, -1, -1, -1);                 /* "udefined", to force first size */
/* window center in tileset coordinates is the center pixel of the tileset. */
   mc->crcTile.pxl_x = TS_PX_WIDTH(tsh) / 2;
   mc->crcTile.pxl_y = TS_PX_HEIGHT(tsh) / 2;
   mc->tileLoadCounter = 1;                             /* reset load stamp */
   for (i = 0; i < TILES_IN_MEM; i++) mc->tile[i].loadCounter = 0; /* all slots are empty */
   mc->pixelBuffer = (char *)mc + n;               /* tiles saved in memory */
   
/* When map changes size geometry is updated and map is re-drawn. When
   position cganges, the origin is podated for screen/window transforms. */
   eventSpec.eventClass = kEventClassWindow;        
   eventSpec.eventKind = kEventWindowBoundsChanged;
   handlerUPP = NewEventHandlerUPP(windowSizeMove);
   InstallWindowEventHandler(window, handlerUPP, 1, &eventSpec, (void *)window, NULL);

/* When the time comes to draw map, tiles are moved from buffer to screen. */
   eventSpec.eventClass = kEventClassWindow;        
   eventSpec.eventKind = kEventWindowDrawContent;
   handlerUPP = NewEventHandlerUPP(drawWindowContent);
   InstallWindowEventHandler(window, handlerUPP, 1, &eventSpec, (void *)window, NULL);

/* When the map window closes, resources must be released. */
   eventSpec.eventClass = kEventClassWindow;        
   eventSpec.eventKind = kEventWindowClose;
   handlerUPP = NewEventHandlerUPP(mapWindowClose);
   InstallWindowEventHandler(window, handlerUPP, 1, &eventSpec, (void *)window, NULL);

/* Clicking on the map will reposition map center. */
   eventSpec.eventClass = kEventClassWindow;        
   eventSpec.eventKind = kEventWindowClickContentRgn;
   handlerUPP = NewEventHandlerUPP(mapLocationClick);
   InstallWindowEventHandler(window, handlerUPP, 1, &eventSpec, (void *)window, NULL);

/* Turning the mouse wheel moves the map North/South or East/West. */
   eventSpec.eventClass = kEventClassMouse;
   eventSpec.eventKind = kEventMouseWheelMoved;
   handlerUPP = NewEventHandlerUPP(mouseWheelMoved);
   InstallWindowEventHandler(window, handlerUPP, 1, &eventSpec, (void *)window, NULL);

/* Map window specific menu commands: view inventory selection and GPS. */
   eventSpec.eventClass = kEventClassCommand;     /* command event handler */
   eventSpec.eventKind = kEventProcessCommand;
   handlerUPP = NewEventHandlerUPP(mapMenuCommand);
   InstallWindowEventHandler(window, handlerUPP, 1, &eventSpec, (void *)window, NULL);
   
/* Ensure new map window does not obstruct a is previously opened one... */
   MoveWindow(window, winPosH, winPosV, TRUE);
   winPosH += 20; winPosV += 24;
   if (++nWin%10 == 0) {
      winPosH = INITIAL_MAP_POSITION_X + 20 * nWin/10; winPosV = INITIAL_MAP_POSITION_Y;
      }

   c2pstrcpy(pTilesetFileName, vtc->tilesetFileName); /* Must be pascal string! */
   SetWTitle(window, pTilesetFileName);                 /* give it the name */
   GetPortBounds(GetWindowPort(window), &newRect);   /* initialize geometry */
   DBG_TRACE((stderr, "Start initial geometry.\n")); 
   resizeWindow(mc, &newRect);
   
   DBG_TRACE((stderr, "---\n"));
   DBG_TRACE((stderr, "Tileset: %s.\n", vtc->tilesetFileName)); 
   DBG_TRACE((stderr, "Tile width %d height %d.\n", mc->tsh->tileWidth, mc->tsh->tileHeight)); 
   DBG_TRACE((stderr, "Tile columns %d, rows %d.\n", mc->tsh->tileColumns, mc->tsh->tileRows));
   DBG_TRACE((stderr, "Tileset size width %d hight %d.\n", mc->tsh->tileWidth * mc->tsh->tileColumns, mc->tsh->tileHeight * mc->tsh->tileRows));
   DBG_TRACE((stderr, "---\n"));
   DBG_TRACE((stderr, "Window: %p\n", window));
   DBG_TRACE((stderr, "left %d right %d   top %d bottom %d.\n", mc->winRect.left, mc->winRect.right, mc->winRect.top, mc->winRect.bottom));
   DBG_TRACE((stderr, "origin x %d  y %d.\n", mc->winOrig.h, mc->winOrig.v));
   DBG_TRACE((stderr, "center in tilset coords: %d, %d.\n", mc->crcTile.pxl_x, mc->crcTile.pxl_y)); 
   DBG_TRACE((stderr, "---\n"));
  
   ShowWindow(window);
   return(0);
   }
/* ======================================================================== */
/* Load tile into the in-memory buffer, return its index,
    -1 if un-compression error was encountered.
 */
int loadTile(struct mapControl *mc,
             int i, int j) {                        /* tile id: column, row */
   int istat;
   int n, nn, m;
   unsigned minCounter;
   unsigned char *pcp;              /* pointer to source: compressed pixels */
   unsigned char *pup;            /* pointer to target: uncompressed pixels */
   unsigned long lcmpr;                              /* so defined for zlib */
/* ------------------------------------------------------------------------ */
/* Get the slot in the buffer where the uncompressed pixels will be placed. */
/* DBG_TRACE((stderr, "Loading tile: %d %d.\n", i, j)); */
   minCounter = LOAD_COUNTER_MAX;
   n = -1;
   for (nn = 0; nn < TILES_IN_MEM; nn++) {    /* find oldest in-memory tile */
      if (mc->tile[nn].loadCounter < minCounter) {
         minCounter = mc->tile[nn].loadCounter;
         n = nn;
         }
      }
   if (n == -1) return(n);

/* Get the file position of the compressed tile pixels. */
   m = j * mc->tsh->tileColumns + i;
   if (m >= mc->tsh->tileColumns * mc->tsh->tileRows) {
      putAlert(makeMessage("Unexpected addressing error, tile %d, %d", i, j), ALERT_CONTINUE); 
      return(-1);
      }

/* get the tile-in-memory address to where to the tile is to be decompressed */
   pup = mc->pixelBuffer + n * mc->tileSize;

   if ((mc->tstt1[m].dataStart == (int)0xffffffff)          /* empty tile? */
    && (mc->tstt1[m].dataLength == 0)) {      /* (there be dragons there?) */
      memset(pup, '\xff', mc->tileSize);                     /* whitespace */
      mc->tile[n].loadCounter = mc->tileLoadCounter++;
      mc->tile[n].column = i;
      mc->tile[n].row = j;
      return(n);
      }

/* get the mapped file address from which the compressed tile is obtained.  */
   pcp = (unsigned char *)mc->fm.fileStart + mc->tstt1[m].dataStart;   /* from where */

/* For simplicity, uncompress is used; it might be better to use lower
   level zlib functions - see zlib documentation for details.
   (Will uncompress allocate any transient workspace? - It shouldn't!)
 */
   lcmpr = mc->tileSize;
   istat = uncompress(pup, &lcmpr, pcp, mc->tstt1[m].dataLength);
   if (istat != Z_OK) {
      putAlert(makeMessage("Unexpected uncompression error - corrupt data? tile: (%d, %d)", i, j), ALERT_CONTINUE); 
      mc->tile[n].loadCounter = 0;         /* did we damage existing data? */
      return(-1);
      }
   if ((int)lcmpr != mc->tileSize) {
      putAlert(makeMessage("Unexpected uncompression size error (%d, %d)", lcmpr, mc->tileSize), ALERT_CONTINUE); 
      mc->tile[n].loadCounter = 0;         /* did we damage existing data? */
      return(-1);
      }
 
/* debugTrace("Tile: %2d %2d, f.o.: %6d size: %6d to slot: %3d at %p", i, j, mc->tstt1[m].dataStart, mc->tstt1[m].dataLength, n, pup);*/
   mc->tile[n].loadCounter = mc->tileLoadCounter++;
   mc->tile[n].column = i;
   mc->tile[n].row = j;

   return(n);
   }
/* ======================================================================== */