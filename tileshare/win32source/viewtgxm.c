/* ---------------------------------------------------------- */
/* viewtgxm.c --- Win32 version of a simple tgxm file viewer. */
/* ---------------------------------------------------------- */

/* This C language source code was developed by Geodyssey Limited of
   Calgary, Alberta, Canada. It is provided to application developers
   free of charge and free of copyright restriction in the hope that it
   will provide C language programming examples to be used in development
   of platform-independent geographical applications utilizing a common
   scanned map file format, as described in tileset.h. The content of
   this file may be freely used and/or modified. Attribution of this
   code to Geodyssey Limited at www.geodyssey.com would be appreciated.
 */

/* This program requires no special build setup. The inventory of
   components - other than standard C and Win32 headers and libraries -
   is as follows:

   viewtgxm.c             main Win32 program, this file

   viewtgxm.h             structures and manifest constants used
                          by this program and its satellites.

   viewtgxm_winproc.c     satellite of this program, a win32 event processor,
                          co-resident and included in this program in sourve
                          via an include pre-processor directive.

   viewtgxm_file.c        as above, tileset file open/close functions.

   viewtgxm_graphics.c    as above, graphical functions.

   viewtgxm_util.c        as above, miscellaneous utility functions.

   tileset.h              structures, macros, manifest constants and
                          function protoptypes, definition of the tileset
                          file format and the cross-platform functions
                          used in its processing.

   tileset.c              source code of cross-platform functions used
                          in tileset processing, included in this
                          program in source code form.

   zlib.h                 zlib (lossless compression library) API.

   zlib.lib               zlib link library.

   jpeglib.h              jpeg (lossy compression) library API.

   libjpeg.lib            jpeg library API.

It should be possible to build the program using the gcc compiler and linker
on any Win32 platform using something like the following:

gcc -mwindows -I. -I/jzlibs -oviewtgxm tgxm.c -L../jzlibs -ljpeg -lz

(This is only an example. The compiler is on the path, /jzlibs is the path of
the directory where the zlib and jpeglib headers and link libraries are located,
and all viewtgxm and tileset .c and .h files are in the current directory).
 */

