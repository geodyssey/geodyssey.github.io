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
/* Invoker must ensure that no tileset file is currently mapped ("opened").
   The function returns:
         0: file open ok
         1: file open failed
         2: memory mapping object creation failed
         3: memory mapping of file failed
         4: unexpected filestamp
         5: unable to locate correct endian version
         6: unable to allocate memory for tile buffer
 */

int openTgxm(struct tgxmViewControl *tvc,
             const char *fileNm) {
   int n, lh;
   char *pa;
   struct fileMapping *fm;
   static int myEndian = 16777216 * 't' + 65536 * 'g' + 256 * 'x' + 'm';
/* ------------------------------------------------------------------------ */
/* debugTrace("opening tile file: %s", fileNm);*/
   fm = &(tvc->fm);

   memset(fm, '\0', sizeof(struct fileMapping));
   fm->hFile = CreateFile(fileNm, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
   if (fm->hFile == INVALID_HANDLE_VALUE) {       /* win32 file open failed */
      return(1);
      }
   fm->hFileMap = CreateFileMapping(fm->hFile, NULL, PAGE_READONLY, 0, 0, NULL);
   if (fm->hFileMap == NULL) {     /* Memory-Mapping of object file failed. */
      CloseHandle(fm->hFile);
      fm->hFile = INVALID_HANDLE_VALUE;
      return(2);
      }
   fm->lpvFile = MapViewOfFile(fm->hFileMap, FILE_MAP_READ, 0, 0, 0);
   if (fm->lpvFile == NULL) {                   /* Map view of file failed. */
      CloseHandle(fm->hFileMap);
      fm->hFileMap = NULL;
      CloseHandle(fm->hFile);
      fm->hFile = INVALID_HANDLE_VALUE;
      return(3);
      }
   fm->fileSize = GetFileSize(fm->hFile, NULL);

/* Test the filestamp: */
   if ((memcmp(fm->lpvFile, TS_FILE_BIG_ENDIAN, 4)) && (memcmp(fm->lpvFile, TS_FILE_LITTLE_ENDIAN, 4))) {
      UnmapViewOfFile(fm->lpvFile);
      CloseHandle(fm->hFileMap);
      CloseHandle(fm->hFile);
      fm->lpvFile = NULL;
      return(4);                                 /* unrecognized file stamp */
      }

   tvc->tsh = (struct tsHdr *)fm->lpvFile;                   /* preliminary */

/* Locate the header mirror of the same endianness as the platform run on.  */
   if (tvc->tsh->fileStamp != myEndian) {                /* the 'wrong one' */
      lh = sizeof(struct tsHdr)                              /* file header */
         + sizeof(struct tsRect)                       /* projection header */
         + (TS_I4_RVRS(tvc->tsh->tileRows) + 1) * 4            /* row table */
         + TS_I4_RVRS(tvc->tsh->tileColumns) * TS_I4_RVRS(tvc->tsh->tileRows) * sizeof(struct tsTile1);  /* tile table */
      tvc->tsh = (struct tsHdr *)((char *)fm->lpvFile + lh); /* point to mirror */
      }

/* Confirm that the second header block is as expected. */
   if (tvc->tsh->fileStamp != myEndian) {
      UnmapViewOfFile(fm->lpvFile);
      CloseHandle(fm->hFileMap);
      CloseHandle(fm->hFile);
      fm->lpvFile = NULL;
      return(5);                                  /* still the 'wrong one'? */
      }

/* We need to know the size of the header block in any case */
   lh = sizeof(struct tsHdr)                                 /* file header */
      + sizeof(struct tsRect)                          /* projection header */
      + (tvc->tsh->tileRows + 1) * 4                           /* row table */
      + tvc->tsh->tileColumns * tvc->tsh->tileRows * sizeof(struct tsTile1);     /* tile table */

   tvc->tsrp = (struct tsRect *)(tvc->tsh + 1);

   tvc->tstt1 = (struct tsTile1 *)((char *)(tvc->tsrp + 1) + (tvc->tsh->tileRows + 1) * sizeof(int));
   tvc->tstt2 = NULL;      /* as an alternative to tstt1, to be implemented */

   pa = (char *)fm->lpvFile + lh + lh;
   tvc->tspltt = NULL;                                 /* to be implemented */
   tvc->tstrsp = NULL;                                 /* to be implemented */
   tvc->dscr_l1 = pa;
   tvc->dscr_l2 = tvc->dscr_l1 + 32;

/* Find the amount of space required and allocate pixel buffer */

   if (tvc->tsh->colorDepth == TS_COLOR_BLACKWHITE) tvc->sLineSize = tvc->tsh->tileWidth / 8;
   else if (tvc->tsh->colorDepth == TS_COLOR_PALETTE256) tvc->sLineSize = tvc->tsh->tileWidth;
   else if (tvc->tsh->colorDepth == TS_COLOR_RGB) tvc->tileSize = tvc->sLineSize = tvc->tsh->tileWidth * 3;
   else {
      UnmapViewOfFile(fm->lpvFile);
      CloseHandle(fm->hFileMap);
      CloseHandle(fm->hFile);
      fm->lpvFile = NULL;
      return(5);                                           /* corrupt file? */
      }
   tvc->tileSize = tvc->sLineSize * tvc->tsh->tileHeight;  /* bytes in tile */

/* note: one extra line is allocated to facilitate line order swapping */
   fm->pixelBuffer = (unsigned char *)malloc(TILES_IN_MEM * tvc->tileSize + tvc->sLineSize);
   if (fm->pixelBuffer == NULL) {
      UnmapViewOfFile(fm->lpvFile);
      CloseHandle(fm->hFileMap);
      CloseHandle(fm->hFile);
      fm->lpvFile = NULL;
      return(6);                              /* no memory for pixel buffer */
      }

   for (n = 0; n < TILES_IN_MEM; n++) {  /* signal that all slots are empty */
      tvc->tile[n].loadCounter = 0;
      }
   tvc->tileLoadCounter = 1;                            /* reset load stamp */

/* window center in tileset coordinates is the center pixel of the tileset. */
   tvc->mpw.crcTile.pxl_x = TS_PX_WIDTH(tvc->tsh) / 2;
   tvc->mpw.crcTile.pxl_y = TS_PX_HEIGHT(tvc->tsh) / 2;

/* For convenience, low and high pixel limits are copied to window block: */
   tvc->mpw.lowLatY = tvc->tsrp->lowLatY;
   tvc->mpw.highLatY = tvc->tsrp->highLatY;

/* Initialize the Win32 specific structure that describes an un-compressed
   tile as platform (MS Windows) specific bitmap (which it is of course not).
   For now, we only deal with tilesets that are RGB color.
 */
   tvc->bm.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   tvc->bm.bmiHeader.biWidth = tvc->tsh->tileWidth;
   tvc->bm.bmiHeader.biHeight = tvc->tsh->tileHeight;
   tvc->bm.bmiHeader.biPlanes = 1;
   tvc->bm.bmiHeader.biBitCount = 24;
   tvc->bm.bmiHeader.biCompression = 0;
   tvc->bm.bmiHeader.biSizeImage = 0;
   tvc->bm.bmiHeader.biXPelsPerMeter = 0;
   tvc->bm.bmiHeader.biYPelsPerMeter = 0;
   tvc->bm.bmiHeader.biClrUsed = 0;
   tvc->bm.bmiHeader.biClrImportant = 0;

/* Finalize and return */
   strncpy(tvc->fn, fileNm, FILENAME_MAX);

   return(0);
   }
/* ======================================================================== */
void closeTgxm(struct fileMapping *fm) {
   if (fm->lpvFile) UnmapViewOfFile(fm->lpvFile);
   if (fm->hFileMap) CloseHandle(fm->hFileMap);
   if (fm->hFile != INVALID_HANDLE_VALUE) CloseHandle(fm->hFile);
   if (fm->pixelBuffer) free(fm->pixelBuffer);
   memset(fm, '\0', sizeof(struct fileMapping));
   return;
   }
/* ======================================================================== */
/* Load tile into the in-memory buffer, return its index,
    -1 if un-compression error was encountered.
 */
int loadTile(struct tgxmViewControl *tvc,
             int i, int j) {                        /* tile id: column, row */
   int istat;
   int n, nn, m;
   unsigned minCounter;
   unsigned char *pcp;              /* pointer to source: compressed pixels */
   unsigned char *pup;            /* pointer to target: uncompressed pixels */
   unsigned long lcmpr;                              /* so defined for zlib */
   unsigned char *pl_a;                          /* first scan line pointer */
   unsigned char *pl_z;                           /* last scan line pointer */
   unsigned char *sline;                   /* where whole line can be saxed */
   unsigned char *px;                              /* pixel traverse poiner */
   unsigned char *pz;                               /* pixel traverse limit */
/* ------------------------------------------------------------------------ */
/* Get the slot in the buffer where the uncompressed pixels will be placed. */
   minCounter = LOAD_COUNTER_MAX;
   n = -1;
   for (nn = 0; nn < TILES_IN_MEM; nn++) {    /* find oldest in-memory tile */
      if (tvc->tile[nn].loadCounter < minCounter) {
         minCounter = tvc->tile[nn].loadCounter;
         n = nn;
         }
      }
   if (n == -1) return(n);

/* Get the file position of the compressed tile pixels. */
   m = j * tvc->tsh->tileColumns + i;
   if (m >= tvc->tsh->tileColumns * tvc->tsh->tileRows) {
      userError("Unexpected addressing error, tile %d, %d", i, j);
      return(-1);
      }

/* get the tile-in-memory address to where the tile is to be decompressed */
   pup = tvc->fm.pixelBuffer + n * tvc->tileSize;

   if ((tvc->tstt1[m].dataStart == (int)0xffffffff)          /* empty tile? */
    && (tvc->tstt1[m].dataLength == 0)) {      /* (there be dragons there?) */
      memset(pup, '\xff', tvc->tileSize);                     /* whitespace */
      tvc->tile[n].loadCounter = tvc->tileLoadCounter++;
      tvc->tile[n].column = i;
      tvc->tile[n].row = j;
      return(n);
      }

/* get the mapped file address from which the compressed tile is obtained.  */
   pcp = (unsigned char *)tvc->fm.lpvFile + tvc->tstt1[m].dataStart; /* from where */

/* For simplicity, uncompress is used; it might be better to use lower
   level zlib functions - see zlib documentation for details.
   (Will uncompress allocate any transient workspace? - It shouldn't!)
 */
   lcmpr = tvc->tileSize;
   istat = uncompress(pup, &lcmpr, pcp, tvc->tstt1[m].dataLength);
   if (istat != Z_OK) {
      userError("Unexpected uncompression error - corrupt data? tile: (%d, %d)", i, j);
      tvc->tile[n].loadCounter = 0;         /* did we damage existing data? */
      return(-1);
      }
   if ((int)lcmpr != tvc->tileSize) {
      userError("Unexpected uncompression size error (%d, %d)", lcmpr, tvc->tileSize);
      tvc->tile[n].loadCounter = 0;         /* did we damage existing data? */
      return(-1);
      }

/* On Win32, DIB's are the canonical objects to blit on the screen. But they
   have both the RGB order and line order opposite from the vast majority
   of graphical platforms and file formats. (line count is known to be even).
 */
   sline = tvc->fm.pixelBuffer + TILES_IN_MEM * tvc->tileSize; /* saved lne */

   for (pl_a = tvc->fm.pixelBuffer + n * tvc->tileSize,       /* first line */
        pl_z = pl_a + tvc->tileSize - tvc->sLineSize;          /* last line */
        pl_a < pl_z;
        pl_a += tvc->sLineSize,
        pl_z -= tvc->sLineSize) {
      memcpy(sline, pl_a, tvc->sLineSize);                 /* save low line */
      if (tvc->tsh->colorDepth == TS_COLOR_RGB) {      /* swap red and blue */
         px = sline;                                       /* in saved line */
         pz = px + tvc->sLineSize;
         while (px < pz) {
            TS_SWAP_RB(px);
            px += 3;
            }
         px = pl_z;                                     /* and in high line */
         pz = px + tvc->sLineSize;
         while (px < pz) {
            TS_SWAP_RB(px);
            px += 3;
            }
         }                                     /* RGB swapped in both lines */
      memcpy(pl_a, pl_z, tvc->sLineSize);               /* copy high to low */
      memcpy(pl_z, sline, tvc->sLineSize);             /* and saved to high */
      }

/* debugTrace("Tile: %2d %2d, f.o.: %6d size: %6d to slot: %3d at %p", i, j, tvc->tstt1[m].dataStart, tvc->tstt1[m].dataLength, n, pup);*/
   tvc->tile[n].loadCounter = tvc->tileLoadCounter++;
   tvc->tile[n].column = i;
   tvc->tile[n].row = j;

   return(n);
   }
/* ======================================================================== */
/* Inverted return code follows GetOpenFileName() usage. */
int selectTileset(HWND hWnd,
                  char *tsFn) {
   int istat;
   char *pa;
   OPENFILENAME ofn;
   static char fileFilter[] = "Tileset files (*.tgxm)\0*.tgxm\0All Files (*.*)\0*.*\0\0";
   static char dirBuffer[FILENAME_MAX + 2];                  /* directories */
/* -------------------------------------------------------------------------*/
   memset(&ofn, '\0', sizeof(OPENFILENAME));
   *tsFn = '\0';

   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.hwndOwner = hWnd;
   ofn.hInstance = NULL;
   ofn.lpstrFilter = fileFilter;
   ofn.lpstrCustomFilter = NULL;
   ofn.nMaxCustFilter = 0;
   ofn.nFilterIndex = 1;
   ofn.lpstrFile = tsFn;
   ofn.nMaxFile = FILENAME_MAX;
   ofn.lpstrFileTitle = NULL;
   ofn.nMaxFileTitle = FILENAME_MAX;
   ofn.lpstrInitialDir = dirBuffer;
   ofn.lpstrTitle = "Select Tileset file";
   ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;
   ofn.nFileOffset = 0;
   ofn.nFileExtension = 0;
   ofn.lpstrDefExt = "tgxm";
   ofn.lCustData = 0;
   ofn.lpfnHook = NULL;
   ofn.lpTemplateName = NULL;

   istat = GetOpenFileName(&ofn);
   if (istat == 0) return(0);

   pa = strchr(tsFn, ' ');           /* tgxm filenames must not have blanks */
   while (pa) {            /* tgxm files are meant to be shared across OSes */
      userError("Error: space character in file path or name [%s]", tsFn);
      istat = GetOpenFileName(&ofn);
      if (istat == 0) return(0);
      pa = strchr(tsFn, ' ');
      }

   strncpy(dirBuffer, tsFn, ofn.nFileOffset);
   *(dirBuffer + ofn.nFileOffset) = '\0';

   return(1);
   }
/* ======================================================================== */
