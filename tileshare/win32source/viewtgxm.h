/* --------------------------------------------------------- */
/* viewtgxm.h : defines used by the Win32 tgxm sample viewer */
/* --------------------------------------------------------- */

/* This C language source code was developed by Geodyssey Limited of
   Calgary, Alberta, Canada. It is provided to application developers
   free of charge and free of copyright restriction with the hope that it
   will provide C language programming examples to be used in development
   of platform-independent geographical applications utilizing a common
   scanned map file format, as described in tileset.h. The content of this
   file may be freely used and/or modified. Attribution of this code to
   Geodyssey Limited at www.geodyssey.com would be appreciated.
 */

#ifndef VIEWTGXM_H
#define VIEWTGXM_H 1

#include <stdio.h>
#include <io.h>
#include <time.h>
#include <zlib.h>
#include <windows.h>
#include "../tileset.h"

#define APP_NAME            "viewtgxm"
#define EXE_NAME            "viewtgxm.exe"
#define TRACE_NAME          "viewtgxm.trace"

#define GPS_MONITOR_PGM     "gpsmonitor.exe"
#define GPS_MONITOR_CLASS   "tsGpsMonitorClass"
#define GPS_MONITOR_SMO     "tsGpsMonitorSharedMemory"

#define MOVE_EAST_WEST              1
#define MOVE_NORTH_SOUTH            2

#define IDM_FILE_POPPOS             0

#define IDM_OPEN                    1
#define IDM_RPRT                    2
#define IDM_EXIT                    3

#define IDM_VIEW_POPPOS             1

#define IDM_TILES                   4
#define IDM_GPSTRACE                5
#define IDM_LLGRID                  6
#define IDM_TILEBDY                 7

#define IDM_GPS_POPPOS              2

#define IDM_GPSON                   8
#define IDM_GPSMOVE                 9

#define IDM_ABOUT                  10

#ifndef FILENAME_MAX
#define FILENAME_MAX              256
#endif

#define TILE_COLOR         RGB(0xf0,0x00,0x40)
#define TILE_WIDTH                  2

#define GRID_COLOR         RGB(0x40,0x40,0xff)
#define GRID_WIDTH                  0

#define TRACE_COLOR        RGB(0x00,0xf0,0x40)
#define TRACE_WIDTH                 2

#define DRAW_PIXELS_A               1
#define DRAW_PIXELS_B               2
#define DRAW_GPSTRACE               4
#define DRAW_LATLNG                 8
#define DRAW_TILEBDY               16

#define LINE_LENGTH               509                 /* this is C standard */

#define TILES_IN_MEM               64              /* tiles saved in memory */
#define LOAD_COUNTER_MAX   0xffffffff         /* maximum unsigned int value */

#define UM_GPS_MONITOR_REQUEST      (WM_APP + 21)      /* request locations */
#define UM_GPS_MONITOR_ACQ          (WM_APP + 22)    /* monitor acknowleges */
#define UM_GPS_POSITION_MESSAGE     (WM_APP + 23) /* monitor gives location */

#define TS_PX_LAT_HEIGHT(p) ((p)->highLatY - (p)->lowLatY)

/* if GPS location is within 1/n of width/height to map edge, recenter map. */
#define EDGE_FRACTION               8

/* External file epoch is the begining of second 0, on 2000-01-01.
   The day is TS_EPOCH days after 1900-01-01 */
#define TS_EPOCH                36524

struct fileMapping {                           /* Win32 file mapping params */
   void *lpvFile;            /* NULL if none open ("master file indicator") */
   int fileSize;
   HANDLE hFileMap;
   HANDLE hFile;
   unsigned char *pixelBuffer;       /* uncompressed pixels saved in memory */
   };

struct tileInMemory {                  /* corresponds to pixel buffer slots */
   int column, row;                              /* tile coordinates ("id") */
   unsigned loadCounter;              /* first-in-first-replaced (0: empty) */
   };