#include "viewtgxm.h"
/* ======================================================================== */
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   PSTR szCmdLine,
                   int iCmdShow) {
   char      *pa;
   HWND       hWnd;
   HMENU      hMenu;
   HMENU      hMenuPopup;
   MSG        msg;
   int        l, istat, wx, wy;
   time_t     timeNow;
   WNDCLASSEX wndclass;
   FILE      *traceFp;
   RECT       cr;
   LOGFONT    lf;
   struct tgxmViewControl *tvc;         /* Overall viewer control structure */
/* ------------------------------------------------------------------------ */
/* MessageBox(NULL, "Hello world...", APP_NAME, MB_OK); */

   tvc = (struct tgxmViewControl *)malloc(sizeof(struct tgxmViewControl));
   if (!tvc) FatalAppExit(0, "Unable to allocate any memory!?");
   memset(tvc, '\0', sizeof(struct tgxmViewControl));

   traceFp = NULL;                     /* assume it'll be dormant */
/* Comment out the following line, and tracing will be dormant... */
/* traceFp = fopen(TRACE_NAME, "at");*/

   debugTrace((char *)traceFp);           /* ...but never comment out this! */

   if (traceFp) {
      timeNow = time(NULL);
      debugTrace("###");
      debugTrace("### %s as-of: %s", APP_NAME, __DATE__ " " __TIME__);
      debugTrace("### start at: %s", ctime(&timeNow));
      }

   tvc->bakCol = RGB(255, 255, 211);
   tvc->bakBrush = CreateSolidBrush(tvc->bakCol);

   wndclass.cbSize        = sizeof(wndclass);
   wndclass.style         = CS_HREDRAW | CS_VREDRAW;
   wndclass.lpfnWndProc   = mapWinProc;
   wndclass.cbClsExtra    = 0;
   wndclass.cbWndExtra    = 0;
   wndclass.hInstance     = hInstance;
   wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
   wndclass.hCursor       = LoadCursor(NULL, IDC_CROSS);
/* wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);*/
   wndclass.hbrBackground = tvc->bakBrush;
   wndclass.lpszMenuName  = APP_NAME;
   wndclass.lpszClassName = APP_NAME;
   wndclass.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
   RegisterClassEx(&wndclass);

   wx = 480;                                        /* if not CW_USEDEFAULT */
   wy = 360;                                        /* if not CW_USEDEFAULT */

   hWnd = CreateWindow(APP_NAME, "tgxm window",
                       WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL,
                       CW_USEDEFAULT, CW_USEDEFAULT,
                       wx, wy,
                       NULL, NULL, hInstance,
                       tvc);

   if (hWnd == NULL) FatalAppExit(0, "Unable to open main window!?");
   debugTrace("map window opened, handle: %d", hWnd);

   hMenu = CreateMenu();     /* window menu will be constructed dynamically */
   hMenuPopup = CreateMenu();
   AppendMenu (hMenuPopup, MF_STRING, IDM_OPEN, "&Open");
   AppendMenu (hMenuPopup, MF_STRING, IDM_RPRT, "&Report");
   AppendMenu (hMenuPopup, MF_SEPARATOR, 0, NULL);
   AppendMenu (hMenuPopup, MF_STRING, IDM_EXIT, "E&xit");
   AppendMenu (hMenu, MF_POPUP, (UINT)hMenuPopup, "&File");

   hMenuPopup = CreateMenu();
   AppendMenu (hMenuPopup, MF_STRING, IDM_TILES,    "&Map Tiles");
   AppendMenu (hMenuPopup, MF_STRING, IDM_GPSTRACE, "GPS &Trace");
   AppendMenu (hMenuPopup, MF_SEPARATOR, 0, NULL);
   AppendMenu (hMenuPopup, MF_STRING, IDM_LLGRID,   "Lat/Long &Grid");
   AppendMenu (hMenuPopup, MF_STRING, IDM_TILEBDY,  "Tile &Boundaries");
   AppendMenu (hMenu, MF_POPUP, (UINT)hMenuPopup,   "&View");

   hMenuPopup = CreateMenu();
   AppendMenu (hMenuPopup, MF_STRING, IDM_GPSON,    "&Read GPS Monitor");
   AppendMenu (hMenuPopup, MF_STRING, IDM_GPSMOVE,  "&Move Map by GPS");
   AppendMenu (hMenu, MF_POPUP, (UINT)hMenuPopup,   "&GPS");

   AppendMenu (hMenu, MF_STRING | MF_HELP, IDM_ABOUT, "&About");
   SetMenu(hWnd, hMenu);
/* debugTrace("map window menu set up: %d", hMenu);*/

   tvc->drawFlags |= DRAW_PIXELS_A;
   CheckMenuItem(hMenu, IDM_TILES, MF_CHECKED);

   tvc->drawFlags |= DRAW_GPSTRACE;
   CheckMenuItem(hMenu, IDM_GPSTRACE, MF_CHECKED);

   if (*szCmdLine) {
      pa = strchr(szCmdLine, ' ');        /* filenames must not have blanks */
      if (pa) {            /* tgxm files are meant to be shared across OSes */
         userError("Error: space character in file path or name [%s]", szCmdLine);
         ExitProcess(1);
         }
      if (*szCmdLine == '"') {
         l = strlen(szCmdLine);
         if (szCmdLine[l-1] == '"') {
            szCmdLine[l-1] = '\0';
            szCmdLine++;
            }
         }
      if (access(szCmdLine, 4)) {
         userError("Unable to locate file [%s]", szCmdLine);
         ExitProcess(1);
         }
      istat = openTgxm(tvc, szCmdLine);
      if (istat) {
         if ((istat == 1) || (istat == 2) || (istat == 3)) userError("Unable to open file [%s], status: %d", szCmdLine, istat);
         else if (istat == 4) userError("File [%s] does not appear to be a tileset file, status: %d", szCmdLine, istat);
         else if (istat == 5) userError("File [%s] appears corrupt, status: %d", szCmdLine, istat);
         else if (istat == 6) userError("File [%s] unable to allocate memory for tile buffer", szCmdLine);
         ExitProcess(1);
         }
      SetWindowText(hWnd, tvc->fn);
      GetClientRect(hWnd, &cr);
      if (resizeWindow(tvc, &cr) == 0) updateScrollGeom(hWnd, tvc);
      debugTrace("command-line tileset opened: %s", tvc->fn);
      }

   tvc->tileBoundaryPen = CreatePen(PS_SOLID, TILE_WIDTH, TILE_COLOR);
   tvc->latLngPen = CreatePen(PS_SOLID, GRID_WIDTH, GRID_COLOR);
   tvc->tracePen = CreatePen(PS_SOLID, TRACE_WIDTH, TRACE_COLOR);

   memset(&lf, '\0', sizeof(LOGFONT));

   lf.lfWeight =        400;
   lf.lfOutPrecision =    3;
   lf.lfClipPrecision =   2;
   lf.lfQuality =         1;
   lf.lfPitchAndFamily = 34;
   strcpy(lf.lfFaceName, "Arial");

   lf.lfHeight =        -16;
   lf.lfWeight =        600;
   tvc->font0  = CreateFontIndirect(&lf);
   lf.lfHeight =        -20;
   lf.lfWeight =        700;
   tvc->font1 = CreateFontIndirect(&lf);
   lf.lfHeight =        -29;
   lf.lfWeight =        800;
   tvc->font2 = CreateFontIndirect(&lf);

   tvc->dispInfo = 1;                           /* display it at least once */
   ShowWindow(hWnd, iCmdShow);
   UpdateWindow(hWnd);

   while(GetMessage(&msg, NULL, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
      }

   if (tvc->fm.lpvFile) closeTgxm(&tvc->fm);
   free(tvc);
   DeleteObject(tvc->font0);
   DeleteObject(tvc->font1);
   DeleteObject(tvc->font2);
   DeleteObject(tvc->tileBoundaryPen);
   DeleteObject(tvc->latLngPen);
   DeleteObject(tvc->tracePen);
   DeleteObject(tvc->bakBrush);
   debugTrace("### end at:   %s\n ", ctime(&timeNow));
   if (traceFp) fclose(traceFp);
   return(msg.wParam);
   }
/* ======================================================================== */
#include "viewtgxm_winproc.c"
#include "viewtgxm_file.c"
#include "viewtgxm_graphics.c"
#include "viewtgxm_util.c"
#include "../tileset.c"