struct mapWindow {                                            /* map window */
   RECT cRect;                         /* window 'client-area' rectangle... */
   struct tsPixel crcTile; /* ...its client rect. center in tileset coords. */
   int lowLatY;         /* tileset coordinate corresponding to low latitude */
   int highLatY;                         /* as above, tileset high latitude */
   };

struct tgxmViewControl {
   struct mapWindow mpw;                       /* map window geometry, etc. */
   BITMAPINFO bm;             /* describes the uncompressed tiles as bitmap */
   struct fileMapping fm;               /* used internally, by file mapping */
   char fn[FILENAME_MAX + 2];                        /* currently open tgxm */
   const struct tsHdr *tsh;                         /* tile-set file header */
   const struct tsRect *tsrp;                     /* rectangular projection */
   const struct tsTile1 *tstt1;                 /* tile table, single layer */
   const struct tsTile2 *tstt2;                  /* tile table, both layers */
   const struct tsColor *tspltt;                         /* palette, if any */
   const struct tsColor *tstrsp;                         /* palette, if any */
   const char *dscr_l1;                              /* description, line 1 */
   const char *dscr_l2;                              /* description, line 2 */
   int tileSize;                                 /* uncompressed pixel size */
   int sLineSize;                            /* uncompressed scan line size */
   unsigned tileLoadCounter;                     /* used to stamp upon load */
   struct tileInMemory tile[TILES_IN_MEM];      /* description of tileSpace */
   HWND hWndGpsMntr;                                /* GPS monitor - active */
   HANDLE smm;                          /* shared memory mapping from above */
   struct tsGpsLocation *gpsLoc;                /* gps locations, read-only */
   int mGpsLoc;                  /* maximum number of elements in the above */
   int lGpsLoc;                          /* number of last received element */
   int drawFlags;                                           /* what to draw */
   int mapMovable;                                       /* GPS "moves" map */
   struct tsTilePixel tspxPrev;   /* previous gps location as tileset pixel */
   int dispInfo;                                       /* display info text */
   HPEN tileBoundaryPen;                         /* to draw tile boundaries */
   HPEN latLngPen;                       /* to draw latitude/longitude grid */
   HPEN tracePen;                                      /* to draw GPS trace */
   HFONT font0;                                               /* small font */
   HFONT font1;                                              /* medium font */
   HFONT font2;                                               /* large font */
   HBRUSH bakBrush;                                     /* background brush */
   COLORREF bakCol;                                     /* backbround color */
   };

/* in viewtgxm.c */
int WINAPI WinMain(HINSTANCE, HINSTANCE, PSTR, int);

/* in viewtgxm_winproc.c */
LRESULT CALLBACK mapWinProc(HWND, UINT, WPARAM, LPARAM);

/* in viewtgxm_file.c */
int openTgxm(struct tgxmViewControl *, const char *);
void closeTgxm(struct fileMapping *);
int loadTile(struct tgxmViewControl *, int, int);
int selectTileset(HWND, char *);

/* in viewtgxm_graphics.c */
void paintWindowTiles(HDC, struct tgxmViewControl *);
void drawTileBoundaries(HDC, struct tgxmViewControl *);
void drawLatLongGrid(HDC, struct tgxmViewControl *);
void drawGPSTrace(HDC, struct tgxmViewControl *);
int gridDensity(double);
void updateScrollGeom(HWND, struct tgxmViewControl *);
void displayInfo(HWND, HDC, struct tgxmViewControl *);

/* in viewtgxm_util.c */
int windowLimits(const struct tsHdr *, const struct tsRect *, const RECT *, const struct tsPixel *, int *, int *, int *, int *);
int resizeWindow(struct tgxmViewControl *, RECT *);
int moveWindow(struct tgxmViewControl *, int, int);
int positionWindow(struct tgxmViewControl *, struct tsLatLng *);
void win2ts(struct mapWindow *, int, int, int *, int *);
void ts2win(struct mapWindow *, int, int, int *, int *);
int windowTiles(struct tgxmViewControl *, int *, int *, int *, int *);
void reportFile(HWND, struct tgxmViewControl *);
void userInfo(char *, ...);
void userError(char *, ...);
void debugTrace(char *, ...);

#endif
